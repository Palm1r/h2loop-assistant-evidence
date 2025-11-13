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
#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/layoutbuilder.h>

#include "SettingsConstants.hpp"

namespace QodeAssist::Settings {

MCPSettings::MCPSettings()
{
    setAutoApply(false);

    setDisplayName("MCP");

    enableMCP.setSettingsKey("MCP/EnableMCP");
    enableMCP.setLabelText("Enable MCP Integration");
    enableMCP.setToolTip("Enable Model Context Protocol server integration");
    enableMCP.setDefaultValue(false);

    mcpServerUrls.setSettingsKey("MCP/ServerUrls");
    mcpServerUrls.setLabelText("MCP Server URLs");
    mcpServerUrls.setToolTip("List of MCP server URLs to connect to");

    readSettings();

    setLayouter([this]() {
        using namespace Layouting;

        return Column{enableMCP, Space{8}, Group{title("MCP Servers"), Column{mcpServerUrls}}};
    });
}

QList<QString> MCPSettings::getServerUrls() const
{
    return mcpServerUrls.value();
}

void MCPSettings::setServerUrls(const QList<QString> &urls)
{
    mcpServerUrls.setValue(urls);
}

void MCPSettings::addServerUrl(const QString &url)
{
    QList<QString> urls = getServerUrls();
    if (!urls.contains(url)) {
        urls.append(url);
        setServerUrls(urls);
    }
}

void MCPSettings::removeServerUrl(const QString &url)
{
    QList<QString> urls = getServerUrls();
    urls.removeAll(url);
    setServerUrls(urls);
}

MCPSettings &mcpSettings()
{
    static MCPSettings settings;
    return settings;
}

class MCPSettingsPage : public Core::IOptionsPage
{
public:
    MCPSettingsPage()
    {
        setId(Constants::QODE_ASSIST_MCP_SETTINGS_PAGE_ID);
        setDisplayName("MCP");
        setCategory(Constants::QODE_ASSIST_GENERAL_OPTIONS_CATEGORY);
        setSettingsProvider([] { return &mcpSettings(); });
    }
};

const MCPSettingsPage mcpSettingsPage;

} // namespace QodeAssist::Settings