#include "Logger.hpp"
#include <coreplugin/messagemanager.h>

namespace QodeAssist {

Logger &Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_loggingEnabled(false)
    , m_debugLoggingEnabled(false)
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
}

#ifdef QT_DEBUG
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
}
#endif
} // namespace QodeAssist
