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

#include "ButtonAspect.hpp"

namespace QodeAssist::Settings {

class MCPSettings : public Utils::AspectContainer
{
public:
    MCPSettings();

    Utils::BoolAspect enableMCP{this};
    Utils::StringAspect mcpServers{this}; // JSON string containing server configurations

    ButtonAspect addMCPServer{this};
    ButtonAspect removeMCPServer{this};
    ButtonAspect testMCPServer{this};

    // Helper methods
    QList<QJsonObject> getServerConfigs() const;
    void setServerConfigs(const QList<QJsonObject> &configs);
    void addServerConfig(const QJsonObject &config);
    void removeServerConfig(const QString &serverName);

private:
    void setupConnections();
};

MCPSettings &mcpSettings();

} // namespace QodeAssist::Settings