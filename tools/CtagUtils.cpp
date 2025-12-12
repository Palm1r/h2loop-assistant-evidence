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
    QString bundledCtags = pluginDir + "/ctags";
#ifdef Q_OS_WIN
    bundledCtags += ".exe";
#endif
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
            if (!docstrings.isNull()) {
                LOG_MESSAGE(QString("Docstrings generated for file: %1\n%2")
                                .arg(filePath)
                                .arg(QString::fromUtf8(docstrings.toJson(QJsonDocument::Indented))));
            } else {
                LOG_MESSAGE(QString("Failed to generate docstrings for file: %1").arg(filePath));
            }

            return filterCtagsOutput(ctagsOutput);
        }
    }
    return QString();
}

} // namespace QodeAssist::Tools