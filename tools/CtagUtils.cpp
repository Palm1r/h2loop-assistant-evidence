#include "CtagUtils.hpp"

#include <logger/Logger.hpp>
#include <QCoreApplication>
#include <QFile>
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

} // namespace QodeAssist::Tools