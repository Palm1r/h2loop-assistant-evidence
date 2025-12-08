#include "Logger.hpp"
#include <coreplugin/messagemanager.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

namespace QodeAssist {

Logger &Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_loggingEnabled(false)
    , m_debugLoggingEnabled(false)
    , m_debugLogFilePath()
{}

void Logger::setLoggingEnabled(bool enable)
{
    m_loggingEnabled = enable;
}

bool Logger::isLoggingEnabled() const
{
    return m_loggingEnabled;
}

void Logger::setDebugLoggingEnabled(bool enable)
{
    m_debugLoggingEnabled = enable;
}

bool Logger::isDebugLoggingEnabled() const
{
    return m_debugLoggingEnabled;
}

void Logger::setDebugLogFilePath(const QString &filePath)
{
    m_debugLogFilePath = filePath;
}

QString Logger::debugLogFilePath() const
{
    return m_debugLogFilePath;
}

void Logger::createNewDebugLogFile(const QString &debugLogsDir)
{
    if (!debugLogsDir.isEmpty()) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
        QString logFilePath = QDir(debugLogsDir).filePath("h2loop_" + timestamp + "_debug.log");
        setDebugLogFilePath(logFilePath);
    }
}

void Logger::renameDebugLogFile(const QString &newFilePath)
{
    if (!m_debugLogFilePath.isEmpty() && QFile::exists(m_debugLogFilePath)) {
        QFile file(m_debugLogFilePath);
        if (file.rename(newFilePath)) {
            setDebugLogFilePath(newFilePath);
        }
    } else {
        // If no current file, just set the new path
        setDebugLogFilePath(newFilePath);
    }
}

void Logger::log(const QString &message, bool silent)
{
    if (!m_loggingEnabled)
        return;

    const QString prefixedMessage = QLatin1String("[H2LoopAssistant] ") + message;
    if (silent) {
        Core::MessageManager::writeSilently(prefixedMessage);
    } else {
        Core::MessageManager::writeFlashing(prefixedMessage);
    }

    if (!m_debugLogFilePath.isEmpty()) {
        QFile file(m_debugLogFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " "
                << prefixedMessage << "\n";
            file.close();
        }
    }
}

void Logger::logMessages(const QStringList &messages, bool silent)
{
    if (!m_loggingEnabled)
        return;

    QStringList prefixedMessages;
    for (const QString &message : messages) {
        prefixedMessages << (QLatin1String("[H2LoopAssistant] ") + message);
    }

    if (silent) {
        Core::MessageManager::writeSilently(prefixedMessages);
    } else {
        Core::MessageManager::writeFlashing(prefixedMessages);
    }

    if (!m_debugLogFilePath.isEmpty()) {
        QFile file(m_debugLogFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            for (const QString &message : prefixedMessages) {
                out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " "
                    << message << "\n";
            }
            file.close();
        }
    }
}

void Logger::debugLog(const QString &message, bool silent)
{
    if (!m_debugLoggingEnabled)
        return;

    const QString prefixedMessage = QLatin1String("[H2LoopAssistant DEBUG] ") + message;
    if (silent) {
        Core::MessageManager::writeSilently(prefixedMessage);
    } else {
        Core::MessageManager::writeFlashing(prefixedMessage);
    }

    // Also write to file if file path is set
    if (!m_debugLogFilePath.isEmpty()) {
        QFile file(m_debugLogFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " "
                << prefixedMessage << "\n";
            file.close();
        }
    }
}

void Logger::debugLogMessages(const QStringList &messages, bool silent)
{
    if (!m_debugLoggingEnabled)
        return;

    QStringList prefixedMessages;
    for (const QString &message : messages) {
        prefixedMessages << (QLatin1String("[H2LoopAssistant DEBUG] ") + message);
    }

    if (silent) {
        Core::MessageManager::writeSilently(prefixedMessages);
    } else {
        Core::MessageManager::writeFlashing(prefixedMessages);
    }

    if (!m_debugLogFilePath.isEmpty()) {
        QFile file(m_debugLogFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            for (const QString &message : prefixedMessages) {
                out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " "
                    << message << "\n";
            }
            file.close();
        }
    }
}
} // namespace QodeAssist
