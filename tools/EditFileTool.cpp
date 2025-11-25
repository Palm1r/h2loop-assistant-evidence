/*
 * Copyright (C) 2025 Petr Mironychev
 *
 * This file is part of QodeAssist.
 *
 * QodeAssist is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QodeAssist is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QodeAssist. If not, see <https://www.gnu.org/licenses/>.
 */

#include "EditFileTool.hpp"
#include "ToolExceptions.hpp"

#include <context/ChangesManager.h>
#include <context/ProjectUtils.hpp>
#include <logger/Logger.hpp>
#include <settings/GeneralSettings.hpp>
#include <settings/ToolsSettings.hpp>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>
#include <QtConcurrent>

namespace QodeAssist::Tools {

EditFileTool::EditFileTool(QObject *parent)
    : BaseTool(parent)
    , m_ignoreManager(new Context::IgnoreManager(this))
{}

QString EditFileTool::name() const
{
    return "edit_file";
}

QString EditFileTool::stringName() const
{
    return {"Editing file"};
}

QString EditFileTool::description() const
{
    return "Edit a file using SEARCH/REPLACE blocks. "
           "Provide the filename (or absolute path) and a content string containing one or more "
           "SEARCH/REPLACE blocks. Changes are applied immediately if auto-apply is enabled. "
           "The user can undo or reapply changes at any time."
           "\n\nSEARCH/REPLACE Block Format:"
           "\n```language"
           "\n<<<<<<< SEARCH"
           "\n[exact lines to find in the file]"
           "\n======="
           "\n[new lines to replace with]"
           "\n>>>>>>> REPLACE"
           "\n```"
           "\n\nIMPORTANT RULES:"
           "\n- The SEARCH section must EXACTLY match existing file content (including whitespace)."
           "\n- Each SEARCH/REPLACE block replaces only the FIRST occurrence found."
           "\n- Include enough context lines (3-5) to uniquely identify the location."
           "\n- For EMPTY files or to APPEND: use an empty SEARCH section."
           "\n- Multiple SEARCH/REPLACE blocks can be provided in a single content string."
           "\n- Blocks are processed in order from top to bottom."
           "\n- The system requires 85% similarity for matching. Provide accurate SEARCH content.";
}

QJsonObject EditFileTool::getDefinition(LLMCore::ToolSchemaFormat format) const
{
    QJsonObject properties;

    QJsonObject filenameProperty;
    filenameProperty["type"] = "string";
    filenameProperty["description"]
        = "The filename or absolute path of the file to edit. If only filename is provided, "
          "it will be searched in the project";
    properties["filename"] = filenameProperty;

    QJsonObject contentProperty;
    contentProperty["type"] = "string";
    contentProperty["description"]
        = "Content containing one or more SEARCH/REPLACE blocks. Each block starts with "
          "<<<<<<< SEARCH, followed by the exact content to find, then =======, "
          "then the replacement content, and ends with >>>>>>> REPLACE. "
          "For appending to a file, use an empty SEARCH section.";
    properties["content"] = contentProperty;

    QJsonObject definition;
    definition["type"] = "object";
    definition["properties"] = properties;
    QJsonArray required;
    required.append("filename");
    required.append("content");
    definition["required"] = required;

    switch (format) {
    case LLMCore::ToolSchemaFormat::OpenAI:
        return customizeForOpenAI(definition);
    case LLMCore::ToolSchemaFormat::Claude:
        return customizeForClaude(definition);
    case LLMCore::ToolSchemaFormat::Ollama:
        return customizeForOllama(definition);
    case LLMCore::ToolSchemaFormat::Google:
        return customizeForGoogle(definition);
    }

    return definition;
}

LLMCore::ToolPermissions EditFileTool::requiredPermissions() const
{
    return LLMCore::ToolPermission::FileSystemWrite;
}

QList<EditFileTool::SearchReplaceBlock> EditFileTool::parseSearchReplaceBlocks(const QString &content)
{
    QList<SearchReplaceBlock> blocks;

    // Pattern to match SEARCH/REPLACE blocks
    // Handles optional code fence with language identifier
    QRegularExpression blockPattern(
        R"((?:```\w*\s*\n)?<<<<<<< SEARCH\n([\s\S]*?)\n=======\n([\s\S]*?)\n>>>>>>> REPLACE(?:\n```)?)",
        QRegularExpression::MultilineOption);

    QRegularExpressionMatchIterator it = blockPattern.globalMatch(content);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        SearchReplaceBlock block;
        block.searchContent = match.captured(1);
        block.replaceContent = match.captured(2);
        blocks.append(block);
    }

    return blocks;
}

QFuture<QString> EditFileTool::executeAsync(const QJsonObject &input)
{
    return QtConcurrent::run([this, input]() -> QString {
        QString filename = input["filename"].toString().trimmed();
        QString content = input["content"].toString();
        QString requestId = input["_request_id"].toString();

        if (filename.isEmpty()) {
            throw ToolInvalidArgument("'filename' parameter is required and cannot be empty");
        }

        if (content.isEmpty()) {
            throw ToolInvalidArgument("'content' parameter is required and cannot be empty");
        }

        // Parse SEARCH/REPLACE blocks from content
        QList<SearchReplaceBlock> blocks = parseSearchReplaceBlocks(content);

        if (blocks.isEmpty()) {
            throw ToolInvalidArgument(
                "No valid SEARCH/REPLACE blocks found in content. "
                "Expected format: <<<<<<< SEARCH\\n[search content]\\n=======\\n[replace "
                "content]\\n>>>>>>> REPLACE");
        }

        QString filePath;
        QFileInfo fileInfo(filename);

        if (fileInfo.isAbsolute() && fileInfo.exists()) {
            filePath = filename;
        } else {
            FileSearchUtils::FileMatch match
                = FileSearchUtils::findBestMatch(filename, QString(), 10, m_ignoreManager);

            if (match.absolutePath.isEmpty()) {
                throw ToolRuntimeError(QString(
                                           "File '%1' not found in project. "
                                           "Please provide a valid filename or absolute path.")
                                           .arg(filename));
            }

            filePath = match.absolutePath;
            LOG_MESSAGE(QString("EditFileTool: Found file '%1' at '%2'").arg(filename, filePath));
        }

        QFile file(filePath);
        if (!file.exists()) {
            throw ToolRuntimeError(QString("File does not exist: %1").arg(filePath));
        }

        QFileInfo finalFileInfo(filePath);
        if (!finalFileInfo.isWritable()) {
            throw ToolRuntimeError(
                QString("File is not writable (read-only or permission denied): %1").arg(filePath));
        }

        bool isInProject = Context::ProjectUtils::isFileInProject(filePath);
        if (!isInProject) {
            const auto &settings = Settings::toolsSettings();
            if (!settings.allowAccessOutsideProject()) {
                throw ToolRuntimeError(QString(
                                           "File path '%1' is not within the current project. "
                                           "Enable 'Allow file access outside project' in settings "
                                           "to edit files outside the project.")
                                           .arg(filePath));
            }
            LOG_MESSAGE(QString("Editing file outside project scope: %1").arg(filePath));
        }

        bool autoApply = Settings::toolsSettings().autoApplyFileEdits();

        QJsonArray editResults;
        int blockIndex = 0;

        for (const SearchReplaceBlock &block : blocks) {
            QString editId = QUuid::createUuid().toString(QUuid::WithoutBraces);

            LOG_MESSAGE(QString("EditFileTool: Processing block %1 for %2:")
                            .arg(blockIndex + 1)
                            .arg(filePath));
            LOG_MESSAGE(
                QString("  searchContent length: %1 chars").arg(block.searchContent.length()));
            LOG_MESSAGE(
                QString("  replaceContent length: %1 chars").arg(block.replaceContent.length()));

            if (block.searchContent.length() <= 200) {
                LOG_MESSAGE(QString("  searchContent: '%1'").arg(block.searchContent));
            } else {
                LOG_MESSAGE(QString("  searchContent (first 200 chars): '%1...'")
                                .arg(block.searchContent.left(200)));
            }
            if (block.replaceContent.length() <= 200) {
                LOG_MESSAGE(QString("  replaceContent: '%1'").arg(block.replaceContent));
            } else {
                LOG_MESSAGE(QString("  replaceContent (first 200 chars): '%1...'")
                                .arg(block.replaceContent.left(200)));
            }

            Context::ChangesManager::instance().addFileEdit(
                editId,
                filePath,
                block.searchContent,
                block.replaceContent,
                autoApply,
                false,
                requestId);

            auto edit = Context::ChangesManager::instance().getFileEdit(editId);
            QString status = "pending";
            if (edit.status == Context::ChangesManager::Applied) {
                status = "applied";
            } else if (edit.status == Context::ChangesManager::Rejected) {
                status = "rejected";
            } else if (edit.status == Context::ChangesManager::Archived) {
                status = "archived";
            }

            QJsonObject blockResult;
            blockResult["edit_id"] = editId;
            blockResult["block_index"] = blockIndex;
            blockResult["status"] = status;
            blockResult["status_message"] = edit.statusMessage;
            editResults.append(blockResult);

            LOG_MESSAGE(QString("File edit created: %1 (ID: %2, Status: %3, Block: %4)")
                            .arg(filePath, editId, status)
                            .arg(blockIndex));

            blockIndex++;
        }

        QJsonObject result;
        result["file"] = filePath;
        result["blocks_processed"] = blocks.size();
        result["edits"] = editResults;

        QString resultStr = "QODEASSIST_FILE_EDIT:"
                            + QString::fromUtf8(
                                QJsonDocument(result).toJson(QJsonDocument::Compact));
        return resultStr;
    });
}

} // namespace QodeAssist::Tools
