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

#include "FileSearchUtils.hpp"

#include <context/IgnoreManager.hpp>
#include <llmcore/BaseTool.hpp>

namespace QodeAssist::Tools {

class EditFileTool : public LLMCore::BaseTool
{
    Q_OBJECT
public:
    explicit EditFileTool(QObject *parent = nullptr);

    QString name() const override;
    QString stringName() const override;
    QString description() const override;
    QJsonObject getDefinition(LLMCore::ToolSchemaFormat format) const override;
    LLMCore::ToolPermissions requiredPermissions() const override;

    QFuture<QString> executeAsync(const QJsonObject &input = QJsonObject()) override;

    // Struct to hold parsed SEARCH/REPLACE block
    struct SearchReplaceBlock
    {
        QString searchContent;
        QString replaceContent;
        int startLine = -1;  // Line hint for search start (-1 = not specified)
        int endLine = -1;    // Line hint for search end (-1 = not specified)
    };

    // Parse SEARCH/REPLACE blocks from content string
    // Made public for testing purposes
    static QList<SearchReplaceBlock> parseSearchReplaceBlocks(const QString &content);

private:
    Context::IgnoreManager *m_ignoreManager;
};

} // namespace QodeAssist::Tools
