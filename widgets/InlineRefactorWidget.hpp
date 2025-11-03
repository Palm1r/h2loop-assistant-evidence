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

#include <QWidget>
#include <QString>

class QPlainTextEdit;
class QToolButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;

namespace QodeAssist {

class InlineRefactorWidget : public QWidget
{
    Q_OBJECT

public:
    enum class Action { Custom, RepeatLast, ImproveCode, AlternativeSolution };

    explicit InlineRefactorWidget(QWidget *parent = nullptr, const QString &lastInstructions = QString());
    ~InlineRefactorWidget() override = default;

    QString instructions() const;
    void setInstructions(const QString &instructions);

    Action selectedAction() const;

    void showAt(const QPoint &globalPos);
    void hideWidget();

signals:
    void accepted();
    void rejected();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void useLastInstructions();
    void useImproveCodeTemplate();
    void useAlternativeSolutionTemplate();
    void updateWidgetSize();
    void onAccepted();
    void onRejected();

private:
    void setupUi();
    void createActionButtons();
    void adjustPosition();

    QPlainTextEdit *m_textEdit;
    QToolButton *m_repeatButton;
    QToolButton *m_improveButton;
    QToolButton *m_alternativeButton;
    QLabel *m_instructionsLabel;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_actionsLayout;

    Action m_selectedAction = Action::Custom;
    QString m_lastInstructions;
    QPoint m_showPosition;
};

} // namespace QodeAssist