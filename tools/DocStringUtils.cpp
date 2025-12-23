#include "DocStringUtils.hpp"

#include <logger/Logger.hpp>
#include <QJsonDocument>
#include <QProcess>
#include <QTemporaryFile>

namespace QodeAssist::Tools {

// Embedded YAML rules for ast-grep
const QString DOCSTRING_RULES_YAML = R"(
id: find-func-docs
message: "Found function docstring"
severity: info
language: C++

rule:
  kind: comment
  pattern: $DOC
  precedes:
    kind: function_definition
)";

DocStringUtils::DocStringUtils(QObject *parent)
    : QObject(parent)
{}

QJsonDocument DocStringUtils::generateDocstrings(const QString &filePath)
{
    if (filePath.isEmpty()) {
        LOG_MESSAGE("DocStringUtils: File path is empty");
        return QJsonDocument();
    }

    if (!QFile::exists(filePath)) {
        LOG_MESSAGE(QString("DocStringUtils: File '%1' does not exist").arg(filePath));
        return QJsonDocument();
    }

    // Create a temporary file for the rules
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    if (!tempFile.open()) {
        LOG_MESSAGE("DocStringUtils: Failed to create temporary file for rules");
        return QJsonDocument();
    }
    tempFile.write(DOCSTRING_RULES_YAML.toUtf8());
    tempFile.close();

    QString rulesFile = tempFile.fileName();

    QStringList arguments;
    arguments << "scan" << "--json" << "-r" << rulesFile << filePath;

    QString astGrepProgram;

    // First try bundled ast-grep
    QString pluginDir = QCoreApplication::applicationDirPath();
    QString bundledAstGrep;
#ifdef Q_OS_WIN
    bundledAstGrep = pluginDir + "/app-aarch64-pc-windows-msvc/ast-grep.exe";
#elif defined(Q_OS_LINUX)
    bundledAstGrep = pluginDir + "/ast-grep-app-x86_64-unknown-linux-gnu/ast-grep";
#endif

    LOG_MESSAGE(QString("Checking for bundled ast-grep at: %1").arg(bundledAstGrep));

    if (QFile::exists(bundledAstGrep)) {
        astGrepProgram = bundledAstGrep;
    } else {
        // Fallback to system ast-grep
        astGrepProgram = "ast-grep";
    }

    LOG_MESSAGE(
        QString("DocStringUtils: Executing ast-grep with args: %1").arg(arguments.join(" ")));

    QProcess process;
    process.setProgram("ast-grep");
    process.setArguments(arguments);
    process.start();
    if (!process.waitForFinished(30000)) { // 30 second timeout
        process.kill();
        LOG_MESSAGE("DocStringUtils: ast-grep process timed out");
        return QJsonDocument();
    }

    int exitCode = process.exitCode();
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QString errorOutput = QString::fromUtf8(process.readAllStandardError());

    if (exitCode != 0) {
        LOG_MESSAGE(QString("DocStringUtils: ast-grep failed with exit code %1\nError: %2")
                        .arg(exitCode)
                        .arg(errorOutput));
        return QJsonDocument();
    }

    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    if (doc.isNull()) {
        LOG_MESSAGE(
            QString("DocStringUtils: Invalid JSON output from ast-grep\nOutput: %1").arg(output));
        return QJsonDocument();
    }

    LOG_MESSAGE("DocStringUtils: Docstring generation completed successfully");
    return filterDocString(doc);
}

QJsonDocument DocStringUtils::filterDocString(const QJsonDocument &rawDoc)
{
    if (!rawDoc.isArray()) {
        return rawDoc;
    }

    QJsonArray filteredArray;

    for (const QJsonValue &value : rawDoc.array()) {
        if (!value.isObject()) {
            continue;
        }

        QJsonObject obj = value.toObject();
        QJsonObject filteredObj;

        // Keep only essential fields
        if (obj.contains("file")) {
            filteredObj["file"] = obj["file"];
        }
        if (obj.contains("language")) {
            filteredObj["language"] = obj["language"];
        }

        // Filter labels to simplified format
        if (obj.contains("labels") && obj["labels"].isArray()) {
            QJsonArray originalLabels = obj["labels"].toArray();
            QJsonArray filteredLabels;

            for (const QJsonValue &labelValue : originalLabels) {
                if (!labelValue.isObject()) {
                    continue;
                }

                QJsonObject labelObj = labelValue.toObject();
                QJsonObject filteredLabel;

                if (labelObj.contains("text")) {
                    filteredLabel["text"] = labelObj["text"];
                }

                if (labelObj.contains("range") && labelObj["range"].isObject()) {
                    QJsonObject rangeObj = labelObj["range"].toObject();
                    QJsonObject simpleRange;

                    if (rangeObj.contains("start") && rangeObj["start"].isObject()) {
                        QJsonObject startObj = rangeObj["start"].toObject();
                        if (startObj.contains("line")) {
                            startObj["line"] = startObj["line"].toInt() + 1;
                        }
                        simpleRange["start"] = startObj;
                    }
                    if (rangeObj.contains("end") && rangeObj["end"].isObject()) {
                        QJsonObject endObj = rangeObj["end"].toObject();
                        if (endObj.contains("line")) {
                            endObj["line"] = endObj["line"].toInt() + 1;
                        }
                        simpleRange["end"] = endObj;
                    }

                    if (!simpleRange.isEmpty()) {
                        filteredLabel["range"] = simpleRange;
                    }
                }

                if (!filteredLabel.isEmpty()) {
                    filteredLabels.append(filteredLabel);
                }
            }

            if (!filteredLabels.isEmpty()) {
                filteredObj["labels"] = filteredLabels;
            }
        }

        if (!filteredObj.isEmpty()) {
            filteredArray.append(filteredObj);
        }
    }

    return QJsonDocument(filteredArray);
}

} // namespace QodeAssist::Tools