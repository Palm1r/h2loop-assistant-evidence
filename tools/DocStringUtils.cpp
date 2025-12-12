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
    return doc;
}

} // namespace QodeAssist::Tools