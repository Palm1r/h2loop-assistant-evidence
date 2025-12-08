#pragma once

#include <QObject>
#include <QString>

namespace QodeAssist {

class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger &instance();

    void setLoggingEnabled(bool enable);
    bool isLoggingEnabled() const;

    void setDebugLoggingEnabled(bool enable);
    bool isDebugLoggingEnabled() const;

    void setDebugLogFilePath(const QString &filePath);
    QString debugLogFilePath() const;

    void createNewDebugLogFile(const QString &debugLogsDir);
    void renameDebugLogFile(const QString &newFilePath);

    void log(const QString &message, bool silent = true);
    void logMessages(const QStringList &messages, bool silent = true);

    void debugLog(const QString &message, bool silent = true);
    void debugLogMessages(const QStringList &messages, bool silent = true);

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    bool m_loggingEnabled;
    bool m_debugLoggingEnabled;
    QString m_debugLogFilePath;
};

#define LOG_MESSAGE(msg) QodeAssist::Logger::instance().log(msg)
#define LOG_MESSAGES(msgs) QodeAssist::Logger::instance().logMessages(msgs)
#ifdef QT_DEBUG
#define DEBUG_LOG_MESSAGE(msg) QodeAssist::Logger::instance().debugLog(msg)
#define DEBUG_LOG_MESSAGES(msgs) QodeAssist::Logger::instance().debugLogMessages(msgs)
#else
#define DEBUG_LOG_MESSAGE(msg) ((void) 0)
#define DEBUG_LOG_MESSAGES(msgs) ((void) 0)
#endif

} // namespace QodeAssist
