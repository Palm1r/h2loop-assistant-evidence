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

#include "MCPSettings.hpp"
#include <QJsonDocument>

namespace QodeAssist::Settings {

MCPSettings::MCPSettings()
{
    setAutoApply(false);

    enableMCP.setSettingsKey("MCP/EnableMCP");
    enableMCP.setLabelText("Enable MCP Integration");
    enableMCP.setToolTip("Enable Model Context Protocol server integration");

    mcpServers.setSettingsKey("MCP/Servers");
    mcpServers.setLabelText("MCP Servers");
    mcpServers.setToolTip("JSON configuration of MCP servers");

    addMCPServer.setSettingsKey("MCP/AddServer");
    addMCPServer.setLabelText("Add Server");
    addMCPServer.setToolTip("Add a new MCP server configuration");

    removeMCPServer.setSettingsKey("MCP/RemoveServer");
    removeMCPServer.setLabelText("Remove Server");
    removeMCPServer.setToolTip("Remove selected MCP server");

    testMCPServer.setSettingsKey("MCP/TestServer");
    testMCPServer.setLabelText("Test Server");
    testMCPServer.setToolTip("Test connection to MCP server");

    setupConnections();
}

QList<QJsonObject> MCPSettings::getServerConfigs() const
{
    QList<QJsonObject> configs;
    if (mcpServers.value().isEmpty()) {
        return configs;
    }

    QJsonDocument doc = QJsonDocument::fromJson(mcpServers.value().toUtf8());
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        for (const QJsonValue &value : array) {
            if (value.isObject()) {
                configs.append(value.toObject());
            }
        }
    }
    return configs;
}

void MCPSettings::setServerConfigs(const QList<QJsonObject> &configs)
{
    QJsonArray array;
    for (const QJsonObject &config : configs) {
        array.append(config);
    }
    QJsonDocument doc(array);
    mcpServers.setValue(QString::fromUtf8(doc.toJson()));
}

void MCPSettings::addServerConfig(const QJsonObject &config)
{
    QList<QJsonObject> configs = getServerConfigs();
    configs.append(config);
    setServerConfigs(configs);
}

void MCPSettings::removeServerConfig(const QString &serverName)
{
    QList<QJsonObject> configs = getServerConfigs();
    for (int i = 0; i < configs.size(); ++i) {
        if (configs[i]["name"].toString() == serverName) {
            configs.removeAt(i);
            break;
        }
    }
    setServerConfigs(configs);
}

void MCPSettings::setupConnections()
{
    // Connections for button actions would be set up here
    // For now, these are placeholders for UI integration
}

MCPSettings &mcpSettings()
{
    static MCPSettings settings;
    return settings;
}

} // namespace QodeAssist::Settings