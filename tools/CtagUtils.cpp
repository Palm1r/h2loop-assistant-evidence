#include "CtagUtils.hpp"
#include "DocStringUtils.hpp"

#include <logger/Logger.hpp>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace QodeAssist::Tools {

QString CtagUtils::runCtags(const QString &filePath)
{
    QString ctagsProgram;

    // First try bundled ctags
    QString pluginDir = QCoreApplication::applicationDirPath();
    QString bundledCtags;
    LOG_MESSAGE(QString("Plugin directory for ctags: %1").arg(pluginDir));
#ifdef Q_OS_WIN
    bundledCtags = pluginDir + "/ctags-p6.2.20251130.0-clang-x64/ctags.exe";
#elif defined(Q_OS_LINUX)
    bundledCtags = pluginDir + "/ctags-2025.11.27-1-x86_64-linux/ctags";
#endif
    LOG_MESSAGE(QString("Checking for bundled ctags at: %1").arg(bundledCtags));

    if (QFile::exists(bundledCtags)) {
        ctagsProgram = bundledCtags;
    } else {
        // Fallback to system ctags
        // QProcess testProcess;
        // testProcess.start("ctags", {"--version"});
        // if (!testProcess.waitForFinished(5000) || testProcess.exitCode() != 0) {
        //     LOG_MESSAGE("ctags command not found in system PATH or bundled location");
        //     return QString();
        // }
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

QList<Tag> CtagUtils::parseCtagsJson(const QString &output)
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

QString CtagUtils::filterCtagsOutput(const QString &output)
{
    QStringList filteredLines;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (!doc.isObject()) {
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj["_type"].toString();

        if (type == "tag") {
            // Remove _type and path fields
            obj.remove("_type");
            obj.remove("path");

            // Convert back to JSON string
            QJsonDocument filteredDoc(obj);
            filteredLines.append(QString::fromUtf8(filteredDoc.toJson(QJsonDocument::Compact)));
        }
    }

    return filteredLines.join('\n');
}

QString CtagUtils::mergeDocstringsWithCtags(
    const QString &ctagsOutput, const QJsonDocument &docstrings)
{
    QStringList ctagsLines = ctagsOutput.split('\n', Qt::SkipEmptyParts);
    QStringList enhancedLines;

    for (const QString &line : ctagsLines) {
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (!doc.isObject()) {
            enhancedLines.append(line);
            continue;
        }

        QJsonObject ctagObj = doc.object();
        QString type = ctagObj["_type"].toString();

        if (type == "tag" && ctagObj.contains("line") && ctagObj.contains("end")) {
            int ctagLine = ctagObj["line"].toInt();
            int ctagEnd = ctagObj["end"].toInt();

            // Find matching docstring
            if (docstrings.isArray()) {
                for (const QJsonValue &docstringValue : docstrings.array()) {
                    if (!docstringValue.isObject()) {
                        continue;
                    }

                    QJsonObject docstringObj = docstringValue.toObject();
                    if (!docstringObj.contains("labels") || !docstringObj["labels"].isArray()) {
                        continue;
                    }

                    QJsonArray labels = docstringObj["labels"].toArray();
                    if (labels.size() < 2) {
                        continue;
                    }

                    // Check if labels[1] (function) matches ctag line range
                    // labels[0] = info about docstring; labels[1] = info about that function
                    QJsonObject functionLabel = labels[1].toObject();
                    if (functionLabel.contains("range") && functionLabel["range"].isObject()) {
                        QJsonObject range = functionLabel["range"].toObject();
                        if (range.contains("start") && range["start"].isObject()
                            && range.contains("end") && range["end"].isObject()) {
                            QJsonObject start = range["start"].toObject();
                            QJsonObject end = range["end"].toObject();

                            int funcStartLine = start["line"].toInt();
                            int funcEndLine = end["line"].toInt();

                            if (funcStartLine == ctagLine && funcEndLine == ctagEnd) {
                                // Found matching docstring, add it to ctag
                                QJsonObject docstringLabel = labels[0].toObject();
                                if (docstringLabel.contains("text")) {
                                    ctagObj["docstring"] = docstringLabel["text"];
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Convert back to JSON
        QJsonDocument enhancedDoc(ctagObj);
        enhancedLines.append(QString::fromUtf8(enhancedDoc.toJson(QJsonDocument::Compact)));
    }

    return enhancedLines.join('\n');
}

QString CtagUtils::generateCtagforFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QStringList supportedExtensions = {"c", "cpp", "h"};

    if (supportedExtensions.contains(suffix)) {
        QString ctagsOutput = runCtags(filePath);
        if (!ctagsOutput.isEmpty()) {
            // Generate docstrings for the file
            DocStringUtils docUtils;
            QJsonDocument docstrings = docUtils.generateDocstrings(filePath);

            QString enhancedCtags = ctagsOutput;

            if (!docstrings.isNull() && docstrings.isArray()) {
                LOG_MESSAGE(QString("Docstrings generated for file: %1\n%2")
                                .arg(filePath)
                                .arg(QString::fromUtf8(docstrings.toJson(QJsonDocument::Indented))));

                enhancedCtags = mergeDocstringsWithCtags(ctagsOutput, docstrings);
            } else {
                LOG_MESSAGE(QString("Failed to generate docstrings for file: %1").arg(filePath));
            }

            return filterCtagsOutput(enhancedCtags);
        }
    }
    return QString();
}

} // namespace QodeAssist::Tools