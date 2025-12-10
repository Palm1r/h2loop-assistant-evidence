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
#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
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
    return "Search for a file by name/path and optionally read its content. "
           "Returns the best matching file and its content.";
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

        if (!searchQuery.isEmpty()) {
            readContent = false;
        }

        FileSearchUtils::FileMatch bestMatch
            = FileSearchUtils::findBestMatch(query, filePattern, 10, m_ignoreManager);

        if (bestMatch.absolutePath.isEmpty()) {
            return QString("No file found matching '%1'").arg(query);
        }

        if (readContent) {
            bestMatch.content = FileSearchUtils::readFileContent(bestMatch.absolutePath);
            if (bestMatch.content.isNull()) {
                bestMatch.error = "Could not read file";
            }
        } else {
            // Use ctags to find specific content based on searchQuery
            if (searchQuery.isEmpty()) {
                bestMatch.error = "search_query parameter is required when read_content is false";
            } else {
                QList<Tag> tags = parseCtagsJson(runCtags(bestMatch.absolutePath));
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

QString FindAndReadFileTool::runCtags(const QString &filePath) const
{
    QString ctagsProgram;

    // First try bundled ctags
    QString pluginDir = QCoreApplication::applicationDirPath();
    QString bundledCtags = pluginDir + "/ctags";
#ifdef Q_OS_WIN
    bundledCtags += ".exe";
#endif
    if (QFile::exists(bundledCtags)) {
        ctagsProgram = bundledCtags;
    } else {
        // Fallback to system ctags
        QProcess whichProcess;
        whichProcess.start("which", {"ctags"});
        if (!whichProcess.waitForFinished(5000) || whichProcess.exitCode() != 0) {
            LOG_MESSAGE("ctags command not found in system PATH or bundled location");
            return QString();
        }
        ctagsProgram = "ctags";
    }

    QProcess process;
    process.setProgram(ctagsProgram);
    process.setArguments(
        {"--output-format=json", "--fields=+neS", "--sort=no", "--extras=+p", filePath});

    process.start();
    if (!process.waitForFinished(30000)) { // 30 second timeout
        LOG_MESSAGE(QString("Ctags process timed out for file: %1").arg(filePath));
        return QString();
    }

    if (process.exitCode() != 0) {
        QString errorOutput = QString::fromUtf8(process.readAllStandardError());
        LOG_MESSAGE(QString("Ctags failed with exit code %1 for file: %2. Error: %3")
                        .arg(process.exitCode())
                        .arg(filePath)
                        .arg(errorOutput));
        return QString();
    }

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    LOG_MESSAGE(QString("Generated ctags for file: %1\nOutput:\n%2").arg(filePath).arg(output));
    return output;
}

QList<Tag> FindAndReadFileTool::parseCtagsJson(const QString &output) const
{
    QList<Tag> tags;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (!doc.isObject()) {
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj["_type"].toString();

        if (type == "tag") {
            Tag tag;
            tag.name = obj["name"].toString();
            tag.kind = obj["kind"].toString();
            tag.scope = obj.contains("scope") ? obj["scope"].toString() : "";
            tag.signature = obj.contains("signature") ? obj["signature"].toString() : "";
            tag.line = obj["line"].toInt();
            tag.endLine = obj.contains("end") ? obj["end"].toInt() : 0;
            tag.pattern = obj["pattern"].toString();
            tags.append(tag);
        }
    }

    return tags;
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
