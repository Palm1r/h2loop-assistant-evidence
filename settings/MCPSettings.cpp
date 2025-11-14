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
#include "logger/Logger.hpp"
#include <coreplugin/dialogs/ioptionspage.h>
#include <llmcore/ProvidersManager.hpp>
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

    // Connect to the aspect container's changed signal to detect when settings are applied
    connect(this, &Utils::AspectContainer::changed, this, [this]() { emit serverUrlsChanged(); });

    setLayouter([this]() {
        using namespace Layouting;

        // Create fresh tree widget each time the layout is built
        auto *mcpToolsWidget = new QTreeWidget();
        QStringList headers;
        headers << "Available MCP Tools";
        mcpToolsWidget->setHeaderLabels(headers);
        mcpToolsWidget->setRootIsDecorated(true);
        mcpToolsWidget->setSelectionMode(QAbstractItemView::NoSelection);
        mcpToolsWidget->setMinimumHeight(200);
        mcpToolsWidget->setMaximumHeight(400);

        // Populate tools synchronously
        populateToolsWidget(mcpToolsWidget);

        return Column{
            enableMCP,
            Space{8},
            Group{title("MCP Servers"), Column{mcpServerUrls}},
            Space{8},
            Group{title("Available MCP Tools"), Column{Layouting::Widget(mcpToolsWidget)}}};
    });
}

void MCPSettings::populateToolsWidget(QTreeWidget *treeWidget)
{
    if (!treeWidget)
        return;

    treeWidget->clear();

    auto mcpManager = LLMCore::ProvidersManager::instance().mcpClientManager();
    if (!mcpManager) {
        // No MCP manager available, show empty message
        auto *item = new QTreeWidgetItem(treeWidget);
        item->setText(0, "No MCP servers configured");
        treeWidget->addTopLevelItem(item);
        return;
    }

    try {
        auto tools = mcpManager->getAvailableTools();
        if (tools.isEmpty()) {
            auto *item = new QTreeWidgetItem(treeWidget);
            item->setText(0, "No MCP tools available");
            treeWidget->addTopLevelItem(item);
            return;
        }

        for (const auto &tool : tools) {
            if (tool.name.isEmpty())
                continue; // Skip invalid tools

            auto *item = new QTreeWidgetItem(treeWidget);
            item->setText(0, tool.name);

            if (!tool.description.isEmpty()) {
                // Add description as child item
                auto *descItem = new QTreeWidgetItem(item);
                descItem->setText(0, tool.description);
            }
        }

        // Collapse all items by default so only tool names are visible
        treeWidget->collapseAll();

    } catch (const std::exception &e) {
        // Handle any exceptions during tool retrieval
        LOG_MESSAGE(QString("Exception in populateToolsWidget: %1").arg(e.what()));
        auto *item = new QTreeWidgetItem(treeWidget);
        item->setText(0, "Error loading MCP tools");
        treeWidget->addTopLevelItem(item);
    } catch (...) {
        // Handle any other exceptions
        LOG_MESSAGE("Unknown exception in populateToolsWidget");
        auto *item = new QTreeWidgetItem(treeWidget);
        item->setText(0, "Error loading MCP tools");
        treeWidget->addTopLevelItem(item);
    }
}

void MCPSettings::updateToolsList()
{
    // This method is kept for backward compatibility but is no longer used
    // since widgets are created fresh each time
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