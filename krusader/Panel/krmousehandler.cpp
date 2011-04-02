#if 1
/*****************************************************************************
 * Copyright (C) 2009 Csaba Karai <cskarai@freemail.hu>                      *
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

#include "krmousehandler.h"
#include "krview.h"
#include "krviewitem.h"
#include "krselectionmode.h"
#include "../krglobal.h"
#include "../defaults.h"

#include <QApplication>

#define CANCEL_TWO_CLICK_RENAME {_singleClicked = false;_renameTimer.stop();}

KrMouseHandler::KrMouseHandler(KrView * view, int contextMenuShift) : _view(view),
        _contextMenuTimer(), _contextMenuShift(contextMenuShift), _singleClicked(false), _singleClickTime(),
        _renameTimer(), _dragStartPos(-1, -1), _emptyContextMenu(false)
{
    KConfigGroup grpSvr(krConfig, "Look&Feel");
    // decide on single click/double click selection
    _singleClick = grpSvr.readEntry("Single Click Selects", _SingleClickSelects) && KGlobalSettings::singleClick();
    connect(&_contextMenuTimer, SIGNAL(timeout()), this, SLOT(showContextMenu()));
    connect(&_renameTimer, SIGNAL(timeout()), this, SIGNAL(renameCurrentItem()));
}

bool KrMouseHandler::mousePressEvent(QMouseEvent *e)
{
    _rightClickedItem = _clickedItem = KFileItem();
    KFileItem item = _view->itemAt(e->pos());
    if (!_view->isFocused())
        _view->op()->emitNeedFocus();
    if (e->button() == Qt::LeftButton) {
        _dragStartPos = e->pos();
        if (e->modifiers() == Qt::NoModifier) {
            if (!item.isNull()) {
                if (KrSelectionMode::getSelectionHandler()->leftButtonSelects()) {
                    if (KrSelectionMode::getSelectionHandler()->leftButtonPreservesSelection())
                        _view->toggleSelected(item);
                    else {
                        if (_view->isItemSelected(item))
                            _clickedItem = item;
                        else {
                            // clear the current selection
                            _view->setSelection(item);
                        }
                    }
                }
                _view->setCurrentItem(item);
            }
            e->accept();
            return true;
        } else if (e->modifiers() == Qt::ControlModifier) {
            if (!item.isNull() && (KrSelectionMode::getSelectionHandler()->shiftCtrlLeftButtonSelects() ||
                         KrSelectionMode::getSelectionHandler()->leftButtonSelects())) {
                _view->toggleSelected(item);
            }
            if (!item.isNull())
                _view->setCurrentItem(item);
            e->accept();
            return true;
        } else if (e->modifiers() == Qt::ShiftModifier) {
            if (!item.isNull() && (KrSelectionMode::getSelectionHandler()->shiftCtrlLeftButtonSelects() ||
                         KrSelectionMode::getSelectionHandler()->leftButtonSelects())) {
                KFileItem current = _view->currentItem();
                if (!current.isNull())
                    _view->selectRegion(item, current, true);
            }
            if (!item.isNull())
                _view->setCurrentItem(item);
            e->accept();
            return true;
        }
    }
    if (e->button() == Qt::RightButton) {
        //dragStartPos = e->pos();
        if (e->modifiers() == Qt::NoModifier) {
            if (!item.isNull()) {
                if (KrSelectionMode::getSelectionHandler()->rightButtonSelects()) {
                    if (KrSelectionMode::getSelectionHandler()->rightButtonPreservesSelection()) {
                        if (KrSelectionMode::getSelectionHandler()->showContextMenu() >= 0) {
                            _rightClickSelects = !_view->isItemSelected(item);
                            _rightClickedItem = item;
                        }
                        _view->toggleSelected(item);
                    } else {
                        if (_view->isItemSelected(item)) {
                            _clickedItem = item;
                        } else {
                            // clear the current selection
                            _view->setSelection(item);
                        }
                    }
                }
                _view->setCurrentItem(item);
            }
            handleContextMenu(item, e->globalPos());
            e->accept();
            return true;
        } else if (e->modifiers() == Qt::ControlModifier) {
            if (!item.isNull() && (KrSelectionMode::getSelectionHandler()->shiftCtrlRightButtonSelects() ||
                         KrSelectionMode::getSelectionHandler()->rightButtonSelects())) {
                _view->toggleSelected(item);
            }
            if (!item.isNull())
                _view->setCurrentItem(item);
            e->accept();
            return true;
        } else if (e->modifiers() == Qt::ShiftModifier) {
            if (!item.isNull() && (KrSelectionMode::getSelectionHandler()->shiftCtrlRightButtonSelects() ||
                         KrSelectionMode::getSelectionHandler()->rightButtonSelects())) {
                KFileItem current = _view->currentItem();
                if (!current.isNull())
                    _view->selectRegion(item, current, true);
            }
            if (!item.isNull())
                _view->setCurrentItem(item);
            e->accept();
            return true;
        }
    }
    return false;
}

bool KrMouseHandler::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        _dragStartPos = QPoint(-1, -1);
    bool itemIsUpUrl = false;
    KFileItem item = _view->itemAt(e->pos(), &itemIsUpUrl);

    if (!item.isNull() && item == _clickedItem) {
        if (((e->button() == Qt::LeftButton) && (e->modifiers() == Qt::NoModifier) &&
                (KrSelectionMode::getSelectionHandler()->leftButtonSelects()) &&
                !(KrSelectionMode::getSelectionHandler()->leftButtonPreservesSelection())) ||
                ((e->button() == Qt::RightButton) && (e->modifiers() == Qt::NoModifier) &&
                 (KrSelectionMode::getSelectionHandler()->rightButtonSelects()) &&
                 !(KrSelectionMode::getSelectionHandler()->rightButtonPreservesSelection()))) {
            // clear the current selection
            _view->setSelection(item);
        }
    }

    if (e->button() == Qt::RightButton) {
        _rightClickedItem = KFileItem();
        _contextMenuTimer.stop();
    }
    if (_singleClick && e->button() == Qt::LeftButton && e->modifiers() == Qt::NoModifier) {
        CANCEL_TWO_CLICK_RENAME;
        e->accept();
        if (item.isNull())
            return true;
        _view->op()->emitExecuted(item.name());
        return true;
    } else if (!_singleClick && e->button() == Qt::LeftButton) {
        if (!item.isNull() && e->modifiers() == Qt::NoModifier) {
            if (_singleClicked && !_renameTimer.isActive() && _singleClickedItem == item) {
                KSharedConfigPtr config = KGlobal::config();
                KConfigGroup group(krConfig, "KDE");
                int doubleClickInterval = group.readEntry("DoubleClickInterval", 400);

                int msecsFromLastClick = _singleClickTime.msecsTo(QTime::currentTime());

                if (msecsFromLastClick > doubleClickInterval && msecsFromLastClick < 5 * doubleClickInterval) {
                    _singleClicked = false;
                    _renameTimer.setSingleShot(true);
                    _renameTimer.start(doubleClickInterval);
                    return true;
                }
            }

            CANCEL_TWO_CLICK_RENAME;
            _singleClicked = true;
            _singleClickedItem = item;
            _singleClickTime = QTime::currentTime();

            return true;
        }
    }

    CANCEL_TWO_CLICK_RENAME;

    if (e->button() == Qt::MidButton && !item.isNull()) {
        e->accept();
        if (item.isNull())
            return true;
        _view->op()->emitMiddleButtonClicked(item, itemIsUpUrl);
        return true;
    }
    return false;
}

bool KrMouseHandler::mouseDoubleClickEvent(QMouseEvent *e)
{
    CANCEL_TWO_CLICK_RENAME;

    KFileItem item = _view->itemAt(e->pos());
    if (_singleClick)
        return false;
    if (e->button() == Qt::LeftButton && !item.isNull()) {
        e->accept();
        _view->op()->emitExecuted(item.name());
        return true;
    }
    return false;
}

bool KrMouseHandler::mouseMoveEvent(QMouseEvent *e)
{
    bool itemIsUpUrl = false;
    KFileItem item = _view->itemAt(e->pos(), &itemIsUpUrl);
    if ((_singleClicked || _renameTimer.isActive()) && item != _singleClickedItem)
        CANCEL_TWO_CLICK_RENAME;

    QString desc = _view->itemDescription(item.isNull() ? KUrl() : item.url(), itemIsUpUrl);
    if(!desc.isEmpty())
        _view->op()->emitItemDescription(desc);

    if (item.isNull())
        return false;

    if (_dragStartPos != QPoint(-1, -1) &&
            (e->buttons() & Qt::LeftButton) && (_dragStartPos - e->pos()).manhattanLength() > QApplication::startDragDistance()) {
        _view->op()->startDrag();
        return true;
    }

    if (KrSelectionMode::getSelectionHandler()->rightButtonPreservesSelection()
            && KrSelectionMode::getSelectionHandler()->rightButtonSelects()
            && KrSelectionMode::getSelectionHandler()->showContextMenu() >= 0 && e->buttons() == Qt::RightButton) {
        e->accept();
        if (!item.isNull() && !_rightClickedItem.isNull() && item != _rightClickedItem) {
            _view->selectRegion(item, _rightClickedItem, _rightClickSelects);
            _rightClickedItem = item;
            _view->setCurrentItem(item);
            _contextMenuTimer.stop();
        }
        return true;
    }
    return false;
}

bool KrMouseHandler::wheelEvent(QWheelEvent *)
{
    if (!_view->isFocused())
        _view->op()->emitNeedFocus();
    return false;
}

void KrMouseHandler::showContextMenu()
{
    if (!_rightClickedItem.isNull())
        _view->selectItem(_rightClickedItem, true);
    if (_emptyContextMenu)
        _view->op()->emitEmptyContextMenu(_contextMenuPoint);
    else
        _view->op()->emitContextMenu(_contextMenuPoint);
}

void KrMouseHandler::handleContextMenu(KFileItem item, const QPoint & pos)
{
    if (!_view->isFocused())
        _view->op()->emitNeedFocus();
    int i = KrSelectionMode::getSelectionHandler()->showContextMenu();
    _contextMenuPoint = QPoint(pos.x(), pos.y() - _contextMenuShift);
    if (i < 0) {
        if (item.isNull())
            _view->op()->emitEmptyContextMenu(_contextMenuPoint);
        else {
            _view->setCurrentItem(item);
            _view->op()->emitContextMenu(_contextMenuPoint);
        }
    } else if (i > 0) {
        _emptyContextMenu = item.isNull();
        _contextMenuTimer.setSingleShot(true);
        _contextMenuTimer.start(i);
    }
}

void KrMouseHandler::otherEvent(QEvent * e)
{
    switch (e->type()) {
    case QEvent::Timer:
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        break;
    default:
        CANCEL_TWO_CLICK_RENAME;
    }
}

void KrMouseHandler::cancelTwoClickRename()
{
    CANCEL_TWO_CLICK_RENAME;
}

bool KrMouseHandler::dragEnterEvent(QDragEnterEvent *e)
{
    KUrl::List URLs = KUrl::List::fromMimeData(e->mimeData());
    e->setAccepted(!URLs.isEmpty());
    return true;
}

bool KrMouseHandler::dragMoveEvent(QDragMoveEvent *e)
{
    KUrl::List URLs = KUrl::List::fromMimeData(e->mimeData());
    e->setAccepted(!URLs.isEmpty());
    return true;
}

bool KrMouseHandler::dragLeaveEvent(QDragLeaveEvent *e)
{
    return false;
}

bool KrMouseHandler::dropEvent(QDropEvent *e)
{
    _view->op()->emitGotDrop(e);
    return true;
}
#endif
