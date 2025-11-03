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

#include "InlineRefactorWidget.hpp"
#include "QodeAssisttr.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScreen>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

namespace QodeAssist {

InlineRefactorWidget::InlineRefactorWidget(QWidget *parent, const QString &lastInstructions)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
    , m_lastInstructions(lastInstructions)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setFocusPolicy(Qt::StrongFocus);

    setupUi();

    QTimer::singleShot(0, this, &InlineRefactorWidget::updateWidgetSize);
    m_textEdit->installEventFilter(this);
    updateWidgetSize();
}

void InlineRefactorWidget::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(8);

    m_actionsLayout = new QHBoxLayout();
    m_actionsLayout->setSpacing(4);
    createActionButtons();
    m_actionsLayout->addWidget(m_repeatButton);
    m_actionsLayout->addWidget(m_improveButton);
    m_actionsLayout->addWidget(m_alternativeButton);
    m_actionsLayout->addStretch();
    m_mainLayout->addLayout(m_actionsLayout);

    m_instructionsLabel = new QLabel(Tr::tr("Enter refactoring instructions:"), this);
    m_mainLayout->addWidget(m_instructionsLabel);

    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setMinimumHeight(100);
    m_textEdit->setPlaceholderText(Tr::tr("Type your refactoring instructions here..."));

    connect(m_textEdit, &QPlainTextEdit::textChanged, this, &InlineRefactorWidget::updateWidgetSize);

    m_mainLayout->addWidget(m_textEdit);

    // No button box for inline widget - use Enter/Escape
}

void InlineRefactorWidget::createActionButtons()
{
    Utils::Icon REPEAT_ICON(
        {{":/resources/images/repeat-last-instruct-icon.png", Utils::Theme::IconsBaseColor}});
    Utils::Icon IMPROVE_ICON(
        {{":/resources/images/improve-current-code-icon.png", Utils::Theme::IconsBaseColor}});
    Utils::Icon ALTER_ICON(
        {{":/resources/images/suggest-new-icon.png", Utils::Theme::IconsBaseColor}});
    m_repeatButton = new QToolButton(this);
    m_repeatButton->setIcon(REPEAT_ICON.icon());
    m_repeatButton->setToolTip(Tr::tr("Repeat Last Instructions"));
    m_repeatButton->setEnabled(!m_lastInstructions.isEmpty());
    connect(m_repeatButton, &QToolButton::clicked, this, &InlineRefactorWidget::useLastInstructions);

    m_improveButton = new QToolButton(this);
    m_improveButton->setIcon(IMPROVE_ICON.icon());
    m_improveButton->setToolTip(Tr::tr("Improve Current Code"));
    connect(
        m_improveButton, &QToolButton::clicked, this, &InlineRefactorWidget::useImproveCodeTemplate);

    m_alternativeButton = new QToolButton(this);
    m_alternativeButton->setIcon(ALTER_ICON.icon());
    m_alternativeButton->setToolTip(Tr::tr("Suggest Alternative Solution"));
    connect(
        m_alternativeButton,
        &QToolButton::clicked,
        this,
        &InlineRefactorWidget::useAlternativeSolutionTemplate);
}

QString InlineRefactorWidget::instructions() const
{
    return m_textEdit->toPlainText();
}

void InlineRefactorWidget::setInstructions(const QString &instructions)
{
    m_textEdit->setPlainText(instructions);
}

InlineRefactorWidget::Action InlineRefactorWidget::selectedAction() const
{
    return m_selectedAction;
}

void InlineRefactorWidget::showAt(const QPoint &globalPos)
{
    m_showPosition = globalPos;
    adjustPosition();
    show();
    raise();
    activateWindow();
    m_textEdit->setFocus();
}

void InlineRefactorWidget::hideWidget()
{
    hide();
    emit rejected();
}

void InlineRefactorWidget::adjustPosition()
{
    QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
    QSize widgetSize = sizeHint();

    QPoint pos = m_showPosition;

    // Position below the cursor if space allows, otherwise above
    if (pos.y() + widgetSize.height() <= screenGeometry.bottom()) {
        // Show below
        pos.setY(pos.y() + 5);
    } else {
        // Show above
        pos.setY(pos.y() - widgetSize.height() - 5);
    }

    // Ensure horizontal fit
    if (pos.x() + widgetSize.width() > screenGeometry.right()) {
        pos.setX(screenGeometry.right() - widgetSize.width() - 10);
    }
    if (pos.x() < screenGeometry.left()) {
        pos.setX(screenGeometry.left() + 10);
    }

    move(pos);
}

bool InlineRefactorWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_textEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                return false;
            }

            onAccepted();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}


void InlineRefactorWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hideWidget();
        return;
    }
    QWidget::keyPressEvent(event);
}

void InlineRefactorWidget::useLastInstructions()
{
    if (!m_lastInstructions.isEmpty()) {
        m_textEdit->setPlainText(m_lastInstructions);
        m_selectedAction = Action::RepeatLast;
        onAccepted();
    }
}

void InlineRefactorWidget::useImproveCodeTemplate()
{
    m_textEdit->setPlainText(Tr::tr(
        "Improve the selected code by enhancing readability, efficiency, and maintainability. "
        "Follow best practices for C++/Qt and fix any potential issues."));
    m_selectedAction = Action::ImproveCode;
    onAccepted();
}

void InlineRefactorWidget::useAlternativeSolutionTemplate()
{
    m_textEdit->setPlainText(
        Tr::tr("Suggest an alternative implementation approach for the selected code. "
               "Provide a different solution that might be cleaner, more efficient, "
               "or uses different Qt/C++ patterns or idioms."));
    m_selectedAction = Action::AlternativeSolution;
    onAccepted();
}

void InlineRefactorWidget::updateWidgetSize()
{
    QString text = m_textEdit->toPlainText();

    QFontMetrics fm(m_textEdit->font());

    QStringList lines = text.split('\n');
    int lineCount = lines.size();

    if (lineCount <= 1) {
        int singleLineHeight = fm.height() + 10;
        m_textEdit->setMinimumHeight(singleLineHeight);
        m_textEdit->setMaximumHeight(singleLineHeight);
    } else {
        m_textEdit->setMaximumHeight(QWIDGETSIZE_MAX);

        int lineHeight = fm.height() + 2;

        int textEditHeight = qMin(qMax(lineCount, 2) * lineHeight, 20 * lineHeight);
        m_textEdit->setMinimumHeight(textEditHeight);
    }

    int maxWidth = 500;
    for (const QString &line : lines) {
        int lineWidth = fm.horizontalAdvance(line) + 30;
        maxWidth = qMax(maxWidth, qMin(lineWidth, 800));
    }

    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    int newWidth = qMin(maxWidth + 40, screenGeometry.width() * 3 / 4);

    int newHeight;
    if (lineCount <= 1) {
        newHeight = 150;
    } else {
        newHeight = m_textEdit->minimumHeight() + 150;
    }
    newHeight = qMin(newHeight, screenGeometry.height() * 3 / 4);

    resize(newWidth, newHeight);
    adjustPosition();
}

void InlineRefactorWidget::onAccepted()
{
    emit accepted();
    hide();
}

void InlineRefactorWidget::onRejected()
{
    emit rejected();
    hide();
}

} // namespace QodeAssist