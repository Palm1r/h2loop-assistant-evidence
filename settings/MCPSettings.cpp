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
#include <QInputDialog>
#include <QMessageBox>

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

    addMCPServer.setSettingsKey("MCP/AddServer");
    addMCPServer.setLabelText("Add Server");
    addMCPServer.setToolTip("Add a new MCP server URL");

    removeMCPServer.setSettingsKey("MCP/RemoveServer");
    removeMCPServer.setLabelText("Remove Server");
    removeMCPServer.setToolTip("Remove selected MCP server URL");

    testMCPServer.setSettingsKey("MCP/TestServer");
    testMCPServer.setLabelText("Test Server");
    testMCPServer.setToolTip("Test connection to MCP server");

    readSettings();

    setupConnections();

    setLayouter([this]() {
        using namespace Layouting;

        return Column{
            enableMCP,
            Space{8},
            Group{
                title("MCP Servers"),
                Column{mcpServerUrls, Row{addMCPServer, removeMCPServer, testMCPServer, Stretch{1}}}}};
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

void MCPSettings::setupConnections()
{
    connect(&addMCPServer, &ButtonAspect::clicked, this, [this]() {
        bool ok;
        QString url = QInputDialog::getText(
            nullptr,
            tr("Add MCP Server"),
            tr("Enter MCP server URL:"),
            QLineEdit::Normal,
            QString(),
            &ok);

        if (ok && !url.isEmpty()) {
            addServerUrl(url);
        }
    });

    connect(&removeMCPServer, &ButtonAspect::clicked, this, [this]() {
        QList<QString> urls = getServerUrls();
        if (urls.isEmpty()) {
            QMessageBox::information(nullptr, tr("No Servers"), tr("No MCP servers configured."));
            return;
        }

        bool ok;
        QString selectedUrl = QInputDialog::getItem(
            nullptr,
            tr("Remove MCP Server"),
            tr("Select server URL to remove:"),
            urls,
            0,
            false,
            &ok);

        if (ok && !selectedUrl.isEmpty()) {
            removeServerUrl(selectedUrl);
        }
    });

    connect(&testMCPServer, &ButtonAspect::clicked, this, [this]() {
        QList<QString> urls = getServerUrls();
        if (urls.isEmpty()) {
            QMessageBox::information(nullptr, tr("No Servers"), tr("No MCP servers configured."));
            return;
        }

        bool ok;
        QString selectedUrl = QInputDialog::getItem(
            nullptr, tr("Test MCP Server"), tr("Select server URL to test:"), urls, 0, false, &ok);

        if (ok && !selectedUrl.isEmpty()) {
            QMessageBox::information(
                nullptr,
                tr("Test Server"),
                tr("Testing server: %1\n\nThis feature is not yet implemented.").arg(selectedUrl));
        }
    });
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