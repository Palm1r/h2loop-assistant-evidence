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

#include "CtagsTool.hpp"
#include "ToolExceptions.hpp"

#include <logger/Logger.hpp>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QtConcurrent>

namespace QodeAssist::Tools {

CtagsTool::CtagsTool(QObject *parent)
    : BaseTool(parent)
    , m_ignoreManager(new Context::IgnoreManager(this))
{}

QString CtagsTool::name() const
{
    return "generate_ctags";
}

QString CtagsTool::stringName() const
{
    return "Generate CTags";
}

QString CtagsTool::description() const
{
    return "Generate ctags information for a file to understand its structure (classes, functions, "
           "variables, etc.). "
           "This provides structural metadata only - for actual code content, use file reading "
           "tools.";
}

QJsonObject CtagsTool::getDefinition(LLMCore::ToolSchemaFormat format) const
{
    QJsonObject properties;

    properties["query"] = QJsonObject{
        {"type", "string"},
        {"description", "Filename, partial name, or path to search for (case-insensitive)"}};

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

LLMCore::ToolPermissions CtagsTool::requiredPermissions() const
{
    return LLMCore::ToolPermission::FileSystemRead;
}

QFuture<QString> CtagsTool::executeAsync(const QJsonObject &input)
{
    return QtConcurrent::run([this, input]() -> QString {
        QString query = input["query"].toString().trimmed();
        if (query.isEmpty()) {
            throw ToolInvalidArgument("Query parameter is required");
        }

        LOG_MESSAGE(QString("CtagsTool: Searching for '%1' to generate ctags").arg(query));

        FileSearchUtils::FileMatch bestMatch
            = FileSearchUtils::findBestMatch(query, QString(), 10, m_ignoreManager);

        if (bestMatch.absolutePath.isEmpty()) {
            return QString("No file found matching '%1'").arg(query);
        }

        QString ctagsOutput = runCtags(bestMatch.absolutePath);
        if (ctagsOutput.isEmpty()) {
            return QString("Failed to generate ctags for file '%1'").arg(bestMatch.relativePath);
        }

        QString parsedOutput = parseCtagsOutput(ctagsOutput);

        return QString(
                   "File: %1\nAbsolute path: %2\n\n"
                   "**CTAGS STRUCTURAL INFORMATION**\n"
                   "This shows the file's structure (classes, functions, variables, etc.) for "
                   "reference only.\n"
                   "For actual code content and editing, use file reading and editing tools.\n\n"
                   "%3")
            .arg(bestMatch.relativePath)
            .arg(bestMatch.absolutePath)
            .arg(parsedOutput);
    });
}

QString CtagsTool::runCtags(const QString &filePath) const
{
    QProcess process;
    process.setProgram("ctags");
    process.setArguments(
        {"--output-format=json", "--fields=+neS", "--sort=no", "--extras=+p", filePath});

    process.start();
    if (!process.waitForFinished(30000)) { // 30 second timeout
        LOG_MESSAGE(QString("Ctags process timed out for file: %1").arg(filePath));
        return QString();
    }

    if (process.exitCode() != 0) {
        LOG_MESSAGE(QString("Ctags failed with exit code %1 for file: %2")
                        .arg(process.exitCode())
                        .arg(filePath));
        return QString();
    }

    return process.readAllStandardOutput();
}

QString CtagsTool::parseCtagsOutput(const QString &output) const
{
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QStringList parsedTags;

    for (const QString &line : lines) {
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (!doc.isObject()) {
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj["_type"].toString();

        if (type == "ptag") {
            // Program tag (metadata)
            QString name = obj["name"].toString();
            QString path = obj["path"].toString();
            QString pattern = obj["pattern"].toString();
            parsedTags.append(QString("META: %1 = %2 (%3)").arg(name, path, pattern));
        } else if (type == "tag") {
            // Actual tag
            QString name = obj["name"].toString();
            QString kind = obj["kind"].toString();
            QString scope = obj.contains("scope") ? obj["scope"].toString() : "";
            QString signature = obj.contains("signature") ? obj["signature"].toString() : "";
            int lineNum = obj["line"].toInt();

            QString tagInfo = QString("%1: %2").arg(kind, name);
            if (!signature.isEmpty()) {
                tagInfo += QString(" %1").arg(signature);
            }
            if (!scope.isEmpty()) {
                tagInfo += QString(" [in %1]").arg(scope);
            }
            tagInfo += QString(" (line %1)").arg(lineNum);

            parsedTags.append(tagInfo);
        }
    }

    if (parsedTags.isEmpty()) {
        return "No tags found in the file.";
    }

    return parsedTags.join("\n");
}

} // namespace QodeAssist::Tools