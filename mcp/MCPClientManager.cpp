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

#include "MCPClientManager.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QThreadPool>
#include <QTimer>
#include <QtConcurrent>

namespace QodeAssist::MCP {

MCPClientManager::MCPClientManager(QObject *parent)
    : QObject(parent)
{}

MCPClientManager::~MCPClientManager()
{
    for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
        if (it.value()->connected) {
            disconnectFromServer(it.value()->config.name);
        }
        delete it.value();
    }
    m_servers.clear();
}

void MCPClientManager::addServer(const MCPServerConfig &config)
{
    if (m_servers.contains(config.name)) {
        removeServer(config.name);
    }

    auto connection = new ServerConnection();
    connection->config = config;
    connection->connected = false;

    m_servers[config.name] = connection;
}

void MCPClientManager::removeServer(const QString &serverName)
{
    if (m_servers.contains(serverName)) {
        disconnectFromServer(serverName);
        delete m_servers[serverName];
        m_servers.remove(serverName);
    }
}

void MCPClientManager::connectToServer(const QString &serverName)
{
    if (!m_servers.contains(serverName)) {
        emit serverError(serverName, "Server not found");
        return;
    }

    auto connection = m_servers[serverName];
    if (connection->connected) {
        return;
    }

    try {
        if (connection->config.useStdio) {
            // Use stdio client
            auto stdioClient = std::make_unique<mcp::stdio_client>(
                connection->config.command.toStdString());
            if (stdioClient->initialize("H2LoopAssistant", "1.0.0")) {
                connection->client = std::move(stdioClient);
                connection->connected = true;
                updateServerTools(serverName);
                emit serverConnected(serverName);
            } else {
                emit serverError(serverName, "Failed to initialize stdio client");
            }
        } else {
            // Use SSE client
            QString normalizedUrl = connection->config.url;
            if (normalizedUrl.endsWith('/')) {
                normalizedUrl.chop(1);
            }
            auto sseClient = std::make_unique<mcp::sse_client>(normalizedUrl.toStdString());

            // Set client capabilities - FastMCP may require this
            nlohmann::json clientCapabilities
                = {{"experimental", nlohmann::json::object()},
                   {"sampling", nlohmann::json::object()}};
            sseClient->set_capabilities(clientCapabilities);

            if (!connection->config.authToken.isEmpty()) {
                sseClient->set_auth_token(connection->config.authToken.toStdString());
            }
            for (auto it = connection->config.headers.begin();
                 it != connection->config.headers.end();
                 ++it) {
                sseClient->set_header(it.key().toStdString(), it.value().toStdString());
            }
            if (sseClient->initialize("H2LoopAssistant", "1.0.0")) {
                connection->client = std::move(sseClient);
                connection->connected = true;
                updateServerTools(serverName);
                emit serverConnected(serverName);
            } else {
                emit serverError(serverName, "Failed to initialize SSE client");
            }
        }
    } catch (const std::exception &e) {
        emit serverError(serverName, QString("Connection error: %1").arg(QString::fromUtf8(e.what())));
    }
}

void MCPClientManager::disconnectFromServer(const QString &serverName)
{
    if (!m_servers.contains(serverName)) {
        return;
    }

    auto connection = m_servers[serverName];
    if (connection->connected) {
        connection->connected = false;
        connection->tools.clear();
        // Perform client cleanup in background to avoid blocking UI
        QThreadPool::globalInstance()->start([this, serverName, connection]() {
            connection->client.reset();
            emit serverDisconnected(serverName);
        });
    }
}

void MCPClientManager::refreshServers()
{
    QStringList connectedServers;
    for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
        if (it.value()->connected) {
            connectedServers.append(it.key());
        }
    }

    // Disconnect all connected servers
    for (const QString &serverName : std::as_const(connectedServers)) {
        disconnectFromServer(serverName);
    }

    // Reconnect all servers after a short delay to allow disconnections to complete
    QTimer::singleShot(100, this, [this, connectedServers]() {
        for (const QString &serverName : std::as_const(connectedServers)) {
            connectToServer(serverName);
        }
    });
}

QList<MCPToolInfo> MCPClientManager::getAvailableTools() const
{
    QList<MCPToolInfo> allTools;
    for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
        if (it.value()->connected) {
            allTools.append(it.value()->tools);
        }
    }
    return allTools;
}

QFuture<QString> MCPClientManager::executeTool(
    const QString &serverName, const QString &toolName, const QJsonObject &params)
{
    if (!m_servers.contains(serverName) || !m_servers[serverName]->connected) {
        return QtConcurrent::run([]() { return QString("Server not connected"); });
    }

    auto connection = m_servers[serverName];

    return QtConcurrent::run([this, serverName, toolName, params, connection]() {
        try {
            // Convert QJsonObject to nlohmann::json
            auto jsonParams = QJsonDocument(params).toJson().toStdString();
            auto nlohmannParams = nlohmann::json::parse(jsonParams);

            auto result = connection->client->call_tool(toolName.toStdString(), nlohmannParams);

            // Convert result back to QString
            return QString::fromStdString(result.dump());
        } catch (const std::exception &e) {
            return QString("Tool execution error: %1").arg(QString::fromUtf8(e.what()));
        }
    });
}

bool MCPClientManager::isServerConnected(const QString &serverName) const
{
    return m_servers.contains(serverName) && m_servers[serverName]->connected;
}

QStringList MCPClientManager::getServerNames() const
{
    return m_servers.keys();
}

void MCPClientManager::onServerConnectionError(const QString &serverName, const QString &error)
{
    emit serverError(serverName, error);
}

void MCPClientManager::updateServerTools(const QString &serverName)
{
    if (!m_servers.contains(serverName) || !m_servers[serverName]->connected) {
        return;
    }

    auto connection = m_servers[serverName];

    try {
        auto tools = connection->client->get_tools();
        connection->tools.clear();

        for (const auto &tool : tools) {
            MCPToolInfo toolInfo;
            toolInfo.name = QString::fromStdString(tool.name);
            toolInfo.description = QString::fromStdString(tool.description);
            toolInfo.serverName = serverName;

            // Convert parameters_schema to QJsonObject
            toolInfo.inputSchema = QJsonDocument::fromJson(
                                       QByteArray::fromStdString(tool.parameters_schema.dump()))
                                       .object();

            connection->tools.append(toolInfo);
        }

        emit toolsUpdated(serverName);
    } catch (const std::exception &e) {
        qWarning() << "Failed to update tools for server" << serverName << ":" << e.what();
    }
}

} // namespace QodeAssist::MCP