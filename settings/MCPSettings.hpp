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

#include <utils/aspects.h>
#include <QJsonArray>
#include <QJsonObject>

namespace QodeAssist::Settings {

class MCPSettings : public Utils::AspectContainer
{
    Q_OBJECT
public:
    MCPSettings();

    Utils::BoolAspect enableMCP{this};
    Utils::StringListAspect mcpServerUrls{this}; // List of MCP server URLs

    // Helper methods
    QList<QString> getServerUrls() const;
    void setServerUrls(const QList<QString> &urls);
    void addServerUrl(const QString &url);
    void removeServerUrl(const QString &url);

signals:
    void serverUrlsChanged();
};

MCPSettings &mcpSettings();

} // namespace QodeAssist::Settings