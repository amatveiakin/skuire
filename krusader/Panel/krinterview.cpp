/*****************************************************************************
 * Copyright (C) 2002 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2002 Rafi Yanai <yanai@users.sourceforge.net>               *
 * Copyright (C) 2010 Jan Lepper <jan_lepper@gmx.de>                         *
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

#include <KColorScheme>
#include <QPainter>

#include "krinterview.h"
#include "krvfsmodel.h"
#include "krcolorcache.h"
#include "krmousehandler.h"
#include "krpreviews.h"
#include "../defaults.h"

//HACK
#include "krviewfactory.h"
#include "krinterdetailedview.h"
#include "krinterbriefview.h"

#include <QHelpEvent>


int ViewWidget::elementWidth(const QModelIndex & index)
{
    QString text = index.data(Qt::DisplayRole).toString();
    int width = 0;

    QVariant font = index.data(Qt::FontRole);
    if (font.isValid() && font.type() == QVariant::Font)
        width += QFontMetrics(font.value<QFont>()).width(text);

    const int textMargin = itemView()->style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    width += 2 * textMargin;

    QVariant decor = index.data(Qt::DecorationRole);
    if (decor.isValid() && decor.type() == QVariant::Pixmap)
        width += decor.value<QPixmap>().width() + 2 * textMargin;

    return width;
}


KrInterView::KrInterView(QWidget *parent, KrViewInstance &instance, KConfig *cfg) :
        KrView(instance, cfg),
        _model(0),
        _itemView(0),
        _mouseHandler(0),
        _parentWidget(parent),
        _widget(0),
        _isDraggedOver(false)
{
}

KrInterView::~KrInterView()
{
    delete _widget;
    _widget = 0;
    _itemView = 0;
    delete _model;
    _model = 0;
    delete _mouseHandler;
    _mouseHandler = 0;
}

void KrInterView::setup()
{
    _model = new KrVfsModel(this);
    _mouseHandler = new KrMouseHandler(this, _emitter);

    //HACK
    switch(_instance.id()) {
    case 0:
        _widget = new KrInterDetailedView(_parentWidget, this, _mouseHandler, _config);
        break;
    case 1:
        _widget = new KrInterBriefView(_parentWidget, this, _mouseHandler, _config);
        break;
    default:
        abort();
    }

    _model->setAlternatingTable(_widget->hasAlternatingTable());

    _itemView = _widget->itemView();
    _itemView->setModel(_model);
    _itemView->setSelectionModel(new DummySelectionModel(_model, _itemView));

    QObject::connect(_model, SIGNAL(renameItem(KFileItem, QString)),
                     _emitter, SIGNAL(renameItem(KFileItem, QString)));
    QObject::connect(_mouseHandler, SIGNAL(startDrag()),
                     op(), SLOT(startDrag()));

    visibleColumnsChanged();
}

void KrInterView::selectRegion(KUrl item1, KUrl item2, bool select, bool clearFirst)
{
    if(clearFirst)
        _selection.clear();

    QModelIndex mi1 = _model->indexFromUrl(item1);
    QModelIndex mi2 = _model->indexFromUrl(item2);

    if (mi1.isValid() && mi2.isValid()) {
        int r1 = mi1.row();
        int r2 = mi2.row();

        if (r1 > r2) {
            int t = r1;
            r1 = r2;
            r2 = t;
        }

        for (int row = r1; row <= r2; row++)
            setSelected(_model->itemAt(row), select);

    } else if (mi1.isValid())
        setSelected(_model->itemAt(mi1), select);
    else if (mi2.isValid())
        setSelected(_model->itemAt(mi2), (select));

    _emitter->emitSelectionChanged();

    redraw();
}

void KrInterView::setSelected(const KrView::Item *item, bool select)
{
    if(item == dummyItem())
        return;

    if(select) {
        clearSavedSelection();
        _selection.insert(item);
    } else
        _selection.remove(item);
}

QString KrInterView::getCurrentItem() const
{
    if (!_model->ready())
        return QString();

    Item *item = _model->itemAt(_itemView->currentIndex());
    return item ? item->name() : QString();
}

void KrInterView::makeCurrentVisible()
{
    _itemView->scrollTo(_itemView->currentIndex());
}

void KrInterView::prepareForActive()
{
    KrView::prepareForActive();
    _itemView->setFocus();
    Item *current = currentViewItem();
    if (current) {
        QString desc = itemDescription(current->url(), current == dummyItem());
        _emitter->emitItemDescription(desc);
    }
}

void KrInterView::prepareForPassive()
{
    KrView::prepareForPassive();
    _mouseHandler->cancelTwoClickRename();
    //if ( renameLineEdit() ->isVisible() )
    //renameLineEdit() ->clearFocus();
}

bool KrInterView::isDraggedOver() const
{
    return _isDraggedOver;
}

KFileItem KrInterView::getDragAndDropTarget()
{
    return _dragAndDropTarget;
}

void KrInterView::setDragState(bool isDraggedOver, KFileItem target)
{
    if (_isDraggedOver == isDraggedOver && _dragAndDropTarget == target)
        return;
    _isDraggedOver = isDraggedOver;
    _dragAndDropTarget = target;
    redraw();
}

void KrInterView::redraw()
{
    _itemView->viewport()->update();
}

void KrInterView::refreshColors()
{
    QPalette p(_itemView->palette());
    KrColorGroup cg;
    KrColorCache::getColorCache().getColors(cg, KrColorItemType(KrColorItemType::File,
        false, _focused, false, false));
    p.setColor(QPalette::Text, cg.text());
    p.setColor(QPalette::Base, cg.background());
    _itemView->setPalette(p);
    redraw();
}

void KrInterView::showContextMenu()
{
    _widget->showContextMenu(_itemView->viewport()->mapToGlobal(QPoint(0,0)));
}

void KrInterView::sortModeUpdated(int column, Qt::SortOrder order)
{
    KrView::sortModeUpdated(static_cast<KrViewProperties::ColumnType>(column),
                            order == Qt::DescendingOrder);
}

KIO::filesize_t KrInterView::calcSize()
{
    KIO::filesize_t size = 0;
    foreach(KrView::Item *item, _model->items()) {
        size += item->size();
    }
    return size;
}

KIO::filesize_t KrInterView::calcSelectedSize()
{
    KIO::filesize_t size = 0;
    foreach(const Item *item, _selection) {
        size += item->size();
    }
    return size;
}

KFileItemList KrInterView::getItems(KRQuery mask, bool dirs, bool files)
{
    KFileItemList list;
    foreach(KrView::Item *item, _model->items()) {
        if (item != _model->dummyItem())
            if ((!item->isDir() && files) || (item->isDir() && dirs)) {
                if(mask.isNull() || mask.match(*item))
                    list << *item;
            }
    }
    return list;
}

KFileItemList KrInterView::getSelectedItems(bool currentIfNoSelection)
{
    KFileItemList list;

    if(_selection.count()) {
        foreach(const Item *item, _selection)
            list << *item;
    } else if(currentIfNoSelection) {
        Item *item = _model->itemAt(_itemView->currentIndex());
        if(item && item != _model->dummyItem())
            list << *item;
    }

    return list;
}

KFileItemList KrInterView::getVisibleItems()
{
    //TODO: more efficient implementation in the subclasses
    KFileItemList list;
    foreach(const KrView::Item *item, _model->items()) {
        if (item != _model->dummyItem()) {
            if(_itemView->viewport()->rect().intersects(itemRect(_model->itemIndex(item))))
                list << *item;
        }
    }
    return list;
}

void KrInterView::makeItemVisible(KUrl url)
{
    QModelIndex ndx = _model->indexFromUrl(url);
    if (ndx.isValid())
        _itemView->scrollTo(ndx);
}

inline KrView::Item *KrInterView::currentViewItem()
{
    return _model->itemAt(_itemView->currentIndex());
}

KFileItem KrInterView::firstItem()
{
    int row = _model->dummyItem() ? 1 : 0;
    if (row < _model->rowCount())
        return *(_model->itemAt(row));
    else
        return KFileItem();
}

KFileItem KrInterView::lastItem()
{
    if (_model->rowCount()) {
        KrView::Item *item = _model->itemAt(_model->rowCount() - 1);
        if (item != _model->dummyItem())
            return *item;
    }
    return KFileItem();
}

KFileItem KrInterView::currentItem()
{
    KrView::Item *item = currentViewItem();
    if(item && item != _model->dummyItem())
        return *item;
    else
        return KFileItem();
}

bool KrInterView::currentItemIsUpUrl()
{
    return _model->dummyItem() &&
        (_model->itemAt(_itemView->currentIndex()) == _model->dummyItem());
}

void KrInterView::currentChanged(const QModelIndex &current)
{
    (void)current;
    Item *item = _model->itemAt(_itemView->currentIndex());
    _emitter->emitCurrentChanged(item ? *item : KFileItem());
}

QRect KrInterView::itemRect(KUrl itemUrl)
{
    QModelIndex index = _model->indexFromUrl(itemUrl);
    return index.isValid() ? itemRect(index) : QRect();
}

KFileItem KrInterView::itemAt(const QPoint &pos, bool *isUpUrl, bool includingUpUrl)
{
    Item *item = _model->itemAt(_itemView->indexAt(pos));
    if(isUpUrl)
        *isUpUrl = (item && (item == _model->dummyItem()));

    if(item && (item != _model->dummyItem() || includingUpUrl))
        return *item;
    else
        return KFileItem();
}

bool KrInterView::itemIsUpUrl(KFileItem item)
{
    return _model->dummyItem() && (item == *_model->dummyItem());
}

void KrInterView::setCurrentItem(KUrl url)
{
    setCurrentIndex(_model->indexFromUrl(url));
}

void KrInterView::setCurrentIndex(QModelIndex index)
{
    if((index.isValid() && index.row() != _itemView->currentIndex().row()) ||
            !index.isValid()) {
        _mouseHandler->cancelTwoClickRename();
        _itemView->setCurrentIndex(index);
    }
}

void KrInterView::changeSelection(KUrl::List urls, bool select, bool clearFirst)
{
    if(clearFirst)
        _selection.clear();

    foreach(const KUrl &url, urls) {
        QModelIndex idx = _model->indexFromUrl(url);
        if(idx.isValid())
            setSelected(_model->itemAt(idx), select);
    }

    _emitter->emitSelectionChanged();

    redraw();
}

void KrInterView::changeSelection(const KRQuery& filter, bool select, bool includeDirs)
{
    foreach(KrView::Item *item, _model->items()) {
        if (item == _model->dummyItem())
            continue;
        if (item->isDir() && !includeDirs)
            continue;

        if (filter.match(*item))
            setSelected(item, select);
    }

    _emitter->emitSelectionChanged();

    redraw();
}

void KrInterView::invertSelection()
{
    KConfigGroup grpSvr(_config, "Look&Feel");
    bool markDirs = grpSvr.readEntry("Mark Dirs", _MarkDirs);

    foreach(KrView::Item *item, _model->items()) {
        if (item == _model->dummyItem())
            continue;
        if (item->isDir() && !markDirs && !isSelected(item))
            continue;
        setSelected(item, !isSelected(item));
    }

    _emitter->emitSelectionChanged();

    redraw();
}

bool KrInterView::isItemSelected(KUrl url)
{
    Item *item = _model->itemAt(_model->indexFromUrl(url));
    return item ? isSelected(item) : false;
}

void KrInterView::updateItemSize(KUrl url, KIO::filesize_t newSize)
{
    Item *item = _model->itemAt(_model->indexFromUrl(url));
    if(item) {
        item->setCalculatedSize(newSize);
//FIXME
//         _itemView->viewport()->update(itemRect(vf));
        redraw();
    }
}

void KrInterView::setCurrentItemBySpec(ItemSpec spec)
{
    QModelIndex newIndex, currentIndex = _itemView->currentIndex();
    switch (spec) {
        case First:
            newIndex = _model->index(0, 0, QModelIndex());
            break;
        case Last:
            newIndex = _model->index(_model->rowCount() - 1, 0, QModelIndex());
            break;
        case Prev:
            if (currentIndex.row() <= 0)
                return;
            newIndex = _model->index(currentIndex.row() - 1, 0, QModelIndex());
            break;
        case Next:
            if (currentIndex.row() >= _model->rowCount() - 1)
                return;
            newIndex = _model->index(currentIndex.row() + 1, 0, QModelIndex());
            break;
        case UpUrl:
            if(!_model->dummyItem())
                return;
            newIndex = _model->itemIndex(_model->dummyItem());
            break;
        default:
            return;
    }
    setCurrentIndex(newIndex);
    _itemView->scrollTo(newIndex);
}

bool KrInterView::isItemVisible(KUrl url)
{
    return _itemView->viewport()->rect().intersects(itemRect(url));
}

QRect KrInterView::itemRectGlobal(KUrl url)
{
    QRect rect = itemRect(url);
    rect.moveTo(_itemView->viewport()->mapToGlobal(rect.topLeft()));
    return rect;
}

QPixmap KrInterView::icon(KUrl url)
{
    Item *item = _model->itemAt(_model->indexFromUrl(url));
    return item ? item->icon() : QPixmap();
}

bool KrInterView::isCurrentItemSelected()
{
    Item *item = _model->itemAt(_itemView->currentIndex());
    return item ? isSelected(item) : false;
}

void KrInterView::selectCurrentItem(bool select)
{
    Item *item = currentViewItem();
    if(item && item != _model->dummyItem()) {
        setSelected(item, select);
        _emitter->emitSelectionChanged();
        redrawItem(_itemView->currentIndex());
    }
}

void KrInterView::pageDown()
{
    int newIndex = _itemView->currentIndex().row() + itemsPerPage();
    if (newIndex >= _model->rowCount())
        newIndex = _model->rowCount() - 1;

    setCurrentIndex(_model->index(newIndex, 0));

//FIXME moveCursor is protected
//_itemView->moveCursor(QAbstractItemView::MovePageDown, Qt::NoModifier);

    makeCurrentVisible();
}

void KrInterView::pageUp()
{
    int newIndex = _itemView->currentIndex().row() - itemsPerPage();
    if (newIndex < 0)
        newIndex = 0;

    setCurrentIndex(_model->index(newIndex, 0));
    makeCurrentVisible();
}


QString KrInterView::currentDescription()
{
    KrView::Item *item = currentViewItem();
    if (item)
        return itemDescription(item->url(), item == _model->dummyItem());
    else
        return QString();
}

const KrView::Item *KrInterView::dummyItem() const
{
    return _model->dummyItem();
}

KrView::Item *KrInterView::itemFromUrl(KUrl url) const
{
    return _model->itemAt(_model->indexFromUrl(url));
}

bool KrInterView::quickSearch(const QString &term, int direction)
{
    if (!currentViewItem())
        return false;

    if (direction == 0) {
        if (currentViewItem() != dummyItem() && quickSearchMatch(currentItem(), term))
            return true;
        else
            direction = 1;
    }

    int startRow = _itemView->currentIndex().row();
    int row = startRow;

    while (true) {
        row += (direction > 0) ? 1 : -1;

        if (row < 0 || row >= _model->rowCount())
            row = (direction > 0) ? 0 : _model->rowCount() - 1;

        if (quickSearchMatch(*(_model->itemAt(row)), term)) {
            QModelIndex modelIndex = _model->index(row, 0);
            setCurrentIndex(modelIndex);
            _itemView->scrollTo(modelIndex);
            return true;
        } else if (row == startRow)
            return false;
    }
}

void KrInterView::refreshIcons()
{
    foreach(Item *item, _model->items())
        item->clearIcon();
    redraw();
}

void KrInterView::gotPreview(KFileItem item, QPixmap preview)
{
    QModelIndex index = _model->indexFromUrl(item.url());
    if(index.isValid()) {
        _model->itemAt(index)->setIcon(preview);
        redrawItem(index);
    }
}

uint KrInterView::count() const
{
    return _model->rowCount();
}

uint KrInterView::calcNumDirs() const
{
    uint num = 0;
    foreach(Item *item, _model->items())
        if(item->isDir())
            num++;
    return num;
}

void KrInterView::clear()
{
    if(_previews)
        _previews->clear();

    _selection.clear();
    _itemView->clearSelection();
    setCurrentIndex(QModelIndex());
    _model->clear();

    redraw();
}

void KrInterView::refresh()
{
    KUrl current = currentUrl();
    KUrl::List selection = getSelectedUrls(false);

    clear();

    if(!_dirLister)
        return;

    _model->refresh();
    setCurrentIndex(_model->index(0, 0));

    if(!selection.isEmpty())
        setSelection(selection);

    if (urlToMakeCurrent().isValid())
        setCurrentItem(urlToMakeCurrent());
    else if (current.isValid())
        setCurrentItem(current);

    updatePreviews();

    redraw();

    _emitter->emitSelectionChanged();
}

void KrInterView::newItems(const KFileItemList& items)
{
    if (items.count() > MaxIncrementalRefreshNum) {
        refresh();
        return;
    }

    KFileItemList itemsAfterFilter;

    foreach(const KFileItem &item, items) {
        if (isFiltered(item))
            continue;
        itemsAfterFilter << item;
    }

    if (itemsAfterFilter.count()) {
        bool wasEmpty = !count();
        _model->addItems(itemsAfterFilter);
        if (wasEmpty) {
            setCurrentIndex(_model->index(0, 0));
            makeCurrentVisible();
        }
        if (_previews)
            _previews->newItems(itemsAfterFilter);
    }

    if (urlToMakeCurrent().isValid()) {
        QModelIndex index = _model->indexFromUrl(urlToMakeCurrent());
        if (index.isValid()) {
            setCurrentIndex(index);
            makeCurrentVisible();
            setUrlToMakeCurrent(KUrl());
        }
    }

    if (!nameToMakeCurrentIfAdded().isEmpty()) {
        foreach(Item *item, _model->items()) {
            if (item->name() == nameToMakeCurrentIfAdded()) {
                KrView::setCurrentItem(*item);
                makeCurrentVisible();
                setNameToMakeCurrentIfAdded(QString());
                break;
            }
        }
    }

    _emitter->emitSelectionChanged();
}

void KrInterView::refreshItems(const QList<QPair<KFileItem, KFileItem> >& items)
{
    if (items.count() > MaxIncrementalRefreshNum) {
        refresh();
        return;
    }

    KFileItemList filtered;
    QList< QPair<KFileItem, KFileItem> > updated;

    for(int i = 0; i < items.count(); i++) {
        QPair<KFileItem, KFileItem> item = items[i];
        if (isFiltered(item.second))
            filtered << item.first;
        else
            updated << item;
    }

    itemsDeleted(filtered);

    _model->updateItems(updated);

    if(_previews)
        _previews->itemsUpdated(updated);

    makeCurrentVisible();
    _emitter->emitSelectionChanged();
}

void KrInterView::itemsDeleted(const KFileItemList& items)
{
    if (items.count() > MaxIncrementalRefreshNum) {
        refresh();
        return;
    }

    if(_previews)
        _previews->itemsDeleted(items);

    setCurrentIndex(_model->removeItems(items));

    makeCurrentVisible();
    _emitter->emitSelectionChanged();
    _emitter->emitItemDescription(currentDescription());
}

QRect KrInterView::itemRect(const QModelIndex &index)
{
    return _widget->itemRect(index);
}

void KrInterView::setFileIconSize(int size)
{
    KrView::setFileIconSize(size);
    _itemView->setIconSize(QSize(fileIconSize(), fileIconSize()));
}

void KrInterView::setSortMode(KrViewProperties::ColumnType sortColumn, bool descending)
{
    _widget->setSortMode(sortColumn, descending);
}

int KrInterView::itemsPerPage()
{
    return _widget->itemsPerPage();
}

void KrInterView::saveSettingsOfType(KConfigGroup grp, KrViewProperties::PropertyType properties)
{
    KrView::saveSettingsOfType(grp, properties);
    _widget->saveSettings(grp, properties);
}

void KrInterView::drawAdditionalDescorations(QAbstractItemView* view, QPainter& painter)
{
    if (_isDraggedOver) {
        painter.save();
        QRect r;
        if (!_dragAndDropTarget.isNull()) {
            QModelIndex index = _model->indexFromUrl(_dragAndDropTarget.url());
            r = itemRect(index).adjusted(0, 0, -1, -1);
        }
        else {
            r = view->viewport()->contentsRect().adjusted(0, 0, -1, -1);
        }
        painter.setPen(KrColorCache::getColorCache().getDropTargetFrameColor(_focused));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(r);
        painter.restore();
    }
}

void KrInterView::doRestoreSettings(KConfigGroup grp)
{
    KrView::doRestoreSettings(grp);
    _widget->restoreSettings(grp);
}

void KrInterView::renameCurrentItem()
{
    _widget->renameCurrentItem();
}

void KrInterView::copySettingsFrom(KrView *other)
{
    if(other->instance() == instance()) { // the other view is of the same type
        KrInterView *v = static_cast<KrInterView*>(other);
        _widget->copySettingsFrom(v->_widget);
        setFileIconSize(v->fileIconSize());
        _widget->setSortMode(v->properties()->sortColumn, v->_model->lastSortDir());
    }
}

void KrInterView::visibleColumnsChanged()
{
    _model->setExtensionEnabled(!_widget->isColumnHidden(KrViewProperties::Ext));
}

void KrInterView::toggleSelected(QModelIndexList indexes)
{
    foreach(const QModelIndex &index, indexes) {
        KrView::Item *item = _model->itemAt(index);
        if (item)
            setSelected(item, !isSelected(item));
    }
    _emitter->emitSelectionChanged();

    redraw();
}

int KrInterView::maxTextHeight()
{
    return QFontMetrics(_model->font()).height();
}

bool KrInterView::handleViewportEvent(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *he = static_cast<QHelpEvent*>(event);
        const QModelIndex index = _itemView->indexAt(he->pos());

        if (index.isValid()) {
            int width = _itemView->visualRect(index).width();
            int fullWidth = _widget->elementWidth(index);

            if (fullWidth <= width) {
                event->accept();
                return true;
            }
        }
    }
    return false;
}
