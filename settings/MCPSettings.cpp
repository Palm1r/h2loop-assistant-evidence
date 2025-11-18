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
#include <mcp_sse_client.h>
#include <mcp_stdio_client.h>
#include <utils/layoutbuilder.h>
#include <QDialog>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "SettingsConstants.hpp"

namespace QodeAssist::Settings {

// Dialog for adding MCP server URL
class AddMCPUrlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddMCPUrlDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Add MCP Server");
        setModal(true);
        setMinimumSize(400, 200);
        setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

        auto *layout = new QVBoxLayout(this);

        auto *formLayout = new QFormLayout();
        m_urlEdit = new QLineEdit();
        m_urlEdit->setPlaceholderText("Enter MCP server URL");
        formLayout->addRow("URL:", m_urlEdit);
        layout->addLayout(formLayout);

        m_progressBar = new QProgressBar();
        m_progressBar->setVisible(false);
        layout->addWidget(m_progressBar);

        m_statusLabel = new QLabel();
        m_statusLabel->setVisible(false);
        layout->addWidget(m_statusLabel);

        auto *buttonLayout = new QHBoxLayout();
        m_addButton = new QPushButton("Add");
        m_cancelButton = new QPushButton("Cancel");
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_cancelButton);
        buttonLayout->addWidget(m_addButton);
        layout->addLayout(buttonLayout);

        connect(m_addButton, &QPushButton::clicked, this, &AddMCPUrlDialog::onAddClicked);
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_urlEdit, &QLineEdit::textChanged, this, &AddMCPUrlDialog::onUrlChanged);

        onUrlChanged();
    }

    QString getUrl() const { return m_urlEdit->text().trimmed(); }

private slots:
    void onUrlChanged() { m_addButton->setEnabled(!m_urlEdit->text().trimmed().isEmpty()); }

    void onAddClicked()
    {
        QString url = getUrl();
        if (url.isEmpty()) {
            return;
        }

        // Start validation
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0); // Indeterminate
        m_statusLabel->setText("Validating connection...");
        m_statusLabel->setVisible(true);
        m_addButton->setEnabled(false);
        m_cancelButton->setEnabled(false);
        m_urlEdit->setEnabled(false);

        // Run validation in background
        m_validationWatcher = new QFutureWatcher<bool>(this);
        connect(m_validationWatcher, &QFutureWatcher<bool>::finished, this, [this, url]() {
            bool success = m_validationWatcher->result();
            m_validationWatcher->deleteLater();
            m_validationWatcher = nullptr;

            if (success) {
                accept();
            } else {
                m_progressBar->setVisible(false);
                m_statusLabel->setText("Failed to connect to MCP server");
                m_addButton->setEnabled(true);
                m_cancelButton->setEnabled(true);
                m_urlEdit->setEnabled(true);
            }
        });

        auto future = QtConcurrent::run([url]() -> bool {
            try {
                QString normalizedUrl = url;
                if (normalizedUrl.endsWith('/')) {
                    normalizedUrl.chop(1);
                }
                auto sseClient = std::make_unique<mcp::sse_client>(normalizedUrl.toStdString());

                // Set client capabilities
                nlohmann::json clientCapabilities
                    = {{"experimental", nlohmann::json::object()},
                       {"sampling", nlohmann::json::object()}};
                sseClient->set_capabilities(clientCapabilities);

                return sseClient->initialize("H2LoopAssistant", "1.0.0");
            } catch (const std::exception &e) {
                return false;
            }
        });

        m_validationWatcher->setFuture(future);
    }

private:
    QLineEdit *m_urlEdit;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QPushButton *m_addButton;
    QPushButton *m_cancelButton;
    QFutureWatcher<bool> *m_validationWatcher = nullptr;
};

// Custom widget for MCP servers list
class MCPServersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MCPServersWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_listWidget = new QListWidget();
        m_listWidget->setMinimumHeight(150);
        layout->addWidget(m_listWidget);

        auto *buttonLayout = new QHBoxLayout();
        m_addButton = new QPushButton("Add Server");
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addStretch();
        layout->addLayout(buttonLayout);

        connect(m_addButton, &QPushButton::clicked, this, &MCPServersWidget::onAddClicked);

        updateList();
    }

    void setUrls(const QList<QString> &urls)
    {
        m_urls.clear();
        for (const QString &url : urls) {
            if (!url.isEmpty()) {
                m_urls.append(url);
            }
        }
        updateList();
    }

    QList<QString> getUrls() const { return m_urls; }

signals:
    void urlsChanged();

private slots:
    void onAddClicked()
    {
        AddMCPUrlDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString url = dialog.getUrl();
            if (!m_urls.contains(url)) {
                m_urls.append(url);
                updateList();
                emit urlsChanged();
            }
        }
    }

    void onRemoveClicked()
    {
        QPushButton *button = qobject_cast<QPushButton *>(sender());
        if (!button)
            return;

        QString url = button->property("url").toString();
        m_urls.removeAll(url);
        updateList();
        emit urlsChanged();
    }

private:
    void updateList()
    {
        m_listWidget->clear();
        for (const QString &url : m_urls) {
            auto *item = new QListWidgetItem();
            auto *widget = new QWidget();
            auto *layout = new QHBoxLayout(widget);
            layout->setContentsMargins(5, 5, 5, 5);

            auto *label = new QLabel(url);
            layout->addWidget(label);

            layout->addStretch();

            auto *removeButton = new QPushButton("Remove");
            removeButton->setProperty("url", url);
            connect(removeButton, &QPushButton::clicked, this, &MCPServersWidget::onRemoveClicked);
            layout->addWidget(removeButton);

            item->setSizeHint(widget->sizeHint());
            m_listWidget->addItem(item);
            m_listWidget->setItemWidget(item, widget);
        }
    }

    QListWidget *m_listWidget;
    QPushButton *m_addButton;
    QList<QString> m_urls;
};

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

        // Create custom servers widget
        auto *serversWidget = new MCPServersWidget();
        serversWidget->setUrls(getServerUrls());

        // Connect to update settings when URLs change
        connect(serversWidget, &MCPServersWidget::urlsChanged, this, [this, serversWidget]() {
            setServerUrls(serversWidget->getUrls());
            writeSettings();
            emit serverUrlsChanged();
        });

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

        // Connect to MCP manager signals to refresh tools when they change
        auto mcpManager = LLMCore::ProvidersManager::instance().mcpClientManager();
        if (mcpManager) {
            connect(
                mcpManager,
                &MCP::MCPClientManager::toolsUpdated,
                mcpToolsWidget,
                [this, mcpToolsWidget]() { populateToolsWidget(mcpToolsWidget); });
            connect(
                mcpManager,
                &MCP::MCPClientManager::serverConnected,
                mcpToolsWidget,
                [this, mcpToolsWidget]() { populateToolsWidget(mcpToolsWidget); });
            connect(
                mcpManager,
                &MCP::MCPClientManager::serverDisconnected,
                mcpToolsWidget,
                [this, mcpToolsWidget]() { populateToolsWidget(mcpToolsWidget); });
        }

        return Column{
            enableMCP,
            Space{8},
            Group{title("MCP Servers"), Column{Layouting::Widget(serversWidget)}},
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
        LOG_MESSAGE(
            QString("Exception in populateToolsWidget: %1").arg(QString::fromUtf8(e.what())));
        auto *item = new QTreeWidgetItem(treeWidget);
        item->setText(0, "Error loading MCP tools");
        treeWidget->addTopLevelItem(item);
    } catch (...) {
        LOG_MESSAGE("Unknown exception in populateToolsWidget");
        auto *item = new QTreeWidgetItem(treeWidget);
        item->setText(0, "Error loading MCP tools");
        treeWidget->addTopLevelItem(item);
    }
}

QList<QString> MCPSettings::getServerUrls() const
{
    return mcpServerUrls.value();
}

void MCPSettings::setServerUrls(const QList<QString> &urls)
{
    QList<QString> filteredUrls;
    for (const QString &url : urls) {
        if (!url.isEmpty()) {
            filteredUrls.append(url);
        }
    }
    mcpServerUrls.setValue(filteredUrls);
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

#include "MCPSettings.moc"