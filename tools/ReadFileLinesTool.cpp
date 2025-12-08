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

#include "ReadFileLinesTool.hpp"
#include "ToolExceptions.hpp"

#include <logger/Logger.hpp>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QtConcurrent>

namespace QodeAssist::Tools {

ReadFileLinesTool::ReadFileLinesTool(QObject *parent)
    : BaseTool(parent)
    , m_ignoreManager(new Context::IgnoreManager(this))
{}

QString ReadFileLinesTool::name() const
{
    return "read_file_lines";
}

QString ReadFileLinesTool::stringName() const
{
    return "Reading file lines";
}

QString ReadFileLinesTool::description() const
{
    return "Search for a file and read specific lines from it.";
}

QJsonObject ReadFileLinesTool::getDefinition(LLMCore::ToolSchemaFormat format) const
{
    QJsonObject properties;

    properties["query"] = QJsonObject{
        {"type", "string"},
        {"description", "Filename, partial name, or path to search for (case-insensitive)"}};

    properties["text_range"] = QJsonObject{
        {"type", "string"},
        {"description",
         "Natural language range description (e.g., 'lines 35 to 45', 'from 35 to 45', '35-45')"}};

    properties["start_line"]
        = QJsonObject{{"type", "integer"}, {"description", "Starting line number (1-based)"}};

    properties["end_line"] = QJsonObject{
        {"type", "integer"},
        {"description", "Ending line number (1-based, optional - reads to end if not provided)"}};

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

LLMCore::ToolPermissions ReadFileLinesTool::requiredPermissions() const
{
    return LLMCore::ToolPermission::FileSystemRead;
}

QFuture<QString> ReadFileLinesTool::executeAsync(const QJsonObject &input)
{
    return QtConcurrent::run([this, input]() -> QString {
        QString query = input["query"].toString().trimmed();
        if (query.isEmpty()) {
            throw ToolInvalidArgument("Query parameter is required");
        }

        int startLine = -1;
        int endLine = -1;

        // Parse text_range parameter if provided
        if (input.contains("text_range")) {
            QString rangeStr = input["text_range"].toString().trimmed();
            if (!parseTextRange(rangeStr, startLine, endLine)) {
                throw ToolInvalidArgument(
                    "Invalid range format. Examples: '35-45', 'lines 35 to 45', 'from 35 to 45'");
            }
        } else {
            // Use individual line parameters
            startLine = input["start_line"].toInt();
            endLine = input["end_line"].toInt(-1);
        }

        if (startLine < 1) {
            throw ToolInvalidArgument("Start line must be >= 1");
        }

        LOG_MESSAGE(QString("ReadFileLinesTool: Searching for '%1' and reading lines %2-%3")
                        .arg(query)
                        .arg(startLine)
                        .arg(endLine == -1 ? "end" : QString::number(endLine)));

        FileSearchUtils::FileMatch bestMatch
            = FileSearchUtils::findBestMatch(query, QString(), 10, m_ignoreManager);

        if (bestMatch.absolutePath.isEmpty()) {
            return QString("No file found matching '%1'").arg(query);
        }

        QString content = readFileLines(bestMatch.absolutePath, startLine, endLine);
        if (content.isNull()) {
            return QString("Error: Could not read lines from file '%1'").arg(bestMatch.relativePath);
        }

        return QString("File: %1\nAbsolute path: %2\nLines %3-%4:\n\n%5")
            .arg(bestMatch.relativePath)
            .arg(bestMatch.absolutePath)
            .arg(startLine)
            .arg(endLine == -1 ? "end" : QString::number(endLine))
            .arg(content);
    });
}

QString ReadFileLinesTool::readFileLines(const QString &filePath, int startLine, int endLine) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_MESSAGE(QString("Failed to open file: %1").arg(filePath));
        return QString();
    }

    QTextStream in(&file);
    QStringList lines;
    while (!in.atEnd()) {
        lines.append(in.readLine());
    }

    int totalLines = lines.size();
    int startIdx = startLine - 1; // 0-based
    int endIdx = (endLine == -1) ? totalLines - 1 : endLine - 1;

    if (startIdx >= totalLines || startIdx < 0) {
        LOG_MESSAGE(QString("Start line %1 out of range for file with %2 lines")
                        .arg(startLine)
                        .arg(totalLines));
        return QString();
    }

    if (endIdx >= totalLines) {
        endIdx = totalLines - 1;
    }

    if (endIdx < startIdx) {
        LOG_MESSAGE(QString("End line %1 is before start line %2").arg(endLine).arg(startLine));
        return QString();
    }

    QString result;
    for (int i = startIdx; i <= endIdx; ++i) {
        result += QString("%1 | %2\n").arg(i + 1).arg(lines[i]);
    }

    return result;
}

bool ReadFileLinesTool::parseTextRange(const QString &rangeStr, int &startLine, int &endLine) const
{
    // Use regex to extract numbers from various formats:
    // "35-45", "35 to 45", "lines 35 to 45", "from 35 to 45", "35 ---> 45", etc.
    QRegularExpression regex(R"((\d+)\s*(?:to|-|-->|→|--)\s*(\d+))");
    QRegularExpressionMatch match = regex.match(rangeStr.toLower());

    if (match.hasMatch()) {
        bool ok1, ok2;
        startLine = match.captured(1).toInt(&ok1);
        endLine = match.captured(2).toInt(&ok2);
        return ok1 && ok2 && startLine > 0 && endLine > 0;
    }

    // Fallback: try to extract any two numbers
    QStringList numbers;
    QRegularExpression numRegex(R"(\d+)");
    QRegularExpressionMatchIterator it = numRegex.globalMatch(rangeStr);

    while (it.hasNext()) {
        numbers.append(it.next().captured());
        if (numbers.size() >= 2)
            break;
    }

    if (numbers.size() >= 2) {
        bool ok1, ok2;
        startLine = numbers[0].toInt(&ok1);
        endLine = numbers[1].toInt(&ok2);
        return ok1 && ok2 && startLine > 0 && endLine > 0;
    }

    return false;
}

} // namespace QodeAssist::Tools