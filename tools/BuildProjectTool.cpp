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

#include "BuildProjectTool.hpp"

#include "GetIssuesListTool.hpp"
#include <logger/Logger.hpp>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>
#include <QPromise>
#include <QTimer>

namespace QodeAssist::Tools {

BuildProjectTool::BuildProjectTool(QObject *parent)
    : BaseTool(parent)
{
    // Ensure IssuesTracker is initialized
    IssuesTracker::instance();
}

QString BuildProjectTool::name() const
{
    return "build_project";
}

QString BuildProjectTool::stringName() const
{
    return "Building project";
}

QString BuildProjectTool::description() const
{
    return "Build the current project in Qt Creator. "
           "Waits for build completion and returns build status along with any compilation "
           "errors/warnings. "
           "Optional 'rebuild' parameter: set to true to force a clean rebuild (default: false).";
}

QJsonObject BuildProjectTool::getDefinition(LLMCore::ToolSchemaFormat format) const
{
    QJsonObject definition;
    definition["type"] = "object";

    QJsonObject properties;
    properties["rebuild"] = QJsonObject{
        {"type", "boolean"},
        {"description", "Force a clean rebuild instead of incremental build (default: false)"}};

    definition["properties"] = properties;
    definition["required"] = QJsonArray();

    switch (format) {
    case LLMCore::ToolSchemaFormat::OpenAI:
        return customizeForOpenAI(definition);
    case LLMCore::ToolSchemaFormat::Claude:
        return customizeForClaude(definition);
    case LLMCore::ToolSchemaFormat::Ollama:
        return customizeForOllama(definition);
    case LLMCore::ToolSchemaFormat::Google:
        return customizeForGoogle(definition);
    }

    return definition;
}

LLMCore::ToolPermissions BuildProjectTool::requiredPermissions() const
{
    return LLMCore::ToolPermission::None;
}

QFuture<QString> BuildProjectTool::executeAsync(const QJsonObject &input)
{
    auto *promise = new QPromise<QString>();
    auto future = promise->future();

    auto *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project) {
        LOG_MESSAGE("BuildProjectTool: No active project found");
        promise->addResult(
            QString("Error: No active project found. Please open a project in Qt Creator."));
        promise->finish();
        delete promise;
        return future;
    }

    LOG_MESSAGE(QString("BuildProjectTool: Active project is '%1'").arg(project->displayName()));

    if (ProjectExplorer::BuildManager::isBuilding(project)) {
        LOG_MESSAGE("BuildProjectTool: Build is already in progress");
        promise->addResult(
            QString("Error: Build is already in progress. Please wait for it to complete."));
        promise->finish();
        delete promise;
        return future;
    }

    bool rebuild = input.value("rebuild").toBool(false);

    QString buildType = rebuild ? QString("rebuild") : QString("build");

    LOG_MESSAGE(QString("BuildProjectTool: Starting %1").arg(buildType));

    // Start the build
    QMetaObject::invokeMethod(
        qApp,
        [project, rebuild]() {
            if (rebuild) {
                ProjectExplorer::BuildManager::rebuildProjectWithDependencies(
                    project, ProjectExplorer::ConfigSelection::Active);
            } else {
                ProjectExplorer::BuildManager::buildProjectWithDependencies(
                    project, ProjectExplorer::ConfigSelection::Active);
            }
        },
        Qt::QueuedConnection);

    // Create a timer to check build completion
    auto *timer = new QTimer(this);
    timer->setSingleShot(false);
    timer->setInterval(500); // Check every 500ms

    // Elapsed timer for timeout
    QElapsedTimer elapsed;
    elapsed.start();

    connect(
        timer, &QTimer::timeout, this, [this, project, buildType, timer, elapsed, promise]() mutable {
            if (!ProjectExplorer::BuildManager::isBuilding(project)
                || elapsed.elapsed() > 10 * 60 * 1000) {
                // Build completed or timed out
                timer->stop();

                bool buildCompleted = !ProjectExplorer::BuildManager::isBuilding(project);
                bool timedOut = elapsed.elapsed() > 10 * 60 * 1000;

                QString result;
                if (timedOut) {
                    result = QString("Build %1 for project '%2' timed out after 10 minutes.")
                                 .arg(buildType)
                                 .arg(project->displayName());
                } else if (buildCompleted) {
                    result = QString("Build %1 completed for project '%2'.\n\n")
                                 .arg(buildType)
                                 .arg(project->displayName());
                } else {
                    result = QString("Build %1 failed to complete for project '%2'.")
                                 .arg(buildType)
                                 .arg(project->displayName());
                }

                // Check for issues after build
                const auto tasks = IssuesTracker::instance().getTasks();
                if (!tasks.isEmpty()) {
                    int errorCount = 0;
                    int warningCount = 0;

                    for (const ProjectExplorer::Task &task : tasks) {
                        auto taskType = task.type();
                        if (taskType == ProjectExplorer::Task::Error) {
                            errorCount++;
                        } else if (taskType == ProjectExplorer::Task::Warning) {
                            warningCount++;
                        }
                    }

                    result += QString("Issues found: %1 errors, %2 warnings\n\n")
                                  .arg(errorCount)
                                  .arg(warningCount);

                    if (errorCount > 0 || warningCount > 0) {
                        result += "Issue details:\n";
                        for (const ProjectExplorer::Task &task : tasks) {
                            auto taskType = task.type();
                            auto taskFile = task.file();
                            auto taskLine = task.line();
                            QString typeStr = (taskType == ProjectExplorer::Task::Error)
                                                  ? QString("ERROR")
                                                  : QString("WARNING");
                            QString issueText = QString("[%1] %2").arg(typeStr, task.description());

                            if (!taskFile.isEmpty()) {
                                issueText += QString("\n  File: %1").arg(taskFile.toUrlishString());
                                if (taskLine > 0) {
                                    issueText += QString(":%1").arg(taskLine);
                                }
                            }

                            result += issueText + "\n\n";
                        }
                    }
                } else {
                    result += "No compilation issues found.";
                }

                LOG_MESSAGE(QString("BuildProjectTool: %1 completed - %2")
                                .arg(buildType)
                                .arg(buildCompleted ? "success" : "failed"));

                promise->addResult(result);
                promise->finish();
                delete promise;

                timer->deleteLater();
            }
        });

    timer->start();

    return future;
}

} // namespace QodeAssist::Tools
