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

#pragma once

#include <memory>
#include <QFuture>
#include <QJsonObject>
#include <QMap>
#include <QObject>

#include "mcp_client.h"
#include "mcp_sse_client.h"
#include "mcp_stdio_client.h"

namespace QodeAssist::MCP {

struct MCPServerConfig
{
    QString name;
    QString url;
    QString command; // for stdio transport
    bool useStdio = false;
    QString authToken;
    QHash<QString, QString> headers;
};

struct MCPToolInfo
{
    QString name;
    QString description;
    QJsonObject inputSchema;
    QString serverName;
};

class MCPClientManager : public QObject
{
    Q_OBJECT

public:
    explicit MCPClientManager(QObject *parent = nullptr);
    ~MCPClientManager() override;

    void addServer(const MCPServerConfig &config);
    void removeServer(const QString &serverName);
    void connectToServer(const QString &serverName);
    void disconnectFromServer(const QString &serverName);

    QList<MCPToolInfo> getAvailableTools() const;
    QFuture<QString> executeTool(
        const QString &serverName, const QString &toolName, const QJsonObject &params);

    bool isServerConnected(const QString &serverName) const;
    QStringList getServerNames() const;

signals:
    void serverConnected(const QString &serverName);
    void serverDisconnected(const QString &serverName);
    void serverError(const QString &serverName, const QString &error);
    void toolsUpdated(const QString &serverName);

private slots:
    void onServerConnectionError(const QString &serverName, const QString &error);

private:
    struct ServerConnection
    {
        MCPServerConfig config;
        std::unique_ptr<mcp::client> client;
        QList<MCPToolInfo> tools;
        bool connected = false;
    };

    QHash<QString, ServerConnection *> m_servers;
    void updateServerTools(const QString &serverName);
};

} // namespace QodeAssist::MCP