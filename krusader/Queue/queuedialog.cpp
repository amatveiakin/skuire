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

class KrImageButton : public QToolButton
{
public:
    KrImageButton(QWidget * parent) : QToolButton(parent), _onIcon(false) {
    }

    virtual QSize sizeHint() const {
        int size = QFontMetrics(font()).height();
        return QSize(size, size);
    }

    virtual void paintEvent(QPaintEvent*) {
        int size = QFontMetrics(font()).height();

        QPixmap pm = icon().pixmap(size, size);

        if (_onIcon) {
            QImage img = pm.toImage();
            KIconEffect::colorize(img, Qt::white, 0.4f);
            pm = QPixmap::fromImage(img);
        }

        QPainter paint(this);
        paint.drawPixmap(QPoint(), pm);
    }

    virtual void enterEvent(QEvent *) {
        _onIcon = true;
        repaint();
    }

    virtual void leaveEvent(QEvent *) {
        _onIcon = false;
        repaint();
    }

private:
    bool _onIcon;
};

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

QueueDialog::QueueDialog() : KDialog(0, Qt::FramelessWindowHint), _autoHide(true)
{
    setWindowModality(Qt::NonModal);
    setWindowTitle(i18n("Krusader::Queue Manager"));
    setSizeGripEnabled(true);

    QGridLayout *grid_main = new QGridLayout;
    grid_main->setContentsMargins(0, 0, 0, 12);
    grid_main->setSpacing(0);

    QFrame *titleWg = new QFrame(this);
    titleWg->setFrameShape(QFrame::Box);
    titleWg->setFrameShadow(QFrame::Raised);
    titleWg->setLineWidth(1);    // a nice 3D touch :-)
    titleWg->setAutoFillBackground(true);
    titleWg->setContentsMargins(2, 2, 2, 2);

    QPalette palette = titleWg->palette();
    palette.setColor(QPalette::WindowText, KGlobalSettings::activeTextColor());
    palette.setColor(QPalette::Window, KGlobalSettings::activeTitleColor());
    titleWg->setPalette(palette);

    QHBoxLayout * hbox = new QHBoxLayout(titleWg);
    hbox->setSpacing(0);

    QLabel *title = new QLabel(i18n("Queue Manager"), titleWg);
    QSizePolicy titlepolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    title->setSizePolicy(titlepolicy);
    title->setAlignment(Qt::AlignHCenter);
    QFont fnt = title->font();
    fnt.setBold(true);
    title->setFont(fnt);
    hbox->addWidget(title);

    KrImageButton * closeBtn = new KrImageButton(titleWg);
    closeBtn->setIcon(KIcon("window-close"));
    closeBtn->setToolTip(i18n("Close"));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(reject()));
    hbox->addWidget(closeBtn);

    grid_main->addWidget(titleWg, 0, 0);

    QWidget *toolWg = new QWidget(this);
    QHBoxLayout * hbox2 = new QHBoxLayout(toolWg);
    hbox2->setSpacing(0);

    _newTabButton = new QToolButton(this);
    _newTabButton->setIcon(KIcon("tab-new"));
    _newTabButton->setToolTip(i18n("Create a new queue (Ctrl+T)"));
    connect(_newTabButton, SIGNAL(clicked()), this, SLOT(slotNewTab()));
    hbox2->addWidget(_newTabButton);

    _closeTabButton = new QToolButton(this);
    _closeTabButton->setIcon(KIcon("tab-close"));
    _closeTabButton->setToolTip(i18n("Remove the current queue (Ctrl+W)"));
    connect(_closeTabButton, SIGNAL(clicked()), this, SLOT(slotDeleteCurrentTab()));
    hbox2->addWidget(_closeTabButton);

    _pauseButton = new QToolButton(this);
    _pauseButton->setIcon(KIcon("media-playback-pause"));
    connect(_pauseButton, SIGNAL(clicked()), this, SLOT(slotPauseClicked()));
    hbox2->addWidget(_pauseButton);

    _progressBar = new QProgressBar(this);
    _progressBar->setMinimum(0);
    _progressBar->setMaximum(100);
    _progressBar->setValue(0);
    _progressBar->setFormat(i18n("unused"));
    _progressBar->setTextVisible(true);
    hbox2->addWidget(_progressBar);

    _scheduleButton = new QToolButton(this);
    _scheduleButton->setIcon(KIcon("chronometer"));
    _scheduleButton->setToolTip(i18n("Schedule queue starting (Ctrl+S)"));
    connect(_scheduleButton, SIGNAL(clicked()), this, SLOT(slotScheduleClicked()));
    hbox2->addWidget(_scheduleButton);

    grid_main->addWidget(toolWg, 1, 0);

    _queueWidget = new QueueWidget(this);
    connect(_queueWidget, SIGNAL(currentChanged()), this, SLOT(slotUpdateToolbar()));
    grid_main->addWidget(_queueWidget, 2, 0);

    _statusLabel = new QLabel(this);
    QSizePolicy statuspolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    statuspolicy.setHeightForWidth(_statusLabel->sizePolicy().hasHeightForWidth());
    _statusLabel->setSizePolicy(statuspolicy);
    _statusLabel->setFrameShape(QLabel::StyledPanel);
    _statusLabel->setFrameShadow(QLabel::Sunken);
    grid_main->addWidget(_statusLabel, 3, 0);

    setLayout(grid_main);

    KConfigGroup group(krConfig, "QueueManager");
    int sizeX = group.readEntry("Window Width",  -1);
    int sizeY = group.readEntry("Window Height",  -1);
    int x = group.readEntry("Window X",  -1);
    int y = group.readEntry("Window Y",  -1);

    if (sizeX != -1 && sizeY != -1)
        resize(sizeX, sizeY);
    else
        resize(300, 400);

    if (x != -1 && y != -1)
        move(x, y);
    else
        move(20, 20);

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

void QueueDialog::paintEvent(QPaintEvent * event)
{
    KDialog::paintEvent(event);
    QPainter p(this);

    int lineWidth = 2;
    int midLineWidth = 0;

    QRect rect = contentsRect();
    rect.adjust(-2, -2, 2, 2);

    QStyleOptionFrame opt;
    qDrawShadeRect(&p, rect, opt.palette, true, lineWidth, midLineWidth);
}

void QueueDialog::mousePressEvent(QMouseEvent *me)
{
    _clickPos = me->globalPos();
    _startPos = pos();
}


void QueueDialog::everyQueueIsEmpty()
{
    if (_queueDialog->_autoHide && _queueDialog != 0 && !_queueDialog->isHidden())
        _queueDialog->reject();
}


void QueueDialog::mouseMoveEvent(QMouseEvent *me)
{
    move(_startPos + me->globalPos() - _clickPos);
}

void QueueDialog::accept()
{
    _autoHide = true;
    saveSettings();
    KDialog::accept();
}

void QueueDialog::reject()
{
    _autoHide = true;
    saveSettings();
    KDialog::reject();
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

        _closeTabButton->setEnabled(_queueWidget->count() > 1);

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
                                                   i18n("Warning")) != KMessageBox::Continue) return;
        }
        QueueManager::removeQueue(currentQueue);
    }
}

void QueueDialog::slotPercentChanged(Queue * queue, int percent)
{
    if (QueueManager::currentQueue() == queue) {
        switch (percent) {
        case PERCENT_UNUSED:
            _progressBar->setMaximum(100);
            _progressBar->setValue(0);
            _progressBar->setFormat(i18n("unused"));
            break;
        case PERCENT_UNKNOWN:
            if (_progressBar->maximum() != 0) {
                _progressBar->setMaximum(0);
                _progressBar->setFormat("");
            }
            break;
        default:
            if (_progressBar->maximum() != 100)
                _progressBar->setMaximum(100);
            _progressBar->setValue(percent);
            _progressBar->setFormat("%p%");
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
    }
    KDialog::keyPressEvent(ke);
}

