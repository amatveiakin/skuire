/*****************************************************************************
 * Copyright (C) 2002 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2002 Rafi Yanai <yanai@users.sourceforge.net>               *
 * Copyright (C) 2011 Jan Lepper <jan_lepper@gmx.de>                         *
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

#include "krinterdetailedview.h"

#include <QDir>
#include <QDirModel>
#include <QHashIterator>
#include <QHeaderView>
#include <QApplication>
#include <QContextMenuEvent>
#include <QPainter>

#include <klocale.h>
#include <kdirlister.h>
#include <kmenu.h>

#include "krviewfactory.h"
#include "krinterviewitemdelegate.h"
#include "krvfsmodel.h"
#include "../VFS/krpermhandler.h"
#include "../defaults.h"
#include "krmousehandler.h"
#include "krcolorcache.h"
#include "../GUI/krstyleproxy.h"

KrInterDetailedView::KrInterDetailedView(QWidget *parentWidget, ViewWidgetParent *parent,
                                         KrMouseHandler *mouseHandler, KConfig *cfg) :
    QTreeView(parentWidget),
    ViewWidget(parent, mouseHandler),
    _autoResizeColumns(true)
{
    Q_UNUSED(cfg)

    setRootIsDecorated(false);

    header()->installEventFilter(this);

    setSelectionMode(QAbstractItemView::NoSelection);
    setAllColumnsShowFocus(true);
    setUniformRowHeights(true);

    setStyle(new KrStyleProxy());
    setItemDelegate(new KrInterViewItemDelegate());
    setMouseTracking(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);

    for (int i = 0; i != KrViewProperties::MAX_COLUMNS; i++)
        header()->setResizeMode(i, QHeaderView::Interactive); //FIXME: may crash if section is not visible
    header()->setStretchLastSection(false);

    connect(header(), SIGNAL(sectionResized(int, int, int)), this, SLOT(sectionResized(int, int, int)));
    connect(header(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(sectionMoved(int, int, int)));
    connect(_mouseHandler, SIGNAL(renameCurrentItem()), this, SLOT(renameCurrentItem()));

    setSortMode(properties()->sortColumn,
                (properties()->sortOptions & KrViewProperties::Descending));
    setSortingEnabled(true);

}

void KrInterDetailedView::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    _parent->currentChanged(current);
    QTreeView::currentChanged(current, previous);
}

void KrInterDetailedView::restoreSettings(KConfigGroup grp)
{
    _autoResizeColumns = grp.readEntry("AutoResizeColumns", true);

    QByteArray savedState = grp.readEntry("Saved State", QByteArray());

    if (savedState.isEmpty()) {
        hideColumn(KrViewProperties::Type);
        hideColumn(KrViewProperties::Permissions);
        hideColumn(KrViewProperties::Owner);
        hideColumn(KrViewProperties::Group);
//FIXME        header()->resizeSection(KrViewProperties::Ext, QFontMetrics(_viewFont).width("tar.bz2  "));
//FIXME        header()->resizeSection(KrViewProperties::KrPermissions, QFontMetrics(_viewFont).width("rwx  "));
//FIXME        header()->resizeSection(KrViewProperties::Size, QFontMetrics(_viewFont).width("9") * 10);

        QDateTime tmp(QDate(2099, 12, 29), QTime(23, 59));
        QString desc = KGlobal::locale()->formatDateTime(tmp) + "  ";

//FIXME        header()->resizeSection(KrViewProperties::Modified, QFontMetrics(_viewFont).width(desc));
    } else
        header()->restoreState(savedState);

    _parent->visibleColumnsChanged();
}

void KrInterDetailedView::saveSettings(KConfigGroup grp, KrViewProperties::PropertyType properties)
{
    if (properties & KrViewProperties::PropColumns) {
        QByteArray state = header()->saveState();
        grp.writeEntry("Saved State", state);
    }
    if (properties & KrViewProperties::PropAutoResizeColumns)
        grp.writeEntry("AutoResizeColumns", _autoResizeColumns);
}

int KrInterDetailedView::itemsPerPage()
{
    QRect rect = visualRect(currentIndex());
    if (!rect.isValid()) {
        for (int i = 0; i != model()->rowCount(); i++) {
            rect = visualRect(model()->index(i, 0));
            if (rect.isValid())
                break;
        }
    }
    if (!rect.isValid())
        return 0;
    int size = (height() - header()->height()) / rect.height();
    if (size < 0)
        size = 0;
    return size;
}

void KrInterDetailedView::keyPressEvent(QKeyEvent *e)
{
    if (_parent->handleKeyEvent(e))
        return;
    else
        QTreeView::keyPressEvent(e);
}

void KrInterDetailedView::mousePressEvent(QMouseEvent * ev)
{
    if (!_mouseHandler->mousePressEvent(ev))
        QTreeView::mousePressEvent(ev);
}

void KrInterDetailedView::mouseReleaseEvent(QMouseEvent * ev)
{
    if (!_mouseHandler->mouseReleaseEvent(ev))
        QTreeView::mouseReleaseEvent(ev);
}

void KrInterDetailedView::mouseDoubleClickEvent(QMouseEvent *ev)
{
    if (!_mouseHandler->mouseDoubleClickEvent(ev))
        QTreeView::mouseDoubleClickEvent(ev);
}

void KrInterDetailedView::mouseMoveEvent(QMouseEvent * ev)
{
    if (!_mouseHandler->mouseMoveEvent(ev))
        QTreeView::mouseMoveEvent(ev);
}

void KrInterDetailedView::wheelEvent(QWheelEvent *ev)
{
    if (!_mouseHandler->wheelEvent(ev))
        QTreeView::wheelEvent(ev);
}

void KrInterDetailedView::dragEnterEvent(QDragEnterEvent *ev)
{
    if (!_mouseHandler->dragEnterEvent(ev))
        QTreeView::dragEnterEvent(ev);
}

void KrInterDetailedView::dragMoveEvent(QDragMoveEvent *ev)
{
    QTreeView::dragMoveEvent(ev);
    _mouseHandler->dragMoveEvent(ev);
}

void KrInterDetailedView::dragLeaveEvent(QDragLeaveEvent *ev)
{
    if (!_mouseHandler->dragLeaveEvent(ev))
        QTreeView::dragLeaveEvent(ev);
}

void KrInterDetailedView::dropEvent(QDropEvent *ev)
{
    if (!_mouseHandler->dropEvent(ev))
        QTreeView::dropEvent(ev);
}

bool KrInterDetailedView::event(QEvent * e)
{
    _mouseHandler->otherEvent(e);
    return QTreeView::event(e);
}

void KrInterDetailedView::renameCurrentItem()
{
    QModelIndex cIndex = currentIndex();
    QModelIndex nameIndex = model()->index(cIndex.row(), KrViewProperties::Name);
    edit(nameIndex);
    updateEditorData();
    update(nameIndex);
}

void KrInterDetailedView::paintEvent(QPaintEvent *ev)
{
    QTreeView::paintEvent(ev);
    QPainter painter(viewport());
    _parent->drawAdditionalDescorations(this, painter);
}

bool KrInterDetailedView::eventFilter(QObject *object, QEvent *event)
{
    if (object == header()) {
        if (event->type() == QEvent::ContextMenu) {
            QContextMenuEvent *me = (QContextMenuEvent *)event;
            showContextMenu(me->globalPos());
            return true;
        } else if (event->type() == QEvent::Resize) {
            recalculateColumnSizes();
            return false;
        }
    }
    return false;
}

void KrInterDetailedView::showContextMenu(const QPoint & p)
{
    KMenu popup(this);
    popup.setTitle(i18n("Columns"));

    QVector<QAction*> actions;

    for(int i = 0; i < model()->columnCount(); i++) {
        QString text = (model()->headerData(i, Qt::Horizontal)).toString();
        QAction *act = popup.addAction(text);
        act->setCheckable(true);
        act->setChecked(!header()->isSectionHidden(i));
        actions.append(act);
    }

    popup.addSeparator();
    QAction *actAutoResize = popup.addAction(i18n("Automatically Resize Columns"));
    actAutoResize->setCheckable(true);
    actAutoResize->setChecked(_autoResizeColumns);

    QAction *res = popup.exec(p);

    if (res == actAutoResize) {
        _autoResizeColumns = actAutoResize->isChecked();
        _parent->settingsChanged(KrViewProperties::PropAutoResizeColumns);
        recalculateColumnSizes();
    } else {
        int idx = actions.indexOf(res);
        if (idx < 0)
            return;

        if (header()->isSectionHidden(idx))
            header()->showSection(idx);
        else
            header()->hideSection(idx);

        _parent->settingsChanged(KrViewProperties::PropColumns);
        _parent->visibleColumnsChanged();
    }
}

void KrInterDetailedView::sectionResized(int /*column*/, int oldSize, int newSize)
{
    // *** taken from dolphin ***
    // If the user changes the size of the headers, the autoresize feature should be
    // turned off. As there is no dedicated interface to find out whether the header
    // section has been resized by the user or by a resize event, another approach is used.
    // Attention: Take care when changing the if-condition to verify that there is no
    // regression in combination with bug 178630 (see fix in comment #8).
    if ((QApplication::mouseButtons() & Qt::LeftButton) && header()->underMouse()) {
        _autoResizeColumns = false;
        _parent->settingsChanged(KrViewProperties::PropColumns);
    }

    if (oldSize == newSize)
        return;

    recalculateColumnSizes();
}

void KrInterDetailedView::sectionMoved(int /*logicalIndex*/, int /*oldVisualIndex*/, int /*newVisualIndex*/)
{
    _parent->settingsChanged(KrViewProperties::PropColumns);
}

void KrInterDetailedView::recalculateColumnSizes()
{
    if (!_autoResizeColumns)
        return;

    int sum = 0;
    for (int i = 0; i != model()->columnCount(); i++) {
        if (!isColumnHidden(i))
            sum += header()->sectionSize(i);
    }

    if (sum != header()->width()) {
        int delta = sum - header()->width();
        int nameSize = header()->sectionSize(KrViewProperties::Name);
        if (nameSize - delta > 20)
            header()->resizeSection(KrViewProperties::Name, nameSize - delta);
    }
}

bool KrInterDetailedView::viewportEvent(QEvent * event)
{
    if (_parent->handleViewportEvent(event))
        return true;
    else
        return QTreeView::viewportEvent(event);
}

void KrInterDetailedView::setSortMode(KrViewProperties::ColumnType sortColumn, bool descending)
{
    Qt::SortOrder sortDir = descending ? Qt::DescendingOrder : Qt::AscendingOrder;
    sortByColumn(sortColumn, sortDir);
}

QRect KrInterDetailedView::itemRect(const QModelIndex &index)
{
    QRect r = visualRect(index);
    r.setLeft(0);
    r.setWidth(header()->length());
    return r;
}

void KrInterDetailedView::copySettingsFrom(ViewWidget *other)
{
    KrInterDetailedView *otherWidget = qobject_cast<KrInterDetailedView*>(other->itemView());

    if (otherWidget) { // the other view is of the same type
        header()->restoreState(otherWidget->header()->saveState());
        _parent->visibleColumnsChanged();
        _autoResizeColumns = otherWidget->_autoResizeColumns;
        recalculateColumnSizes();
    }
}
