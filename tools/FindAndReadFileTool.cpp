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

#include "FindAndReadFileTool.hpp"
#include "ToolExceptions.hpp"

#include <logger/Logger.hpp>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QtConcurrent>

namespace QodeAssist::Tools {

FindAndReadFileTool::FindAndReadFileTool(QObject *parent)
    : BaseTool(parent)
    , m_ignoreManager(new Context::IgnoreManager(this))
{}

QString FindAndReadFileTool::name() const
{
    return "find_and_read_file";
}

QString FindAndReadFileTool::stringName() const
{
    return "Finding and reading file";
}

QString FindAndReadFileTool::description() const
{
    return "Search for a file by name/path and optionally read its content with line numbers. "
           "Returns the best matching file and its line-numbered content. "
           "Supports reading specific line ranges for efficiency.";
}

QJsonObject FindAndReadFileTool::getDefinition(LLMCore::ToolSchemaFormat format) const
{
    QJsonObject properties;

    properties["query"] = QJsonObject{
        {"type", "string"},
        {"description", "Filename, partial name, or path to search for (case-insensitive)"}};

    properties["file_pattern"] = QJsonObject{
        {"type", "string"}, {"description", "File pattern filter (e.g., '*.cpp', '*.h', '*.qml')"}};

    properties["read_content"] = QJsonObject{
        {"type", "boolean"},
        {"description",
         "Whether to read the full file content (true) or use targeted search mode (false). "
         "Defaults to true. Automatically set to false when search_query is provided."}};

    properties["search_query"] = QJsonObject{
        {"type", "string"},
        {"description",
         "Specific query to search for in the file (function name, variable, class, etc.). "
         "When provided, automatically enables targeted search mode (read_content=false) "
         "to find and return only the relevant code elements, such as when user asks about "
         "one function, a specific class member, variable definition, or any code element "
         "without needing the entire file content"}};

    properties["start_line"] = QJsonObject{
        {"type", "integer"},
        {"description", "Starting line number to read from (1-based, default: 1)"},
        {"minimum", 1}};

    properties["end_line"] = QJsonObject{
        {"type", "integer"},
        {"description", "Ending line number to read to (1-based, default: end of file)"},
        {"minimum", 1}};

    QJsonObject definition;
    definition["type"] = "object";
    definition["properties"] = properties;
    definition["required"] = QJsonArray{"query"};

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

LLMCore::ToolPermissions FindAndReadFileTool::requiredPermissions() const
{
    return LLMCore::ToolPermission::FileSystemRead;
}

QFuture<QString> FindAndReadFileTool::executeAsync(const QJsonObject &input)
{
    return QtConcurrent::run([this, input]() -> QString {
        QString query = input["query"].toString().trimmed();
        if (query.isEmpty()) {
            throw ToolInvalidArgument("Query parameter is required");
        }

        QString filePattern = input["file_pattern"].toString();
        bool readContent = input["read_content"].toBool(true);
        QString searchQuery = input["search_query"].toString();
        int startLine = input["start_line"].toInt(0);
        int endLine = input["end_line"].toInt(0);

        LOG_MESSAGE(
            QString("FindAndReadFileTool: Searching for '%1' (pattern: %2, read: %3, lines: %4-%5)")
                .arg(query, filePattern.isEmpty() ? "none" : filePattern)
                .arg(readContent)
                .arg(startLine > 0 ? QString::number(startLine) : "1")
                .arg(endLine > 0 ? QString::number(endLine) : "end"));

        if (!searchQuery.isEmpty()) {
            readContent = false;
        }

        FileSearchUtils::FileMatch bestMatch
            = FileSearchUtils::findBestMatch(query, filePattern, 10, m_ignoreManager);

        if (bestMatch.absolutePath.isEmpty()) {
            return QString("No file found matching '%1'").arg(query);
        }

        if (readContent) {
            bestMatch.content = FileSearchUtils::readFileContentWithLineNumbers(
                bestMatch.absolutePath, startLine, endLine);
            if (bestMatch.content.isNull()) {
                bestMatch.error = "Could not read file";
            }
        } else {
            // Use ctags to find specific content based on searchQuery
            if (searchQuery.isEmpty()) {
                bestMatch.error = "search_query parameter is required when read_content is false";
            } else {
                QList<Tag> tags = CtagUtils::parseCtagsJson(
                    CtagUtils::runCtags(bestMatch.absolutePath));
                QList<Tag> matchingTags = findMatchingTags(tags, searchQuery);
                if (!matchingTags.isEmpty()) {
                    bestMatch.content = readLines(bestMatch.absolutePath, matchingTags);
                } else {
                    bestMatch.error = QString("No tags found matching '%1'").arg(searchQuery);
                }
            }
        }

        return formatResult(bestMatch, readContent);
    });
}

QList<Tag> FindAndReadFileTool::findMatchingTags(const QList<Tag> &tags, const QString &query) const
{
    QList<Tag> matchingTags;
    for (const Tag &tag : tags) {
        if (tag.matchesQuery(query)) {
            matchingTags.append(tag);
        }
    }
    return matchingTags;
}

QString FindAndReadFileTool::readLines(const QString &filePath, const QList<Tag> &tags) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: Could not open file for reading");
    }

    QStringList lines;
    QTextStream in(&file);
    int lineNumber = 1;
    while (!in.atEnd()) {
        QString line = in.readLine();
        // Check if this line is within any of the tag ranges
        for (const Tag &tag : tags) {
            if (lineNumber >= tag.line && (tag.endLine == 0 || lineNumber <= tag.endLine)) {
                lines.append(QString("%1: %2").arg(lineNumber).arg(line));
                break;
            }
        }
        lineNumber++;
    }
    file.close();

    if (lines.isEmpty()) {
        return "No matching content found";
    }

    return lines.join("\n");
}

QString FindAndReadFileTool::formatResult(
    const FileSearchUtils::FileMatch &match, bool readContent) const
{
    QString result
        = QString("Found file: %1\nAbsolute path: %2").arg(match.relativePath, match.absolutePath);

    if (!match.error.isEmpty()) {
        result += QString("\nError: %1").arg(match.error);
    } else if (!match.content.isEmpty()) {
        result += QString("\n\n=== Content ===\n%1").arg(match.content);
    }

    return result;
}

} // namespace QodeAssist::Tools