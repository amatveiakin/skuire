/*****************************************************************************
 * Copyright (C) 2009 Csaba Karai <cskarai@freemail.hu>                      *
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

#include "krinterbriefview.h"

#include <QDir>
#include <QDirModel>
#include <QHashIterator>
#include <QHeaderView>
#include <QPainter>
#include <QScrollBar>
#include <QRegion>
#include <QItemSelection>
#include <QItemSelectionRange>
#include <QApplication>
#include <QKeyEvent>

#include <kmenu.h>
#include <klocale.h>
#include <kdirlister.h>

#include "krviewfactory.h"
#include "krinterviewitemdelegate.h"
#include "krvfsmodel.h"
#include "krmousehandler.h"
#include "krcolorcache.h"
#include "../VFS/krpermhandler.h"
#include "../defaults.h"
#include "../GUI/krstyleproxy.h"


KrInterBriefView::KrInterBriefView(QWidget *parentWidget, ViewWidgetParent *parent,
                       KrMouseHandler *mouseHandler, KConfig *cfg) :
        KrItemView(parentWidget, parent, mouseHandler, cfg),
        _numOfColumns(_NumberOfBriefColumns),
        _header(0)
{
    _header = new QHeaderView(Qt::Horizontal, this);
    _header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    _header->setParent(this);
    _header->setStretchLastSection(true);
    _header->setResizeMode(QHeaderView::Fixed);
    _header->setClickable(true);
    _header->setSortIndicatorShown(true);
    _header->installEventFilter(this);

    setSortMode(properties()->sortColumn,
                properties()->sortOptions & KrViewProperties::Descending);
}

KrInterBriefView::~KrInterBriefView()
{
}

void KrInterBriefView::setModel(QAbstractItemModel *model)
{
    QAbstractItemView::setModel(model);
    connect(model, SIGNAL(layoutChanged()), SLOT(updateGeometries()));

    _header->setModel(model);
    _header->hideSection(KrViewProperties::Type);
    _header->hideSection(KrViewProperties::Permissions);
    _header->hideSection(KrViewProperties::KrPermissions);
    _header->hideSection(KrViewProperties::Owner);
    _header->hideSection(KrViewProperties::Group);
    connect(_header, SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)),
            model, SLOT(sort(int, Qt::SortOrder)));
}

void KrInterBriefView::restoreSettings(KConfigGroup group)
{
    _numOfColumns = group.readEntry("Number Of Brief Columns", _NumberOfBriefColumns);
    if (_numOfColumns  < 1)
        _numOfColumns  = 1;
    else if (_numOfColumns > MAX_BRIEF_COLS)
        _numOfColumns  = MAX_BRIEF_COLS;

    updateGeometries();
}

void KrInterBriefView::saveSettings(KConfigGroup grp, KrViewProperties::PropertyType properties)
{
    if(properties & KrViewProperties::PropColumns)
        grp.writeEntry("Number Of Brief Columns", _numOfColumns);
}

int KrInterBriefView::itemsPerPage()
{
    int height = getItemHeight();
    if (height == 0)
        height ++;
    int numRows = viewport()->height() / height;
    return numRows;
}

void KrInterBriefView::keyPressEvent(QKeyEvent *e)
{
    if ((e->key() != Qt::Key_Left && e->key() != Qt::Key_Right) &&
            (_parent->handleKeyEvent(e)))      // did the view class handled the event?
        return;

    switch (e->key()) {
    case Qt::Key_Right : {
        if (e->modifiers() == Qt::ControlModifier) {   // let the panel handle it
            e->ignore();
            break;
        }

        if (!currentIndex().isValid())
            break;

        int newCurrentRow = currentIndex().row() + itemsPerPage();
        if (newCurrentRow >= model()->rowCount())
            newCurrentRow = model()->rowCount() - 1;

        if (e->modifiers() & Qt::ShiftModifier) {
            QModelIndexList toggleSelected;
            for (int row = currentIndex().row(); row < newCurrentRow; row++)
                toggleSelected << model()->index(row, 0);
            _parent->toggleSelected(toggleSelected);
        }

        if (newCurrentRow >= 0) {
            setCurrentIndex(model()->index(newCurrentRow, 0));
            scrollTo(currentIndex());
        }

        break;
    }
    case Qt::Key_Left : {
        if (e->modifiers() == Qt::ControlModifier) {   // let the panel handle it
            e->ignore();
            break;
        }

        if (!currentIndex().isValid())
            break;

        int newCurrentRow = currentIndex().row() - itemsPerPage();
        if(newCurrentRow < 0)
            newCurrentRow = 0;

        if (e->modifiers() & Qt::ShiftModifier) {
            QModelIndexList toggleSelected;
            for (int row = currentIndex().row(); row > newCurrentRow; row--)
                toggleSelected << model()->index(row, 0);
            _parent->toggleSelected(toggleSelected);
        }

        if (newCurrentRow >= 0) {
            setCurrentIndex(model()->index(newCurrentRow, 0));
            scrollTo(currentIndex());
        }

        break;
    }
    default:
        QAbstractItemView::keyPressEvent(e);
    }
}

void KrInterBriefView::wheelEvent(QWheelEvent *ev)
{
    if (!_mouseHandler->wheelEvent(ev))
        QApplication::sendEvent(horizontalScrollBar(), ev);
}

bool KrInterBriefView::eventFilter(QObject *object, QEvent *event)
{
    if (object == _header) {
        if (event->type() == QEvent::ContextMenu) {
            QContextMenuEvent *me = (QContextMenuEvent *)event;
            showContextMenu(me->globalPos());
            return true;
        }
    }
    return false;
}

void KrInterBriefView::showContextMenu(const QPoint & p)
{
    KMenu popup(this);
    popup.setTitle(i18n("Columns"));

    int COL_ID = 14700;

    for (int i = 1; i <= MAX_BRIEF_COLS; i++) {
        QAction *act = popup.addAction(QString("%1").arg(i));
        act->setData(QVariant(COL_ID + i));
        act->setCheckable(true);
        act->setChecked(_numOfColumns == i);
    }

    QAction * res = popup.exec(p);
    int result = -1;
    if (res && res->data().canConvert<int>())
        result = res->data().toInt();

    if (result > COL_ID && result <= COL_ID + MAX_BRIEF_COLS) {
        _numOfColumns = result - COL_ID;
        updateGeometries();
        _parent->settingsChanged(KrViewProperties::PropColumns);
    }
}

QRect KrInterBriefView::visualRect(const QModelIndex&ndx) const
{
    int width = (viewport()->width()) / _numOfColumns;
    if ((viewport()->width()) % _numOfColumns)
        width++;
    int height = getItemHeight();
    int numRows = viewport()->height() / height;
    if (numRows == 0)
        numRows++;
    int x = width * (ndx.row() / numRows);
    int y = height * (ndx.row() % numRows);
    return mapToViewport(QRect(x, y, width, height));
}

void KrInterBriefView::scrollTo(const QModelIndex &ndx, QAbstractItemView::ScrollHint hint)
{
    const QRect rect = visualRect(ndx);
    if (hint == EnsureVisible && viewport()->rect().contains(rect)) {
        setDirtyRegion(rect);
        return;
    }

    const QRect area = viewport()->rect();

    const bool leftOf = rect.left() < area.left();
    const bool rightOf = rect.right() > area.right();

    int horizontalValue = horizontalScrollBar()->value();

    if (leftOf)
        horizontalValue -= area.left() - rect.left();
    else if (rightOf)
        horizontalValue += rect.right() - area.right();

    horizontalScrollBar()->setValue(horizontalValue);
}

QModelIndex KrInterBriefView::indexAt(const QPoint& p) const
{
    int x = p.x() + horizontalOffset();
    int y = p.y() + verticalOffset();

    int itemWidth = (viewport()->width()) / _numOfColumns;
    if ((viewport()->width()) % _numOfColumns)
        itemWidth++;
    int itemHeight = getItemHeight();

    int numRows = viewport()->height() / itemHeight;
    if (numRows == 0)
        numRows++;

    int row = y / itemHeight;
    int col = x / itemWidth;

    int numColsTotal = model()->rowCount() / numRows;
    if(model()->rowCount() % numRows)
        numColsTotal++;

    if(row < numRows && col < numColsTotal)
        return model()->index((col * numRows) + row, 0);

    return QModelIndex();
}

QModelIndex KrInterBriefView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers)
{
    if (model()->rowCount() == 0)
        return QModelIndex();

    QModelIndex current = currentIndex();
    if (!current.isValid())
        return model()->index(0, 0);

    switch (cursorAction) {
    case MoveLeft:
    case MovePageDown: {
        int newRow = current.row() - itemsPerPage();
        if (newRow < 0)
            newRow = 0;
        return model()->index(newRow, 0);
    }
    case MoveRight:
    case MovePageUp: {
        int newRow = current.row() + itemsPerPage();
        if (newRow >= model()->rowCount())
            newRow = model()->rowCount() - 1;
        return model()->index(newRow, 0);
    }
    case MovePrevious:
    case MoveUp: {
        int newRow = current.row() - 1;
        if (newRow < 0)
            newRow = 0;
        return model()->index(newRow, 0);
    }
    case MoveNext:
    case MoveDown: {
        int newRow = current.row() + 1;
        if (newRow >= model()->rowCount())
            newRow = model()->rowCount() - 1;
        return model()->index(newRow, 0);
    }
    case MoveHome:
        return model()->index(0, 0);
    case MoveEnd:
        return model()->index(model()->rowCount() - 1, 0);
    }

    return current;
}

int KrInterBriefView::horizontalOffset() const
{
    return horizontalScrollBar()->value();
}

int KrInterBriefView::verticalOffset() const
{
    return 0;
}

bool KrInterBriefView::isIndexHidden(const QModelIndex&ndx) const
{
    return ndx.column() != 0;
}

#if 0
QRegion KrInterBriefView::visualRegionForSelection(const QItemSelection &selection) const
{
    if (selection.isEmpty())
        return QRegion();

    QRegion selectionRegion;
    for (int i = 0; i < selection.count(); ++i) {
        QItemSelectionRange range = selection.at(i);
        if (!range.isValid())
            continue;
        QModelIndex leftIndex = range.topLeft();
        if (!leftIndex.isValid())
            continue;
        const QRect leftRect = visualRect(leftIndex);
        int top = leftRect.top();
        QModelIndex rightIndex = range.bottomRight();
        if (!rightIndex.isValid())
            continue;
        const QRect rightRect = visualRect(rightIndex);
        int bottom = rightRect.bottom();
        if (top > bottom)
            qSwap<int>(top, bottom);
        int height = bottom - top + 1;
        QRect combined = leftRect | rightRect;
        combined.setX(range.left());
        selectionRegion += combined;
    }
    return selectionRegion;
}
#endif

void KrInterBriefView::paintEvent(QPaintEvent *e)
{
    QStyleOptionViewItemV4 option = viewOptions();
    option.widget = this;
    option.decorationSize = iconSize();
    option.decorationPosition = QStyleOptionViewItem::Left;
    QPainter painter(viewport());

    QModelIndex curr = currentIndex();

    QVector<QModelIndex> intersectVector;
    QRect area = e->rect();
    area.adjust(horizontalOffset(), verticalOffset(), horizontalOffset(), verticalOffset());

    intersectionSet(area, intersectVector);

    foreach(const QModelIndex &mndx, intersectVector) {
        option.state = QStyle::State_None;
        option.rect = visualRect(mndx);
        painter.save();

        bool focus = curr.isValid() && curr.row() == mndx.row() && hasFocus();

        itemDelegate()->paint(&painter, option, mndx);

        if (focus) {
            QStyleOptionFocusRect o;
            o.QStyleOption::operator=(option);
            QPalette::ColorGroup cg = QPalette::Normal;
            o.backgroundColor = option.palette.color(cg, QPalette::Background);
            style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, &painter);
        }

        painter.restore();
    }

    _parent->drawAdditionalDescorations(this, painter);
}

int KrInterBriefView::getItemHeight() const
{
    int textHeight = _parent->maxTextHeight();
    int height = textHeight;
    int iconSize = 0;
    if (properties()->displayIcons)
        iconSize = this->iconSize().height();
    if (iconSize > textHeight)
        height = iconSize;
    if (height == 0)
        height++;
    return height;
}

void KrInterBriefView::updateGeometries()
{
    if (_header) {
        QSize hint = _header->sizeHint();
        setViewportMargins(0, hint.height(), 0, 0);
        QRect vg = viewport()->geometry();
        QRect geometryRect(vg.left(), vg.top() - hint.height(), vg.width(), hint.height());
        _header->setGeometry(geometryRect);

        int items = 0;
        for (int i = 0;  i != _header->count(); i++)
            if (!_header->isSectionHidden(i))
                items++;
        if (items == 0)
            items++;

        int sectWidth = viewport()->width() / items;
        for (int i = 0;  i != _header->count(); i++)
            if (!_header->isSectionHidden(i))
                _header->resizeSection(i, sectWidth);

        QMetaObject::invokeMethod(_header, "updateGeometries"); // UGLY isn't it ?
    }

    if (model()->rowCount() <= 0)
        horizontalScrollBar()->setRange(0, 0);
    else {
        int itemsPerColumn = viewport()->height() / getItemHeight();
        if (itemsPerColumn <= 0)
            itemsPerColumn = 1;
        int columnWidth = (viewport()->width()) / _numOfColumns;
        if ((viewport()->width()) % _numOfColumns)
            columnWidth++;
        int maxWidth = model()->rowCount() / itemsPerColumn;
        if (model()->rowCount() % itemsPerColumn)
            maxWidth++;
        maxWidth *= columnWidth;
        if (maxWidth > viewport()->width()) {
            horizontalScrollBar()->setSingleStep(columnWidth);
            horizontalScrollBar()->setPageStep(columnWidth * _numOfColumns);
            horizontalScrollBar()->setRange(0, maxWidth - viewport()->width());
        } else {
            horizontalScrollBar()->setRange(0, 0);
        }
    }

    QAbstractItemView::updateGeometries();
}

void KrInterBriefView::setSortMode(KrViewProperties::ColumnType sortColumn, bool descending)
{
    Qt::SortOrder sortDir = descending ? Qt::DescendingOrder : Qt::AscendingOrder;
    _header->setSortIndicator(sortColumn, sortDir);
}

void KrInterBriefView::intersectionSet(const QRect &rect, QVector<QModelIndex> &ndxList)
{
    int maxNdx = model()->rowCount();
    int width = (viewport()->width()) / _numOfColumns;
    if ((viewport()->width()) % _numOfColumns)
        width++;

    int height = getItemHeight();
    int items = viewport()->height() / height;
    if (items == 0)
        items++;

    int xmin = -1;
    int ymin = -1;
    int xmax = -1;
    int ymax = -1;

    xmin = rect.x() / width;
    ymin = rect.y() / height;
    xmax = (rect.x() + rect.width()) / width;
    if ((rect.x() + rect.width()) % width)
        xmax++;
    ymax = (rect.y() + rect.height()) / height;
    if ((rect.y() + rect.height()) % height)
        ymax++;

    for (int i = ymin; i < ymax; i++)
        for (int j = xmin; j < xmax; j++) {
            int ndx = j * items + i;
            if (ndx < maxNdx)
                ndxList.append(model()->index(ndx, 0));
        }
}

QRect KrInterBriefView::itemRect(const QModelIndex &index)
{
    return visualRect(index);
}

void KrInterBriefView::copySettingsFrom(ViewWidget *other)
{
    KrInterBriefView *otherWidget = qobject_cast<KrInterBriefView*>(other->itemView());
    if(otherWidget) { // the other view is of the same type
        _numOfColumns = otherWidget->_numOfColumns;
        updateGeometries();
    }
}
