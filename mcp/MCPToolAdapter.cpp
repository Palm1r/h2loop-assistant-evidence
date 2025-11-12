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

#include "MCPToolAdapter.hpp"

namespace QodeAssist::MCP {

MCPToolAdapter::MCPToolAdapter(
    const MCPToolInfo &toolInfo, MCPClientManager *clientManager, QObject *parent)
    : LLMCore::BaseTool(parent)
    , m_toolInfo(toolInfo)
    , m_clientManager(clientManager)
{}

QString MCPToolAdapter::name() const
{
    return m_toolInfo.name;
}

QString MCPToolAdapter::stringName() const
{
    return QString("mcp_%1_%2").arg(m_toolInfo.serverName, m_toolInfo.name);
}

QString MCPToolAdapter::description() const
{
    return m_toolInfo.description;
}

QJsonObject MCPToolAdapter::getDefinition(LLMCore::ToolSchemaFormat format) const
{
    if (format == LLMCore::ToolSchemaFormat::OpenAI) {
        QJsonObject functionDef;
        functionDef["name"] = m_toolInfo.name;
        functionDef["description"] = m_toolInfo.description;

        // Use the input schema from MCP tool info
        if (!m_toolInfo.inputSchema.isEmpty()) {
            functionDef["parameters"] = m_toolInfo.inputSchema;
        } else {
            // Fallback to basic object schema
            QJsonObject parameters;
            parameters["type"] = "object";
            parameters["properties"] = QJsonObject();
            functionDef["parameters"] = parameters;
        }

        QJsonObject toolDef;
        toolDef["type"] = "function";
        toolDef["function"] = functionDef;
        return toolDef;
    } else {
        // For other formats, return basic definition
        QJsonObject definition;
        definition["name"] = m_toolInfo.name;
        definition["description"] = m_toolInfo.description;

        // Use the input schema from MCP tool info
        if (!m_toolInfo.inputSchema.isEmpty()) {
            definition["parameters"] = m_toolInfo.inputSchema;
        } else {
            // Fallback to basic object schema
            QJsonObject parameters;
            parameters["type"] = "object";
            parameters["properties"] = QJsonObject();
            definition["parameters"] = parameters;
        }

        return definition;
    }
}

LLMCore::ToolPermissions MCPToolAdapter::requiredPermissions() const
{
    // MCP tools can potentially access various resources
    // For now, assume they need network access and possibly filesystem
    return LLMCore::ToolPermissions(
        LLMCore::ToolPermission::NetworkAccess | LLMCore::ToolPermission::FileSystemRead);
}

QFuture<QString> MCPToolAdapter::executeAsync(const QJsonObject &input)
{
    return m_clientManager->executeTool(m_toolInfo.serverName, m_toolInfo.name, input);
}

} // namespace QodeAssist::MCP