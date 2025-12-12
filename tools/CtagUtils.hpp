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

#include <QJsonDocument>
#include <QList>
#include <QString>

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

class CtagUtils
{
public:
    static QString runCtags(const QString &filePath);
    static QList<Tag> parseCtagsJson(const QString &output);
    static QString filterCtagsOutput(const QString &output);
    static QString generateCtagforFile(const QString &filePath);
    static QString mergeDocstringsWithCtags(
        const QString &ctagsOutput, const QJsonDocument &docstrings);
};

} // namespace QodeAssist::Tools