/*****************************************************************************
 * Copyright (C) 2008-2009 Csaba Karai <cskarai@freemail.hu>                 *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This package is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this package; if not, write to the Free Software               *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA *
 *****************************************************************************/

#include "queuedialog.h"

#include <QtCore/QEvent>
#include <QtCore/QRect>
#include <QtGui/QLayout>
#include <QtGui/QFrame>
#include <QtGui/QPainter>
#include <QtGui/QStyleOption>
#include <QtGui/QLabel>
#include <QtGui/QFont>
#include <QtGui/QToolButton>
#include <QtGui/QImage>
#include <QtGui/QTimeEdit>
#include <QtGui/QProgressBar>
#include <QtGui/QStackedWidget>
#include <QMouseEvent>
#include <QKeyEvent>

#include <KLocale>
#include <KGlobalSettings>
#include <KIconEffect>
#include <KInputDialog>
#include <KMessageBox>

#include "queuewidget.h"
#include "queue_mgr.h"
#include "../krglobal.h"

class KrTimeDialog : public KDialog
{
public:
    KrTimeDialog(QWidget * parent = 0) : KDialog(parent) {
        setWindowTitle(i18n("Krusader::Queue Manager"));
        setButtons(KDialog::Ok | KDialog::Cancel);
        setDefaultButton(KDialog::Ok);
        setWindowModality(Qt::WindowModal);

        QWidget *mainWidget = new QWidget(this);
        QGridLayout *grid_time = new QGridLayout;
        QLabel * label = new QLabel(i18n("Please enter the time to start processing the queue:"), mainWidget);
        grid_time->addWidget(label, 0, 0);

        QTime time = QTime::currentTime();
        time = time.addSecs(60 * 60);   // add 1 hour
        _timeEdit = new QTimeEdit(time, mainWidget);
        _timeEdit->setDisplayFormat("hh:mm:ss");
        grid_time->addWidget(_timeEdit, 1, 0);
        mainWidget->setLayout(grid_time);
        setMainWidget(mainWidget);

        _timeEdit->setFocus();
    }
    QTime getStartTime() {
        return _timeEdit->time();
    }

private:
    QTimeEdit * _timeEdit;
};

QueueDialog * QueueDialog::_queueDialog = 0;

QueueDialog::QueueDialog() : KDialog(), _autoHide(true)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(i18n("Krusader::Queue Manager"));
    setButtons(0);

    QVBoxLayout *vert_main = new QVBoxLayout;
    vert_main->setContentsMargins(0, 0, 0, 0);

    QWidget *toolWg = new QWidget(mainWidget());
    QHBoxLayout * hbox2 = new QHBoxLayout(toolWg);
    hbox2->setContentsMargins(0, 0, 0, 0);
    hbox2->setSpacing(0);

    _newTabButton = new QToolButton(mainWidget());
    _newTabButton->setIcon(KIcon("tab-new"));
    _newTabButton->setToolTip(i18n("Create a new queue (Ctrl+T)"));
    connect(_newTabButton, SIGNAL(clicked()), this, SLOT(slotNewTab()));
    hbox2->addWidget(_newTabButton);

    _closeTabButton = new QToolButton(mainWidget());
    _closeTabButton->setIcon(KIcon("tab-close"));
    _closeTabButton->setToolTip(i18n("Remove the current queue (Ctrl+W)"));
    connect(_closeTabButton, SIGNAL(clicked()), this, SLOT(slotDeleteCurrentTab()));
    hbox2->addWidget(_closeTabButton);

    _pauseButton = new QToolButton(mainWidget());
    _pauseButton->setIcon(KIcon("media-playback-pause"));
    connect(_pauseButton, SIGNAL(clicked()), this, SLOT(slotPauseClicked()));
    hbox2->addWidget(_pauseButton);

    _progressBar = new QProgressBar(mainWidget());
    _progressBar->setTextVisible(true);
    _progressBarPlaceholder = new QWidget(this);
    _progressBarStackedWidget = new QStackedWidget();
    _progressBarStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _progressBarStackedWidget->addWidget(_progressBar);
    _progressBarStackedWidget->addWidget(_progressBarPlaceholder);
    _progressBarStackedWidget->setCurrentWidget(_progressBarStackedWidget);
    hbox2->addWidget(_progressBarStackedWidget);

    _scheduleButton = new QToolButton(mainWidget());
    _scheduleButton->setIcon(KIcon("chronometer"));
    _scheduleButton->setToolTip(i18n("Schedule queue starting (Ctrl+S)"));
    connect(_scheduleButton, SIGNAL(clicked()), this, SLOT(slotScheduleClicked()));
    hbox2->addWidget(_scheduleButton);

    vert_main->addWidget(toolWg);

    _queueWidget = new QueueWidget(mainWidget());
    connect(_queueWidget, SIGNAL(currentChanged()), this, SLOT(slotUpdateToolbar()));
    vert_main->addWidget(_queueWidget);

    _statusLabel = new QLabel(mainWidget());
    QSizePolicy statuspolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    statuspolicy.setHeightForWidth(_statusLabel->sizePolicy().hasHeightForWidth());
    _statusLabel->setSizePolicy(statuspolicy);
    _statusLabel->setFrameShape(QLabel::StyledPanel);
    _statusLabel->setFrameShadow(QLabel::Sunken);
    vert_main->addWidget(_statusLabel);

    mainWidget()->setLayout(vert_main);

    KConfigGroup group(krConfig, "QueueManager");
    int sizeX = group.readEntry("Window Width",  -1);
    int sizeY = group.readEntry("Window Height",  -1);
    int x = group.readEntry("Window X",  -1);
    int y = group.readEntry("Window Y",  -1);

    if (sizeX != -1 && sizeY != -1)
        resize(sizeX, sizeY);
    if (x != -1 && y != -1)
        move(x, y);

    slotUpdateToolbar();

    connect(QueueManager::instance(), SIGNAL(queueInserted(Queue *)), this, SLOT(slotUpdateToolbar()));
    connect(QueueManager::instance(), SIGNAL(percent(Queue *, int)), this, SLOT(slotPercentChanged(Queue *, int)));

    show();
    _queueWidget->currentWidget()->setFocus();

    _queueDialog = this;
}

QueueDialog::~QueueDialog()
{
    if (_queueDialog)
        saveSettings();
    _queueDialog = 0;
}

void QueueDialog::showDialog(bool autoHide)
{
    if (_queueDialog == 0)
        _queueDialog = new QueueDialog();
    else {
        _queueDialog->show();
        if (!autoHide) {
            _queueDialog->raise();
            _queueDialog->activateWindow();
        }
    }
    if (!autoHide)
        _queueDialog->_autoHide = false;
}

void QueueDialog::everyQueueIsEmpty()
{
    if (_queueDialog && _queueDialog->_autoHide && !_queueDialog->isHidden())
        _queueDialog->close();
}

void QueueDialog::saveSettings()
{
    KConfigGroup group(krConfig, "QueueManager");
    group.writeEntry("Window Width", width());
    group.writeEntry("Window Height", height());
    group.writeEntry("Window X", x());
    group.writeEntry("Window Y", y());
}

void QueueDialog::deleteDialog()
{
    if (_queueDialog)
        delete _queueDialog;
}

void QueueDialog::slotUpdateToolbar()
{
    Queue * currentQueue = QueueManager::currentQueue();
    if (currentQueue) {
        if (currentQueue->isSuspended()) {
            _pauseButton->setIcon(KIcon("media-playback-start"));
            _pauseButton->setToolTip(i18n("Start processing the queue (Ctrl+P)"));
            QTime time = currentQueue->scheduleTime();
            if (time.isNull()) {
                _statusLabel->setText(i18n("The queue is paused."));
            } else {
                _statusLabel->setText(i18n("Scheduled to start at %1.",
                                           time.toString("hh:mm:ss")));
            }
        } else {
            _statusLabel->setText(i18n("The queue is running."));
            _pauseButton->setIcon(KIcon("media-playback-pause"));
            _pauseButton->setToolTip(i18n("Pause processing the queue (Ctrl+P)"));
        }

        slotPercentChanged(currentQueue, currentQueue->getPercent());
    }
}

void QueueDialog::slotPauseClicked()
{
    Queue * currentQueue = QueueManager::currentQueue();
    if (currentQueue) {
        if (currentQueue->isSuspended())
            currentQueue->resume();
        else
            currentQueue->suspend();
    }
}

void QueueDialog::slotScheduleClicked()
{
    QPointer<KrTimeDialog> dialog = new KrTimeDialog(this);
    if (dialog->exec() == QDialog::Accepted) {
        QTime startTime = dialog->getStartTime();
        Queue * queue = QueueManager::currentQueue();
        queue->schedule(startTime);
    }
    delete dialog;
}

void QueueDialog::slotNewTab()
{
    bool ok = false;
    QString queueName = KInputDialog::getText(
                            i18n("Krusader::Queue Manager"),  // Caption
                            i18n("Please enter the name of the new queue"), // Questiontext
                            QString(), // Default
                            &ok, this);

    if (!ok || queueName.isEmpty())
        return;

    Queue * queue = QueueManager::createQueue(queueName);
    if (queue == 0) {
        KMessageBox::error(this, i18n("A queue already exists with this name."));
    }
}

void QueueDialog::slotDeleteCurrentTab()
{
    Queue * currentQueue = QueueManager::currentQueue();
    if (currentQueue) {
        if (currentQueue->count() != 0) {
            if (KMessageBox::warningContinueCancel(this,
                                                   i18n("Deleting the queue requires aborting all pending jobs. Do you wish to continue?"),
                                                   i18n("Warning"), KGuiItem(i18n("&Delete"))) != KMessageBox::Continue) return;
        }
        QueueManager::removeQueue(currentQueue);
    }
}

void QueueDialog::slotPercentChanged(Queue * queue, int percent)
{
    if (QueueManager::currentQueue() == queue) {
        switch (percent) {
        case Queue::PERCENT_UNUSED:
            _progressBarStackedWidget->setCurrentWidget(_progressBarPlaceholder);
            break;
        case Queue::PERCENT_UNKNOWN:
            _progressBarStackedWidget->setCurrentWidget(_progressBar);
            _progressBar->setRange(0, 0);
            break;
        default:
            _progressBarStackedWidget->setCurrentWidget(_progressBar);
            _progressBar->setRange(0, 100);
            _progressBar->setValue(percent);
            break;
        }
    }
}

void QueueDialog::keyPressEvent(QKeyEvent *ke)
{
    switch (ke->key()) {
    case Qt::Key_T : {
        if (ke->modifiers() == Qt::ControlModifier) {
            slotNewTab();
            return;
        }
    }
    break;
    case Qt::Key_W : {
        if (ke->modifiers() == Qt::ControlModifier) {
            slotDeleteCurrentTab();
            return;
        }
    }
    break;
    case Qt::Key_S : {
        if (ke->modifiers() == Qt::ControlModifier) {
            slotScheduleClicked();
            return;
        }
    }
    break;
    case Qt::Key_Right : {
        if (ke->modifiers() == Qt::ShiftModifier) {
            int curr = _queueWidget->currentIndex();
            curr = (curr + 1) % _queueWidget->count();
            _queueWidget->setCurrentIndex(curr);
            return;
        }
    }
    break;
    case Qt::Key_Left : {
        if (ke->modifiers() == Qt::ShiftModifier) {
            int curr = _queueWidget->currentIndex();
            curr = (curr + _queueWidget->count() - 1) % _queueWidget->count();
            _queueWidget->setCurrentIndex(curr);
            return;
        }
    }
    break;
    case Qt::Key_P : {
        if (ke->modifiers() == Qt::ControlModifier) {
            slotPauseClicked();
            return;
        }
    }
    break;
    case Qt::Key_Delete : {
        if (ke->modifiers() == 0) {
            _queueWidget->deleteCurrent();
            return;
        }
    }
    break;
    // eat these events - otherwise QDialog::keyPressEvent() triggers the close button
    case Qt::Key_Enter:
    case Qt::Key_Return:
        return;
    }
    KDialog::keyPressEvent(ke);
}

void QueueDialog::closeEvent(QCloseEvent *e)
{
    _autoHide = true;
    saveSettings();
    KDialog::closeEvent(e);
}
