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
#include <QFuture>
#include <QJsonObject>
#include <QObject>

namespace QodeAssist::Tools {

// Struct to represent a ctags tag
struct Tag
{
    QString name;
    QString kind;
    QString scope;
    QString signature;
    int line;
    int endLine;
    QString pattern;

    bool matchesQuery(const QString &query) const
    {
        return name.contains(query, Qt::CaseInsensitive)
               || kind.contains(query, Qt::CaseInsensitive)
               || (!scope.isEmpty() && scope.contains(query, Qt::CaseInsensitive));
    }
};

class FindAndReadFileTool : public LLMCore::BaseTool
{
    Q_OBJECT

public:
    explicit FindAndReadFileTool(QObject *parent = nullptr);

    QString name() const override;
    QString stringName() const override;
    QString description() const override;
    QJsonObject getDefinition(LLMCore::ToolSchemaFormat format) const override;
    LLMCore::ToolPermissions requiredPermissions() const override;
    QFuture<QString> executeAsync(const QJsonObject &input) override;

private:
    QString formatResult(const FileSearchUtils::FileMatch &match, bool readContent) const;

    QString runCtags(const QString &filePath) const;
    QList<Tag> parseCtagsJson(const QString &output) const;
    QList<Tag> findMatchingTags(const QList<Tag> &tags, const QString &query) const;
    QString readLines(const QString &filePath, const QList<Tag> &tags) const;

    Context::IgnoreManager *m_ignoreManager;
};

} // namespace QodeAssist::Tools
