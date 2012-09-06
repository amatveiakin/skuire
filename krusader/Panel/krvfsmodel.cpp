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

#include "krvfsmodel.h"
#include <klocale.h>
#include <QtDebug>
#include <QtAlgorithms>
#include "../VFS/krpermhandler.h"
#include "../VFS/abstractdirlister.h"
#include "../defaults.h"
#include "../krglobal.h"
#include "krcolorcache.h"
#include "krsort.h"


KrVfsModel::KrVfsModel(KrInterView * view): QAbstractListModel(0), _extensionEnabled(true), _view(view),
        _dummyItem(0), _ready(false), _justForSizeHint(false),
        _alternatingTable(false)
{
    KConfigGroup grpSvr(krConfig, "Look&Feel");
    _defaultFont = grpSvr.readEntry("Filelist Font", _FilelistFont);
}

KrVfsModel::~KrVfsModel()
{
    clear();
}

void KrVfsModel::refresh()
{
    clear();

#if QT_VERSION >= 0x040700
    _items.reserve(_view->dirLister()->numItems());
#endif

    if(!_view->dirLister()->isRoot()) {
        _dummyItem = new KrView::Item(KFileItem(KUrl(".."), "inode/directory", 0), _view, true);
        _items << _dummyItem;
    }

    bool showHidden = KrView::isShowHidden();

    foreach(KFileItem item, _view->dirLister()->items()) {
        //TODO: more efficient allocation
        if((!showHidden && item.isHidden()) || _view->isFiltered(item))
            continue;

        _items << new KrView::Item(item, _view);
    }

    _ready = true;

    if(lastSortOrder() != KrViewProperties::NoColumn)
        sort();
    else {
        emit layoutAboutToBeChanged();
        for(int i = 0; i < _items.count(); i++) {
            _itemIndex[_items[i]] = index(i, 0);
            _urlNdx[_items[i]->url()] = index(i, 0);
        }
        emit layoutChanged();
    }
}

void KrVfsModel::clear()
{
    if(!_items.count())
        return;

    emit layoutAboutToBeChanged();
    // clear persistent indexes
    QModelIndexList oldPersistentList = persistentIndexList();
    QModelIndexList newPersistentList;
    for(int i = 0; i < oldPersistentList.count(); i++)
        newPersistentList << QModelIndex();
    changePersistentIndexList(oldPersistentList, newPersistentList);

    _dummyItem = 0;
    foreach(KrView::Item *item, _items)
        delete item;
    _items.clear();
    _itemIndex.clear();
    _urlNdx.clear();

    emit layoutChanged();
}

int KrVfsModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return _items.count();
}

int KrVfsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return KrViewProperties::MAX_COLUMNS;
}

QVariant KrVfsModel::data(const QModelIndex& index, int role) const
{
    const KrView::Item *item = itemAt(index);
    if(!item)
        return QVariant();

    bool isDummy = (item == _dummyItem);

    switch (role) {
    case Qt::FontRole:
        return _defaultFont;
    case Qt::EditRole: {
        if (index.column() == 0)
            return item->name();
        else
            return QVariant();
    }
    case Qt::UserRole: {
        if (index.column() == 0)
            return item->nameWithoutExtension();
        else
            return QVariant();
    }
    case Qt::ToolTipRole:
    case Qt::DisplayRole: {
        if(isDummy) {
            switch (index.column()) {
            case KrViewProperties::Name:
                return "..";
            case KrViewProperties::Size:
                return i18n("<DIR>");
            default:
                return QString();
            }
        }
        switch (index.column()) {
        case KrViewProperties::Name:
            if(_extensionEnabled)
                return item->nameWithoutExtension();
            else
                return item->name();
        case KrViewProperties::Ext:
            return item->extension();
        case KrViewProperties::Size:
            return item->sizeString();
        case KrViewProperties::Type:
            return item->mimeComment();
        case KrViewProperties::Modified:
            return item->mtimeString();
        case KrViewProperties::Permissions:
            return item->permissionsString();
        case KrViewProperties::KrPermissions:
            return item->krPermissions();
        case KrViewProperties::Owner:
            return item->user();
        case KrViewProperties::Group:
            return item->group();
        default:
            return QString();
        }
        return QVariant();
    }
    case Qt::DecorationRole: {
        switch (index.column()) {
        case KrViewProperties::Name: {
            if (properties()->displayIcons) {
                if (_justForSizeHint)
                    //FIXME: cache this
                    return QPixmap(_view->fileIconSize(), _view->fileIconSize());
                else
                    return item->icon();
            }
            break;
        }
        default:
            break;
        }
        return QVariant();
    }
    case Qt::TextAlignmentRole: {
        switch (index.column()) {
        case KrViewProperties::Size:
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
        return QVariant();
    }
    case Qt::BackgroundRole:
    case Qt::ForegroundRole: {
        KrColorItemType colorItemType;
        colorItemType.m_activePanel = _view->isFocused();
        int actRow = index.row();
        if (_alternatingTable) {
            int itemNum = _view->itemsPerPage();
            if (itemNum == 0)
                itemNum++;
            if ((itemNum & 1) == 0)
                actRow += (actRow / itemNum);
        }
        colorItemType.m_alternateBackgroundColor = (actRow & 1);
        colorItemType.m_currentItem = _view->getCurrentIndex().row() == index.row();
        colorItemType.m_selectedItem = _view->isSelected(item);
        if (item->isLink()) {
            if (item->isBrokenLink())
                colorItemType.m_fileType = KrColorItemType::InvalidSymlink;
            else
                colorItemType.m_fileType = KrColorItemType::Symlink;
        } else if (item->isDir())
            colorItemType.m_fileType = KrColorItemType::Directory;
        //FIXME
//         else if (item->isExecutable())
//             colorItemType.m_fileType = KrColorItemType::Executable;
        else
            colorItemType.m_fileType = KrColorItemType::File;

        KrColorGroup cols;
        KrColorCache::getColorCache().getColors(cols, colorItemType);

        if (colorItemType.m_selectedItem) {
            if (role == Qt::ForegroundRole)
                return cols.highlightedText();
            else
                return cols.highlight();
        }
        if (role == Qt::ForegroundRole)
            return cols.text();
        else
            return cols.background();
    }
    default:
        return QVariant();
    }
}

bool KrVfsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role == Qt::EditRole) {
        KrView::Item *item = itemAt(index);
        if (!item)
            return false;
        emit renameItem(*item, value.toString());
    } else if (role == Qt::UserRole && index.isValid())
        _justForSizeHint = value.toBool();

    return QAbstractListModel::setData(index, value, role);
}

void KrVfsModel::sort(int column, Qt::SortOrder order)
{
    _view->sortModeUpdated(column, order);

    if(lastSortOrder() == KrViewProperties::NoColumn)
        return;

    emit layoutAboutToBeChanged();

    int count = _items.count();

    QModelIndexList oldPersistentList = persistentIndexList();

    KrSort::Sorter sorter = createSorter();
    sorter.sort();

    _items.clear();
    _itemIndex.clear();
    _urlNdx.clear();

#if QT_VERSION >= 0x040700
    _items.reserve(count);
#endif
    _itemIndex.reserve(count);
    _urlNdx.reserve(count);

    bool sortOrderChanged = false;
    QHash<int, int> changeMap;
    for (int i = 0; i < sorter.items().count(); ++i) {
        const KrSort::SortProps *props = sorter.items()[i];
        _items << const_cast<KrView::Item*>(props->viewItem());
        changeMap[ props->originalIndex() ] = i;
        if (i != props->originalIndex())
            sortOrderChanged = true;
        _itemIndex[ props->viewItem()] = index(i, 0);
        _urlNdx[ props->viewItem()->url()] = index(i, 0);
    }

    QModelIndexList newPersistentList;
    foreach(const QModelIndex &mndx, oldPersistentList)
        newPersistentList << index(changeMap[ mndx.row()], mndx.column());

    changePersistentIndexList(oldPersistentList, newPersistentList);

    emit layoutChanged();
    if (sortOrderChanged)
        _view->makeCurrentVisible();
}

void KrVfsModel::addItems(KFileItemList items)
{
    //FIXME: optimize

    emit layoutAboutToBeChanged();

    foreach(KFileItem fileItem, items) {
        //FIXME: make sure this wasn't  already added
        KrView::Item *newItem = new KrView::Item(fileItem, _view);

        if(lastSortOrder() == KrViewProperties::NoColumn) {
            int row = _items.count();
            _items << newItem;
            _itemIndex[newItem] = index(row, 0);
            _urlNdx[newItem->url()] = index(row, 0);
            continue;
        }

        QModelIndexList oldPersistentList = persistentIndexList();

        KrSort::Sorter sorter = createSorter();

        int insertRow = sorter.insertIndex(newItem, false, customSortData(newItem));
        if (insertRow != _items.count())
            _items.insert(insertRow, newItem);
        else
            _items << newItem;

        for (int i = insertRow; i < _items.count(); ++i) {
            _itemIndex[ _items[i] ] = index(i, 0);
            _urlNdx[ _items[ i ]->url()] = index(i, 0);
        }

        QModelIndexList newPersistentList;
        foreach(const QModelIndex &mndx, oldPersistentList) {
            int newRow = mndx.row();
            if (newRow >= insertRow)
                newRow++;
            newPersistentList << index(newRow, mndx.column());
        }

        changePersistentIndexList(oldPersistentList, newPersistentList);
    }

    emit layoutChanged();
}

QModelIndex KrVfsModel::removeItems(KFileItemList items)
{
    //FIXME: optimze

    QModelIndex currIndex = _view->getCurrentIndex();

    emit layoutAboutToBeChanged();

    foreach(KFileItem fileItem, items) {
        QModelIndex itemIdx = indexFromUrl(fileItem.url());
        if(!itemIdx.isValid())
            continue;

        int removeRow = itemIdx.row();
        KrView::Item *item = _items[removeRow];

        _view->setSelected(item, false);

        QModelIndexList oldPersistentList = persistentIndexList();
        QModelIndexList newPersistentList;

        _items.removeAt(removeRow);

        if (currIndex.row() == removeRow) {
            if (_items.count() == 0)
                currIndex = QModelIndex();
            else if (removeRow >= _items.count())
                currIndex = index(_items.count() - 1, 0);
            else
                currIndex = index(removeRow, 0);
        } else if (currIndex.row() > removeRow) {
            currIndex = index(currIndex.row() - 1, 0);
        }

        _itemIndex.remove(item);
        _urlNdx.remove(item->url());
        // update item/url index for items following the removed item
        for (int i = removeRow; i < _items.count(); i++) {
            _itemIndex[ _items[i] ] = index(i, 0);
            _urlNdx[ _items[i]->url() ] = index(i, 0);
        }

        foreach(const QModelIndex &mndx, oldPersistentList) {
            int newRow = mndx.row();
            if (newRow > removeRow)
                newRow--;
            if (newRow != removeRow)
                newPersistentList << index(newRow, mndx.column());
            else
                newPersistentList << QModelIndex();
        }
        changePersistentIndexList(oldPersistentList, newPersistentList);

        delete item;
    }

    emit layoutChanged();

    return currIndex;
}

void KrVfsModel::updateItems(const QList<QPair<KFileItem, KFileItem> >& items)
{
    //FIXME optimize

    KFileItemList addedItems;

    for(int i = 0; i < items.count(); i++) {
        KFileItem oldFile = items[i].first;
        KFileItem newFile = items[i].second;

        QModelIndex oldIndex = indexFromUrl(oldFile.url());

        if (!oldIndex.isValid()) {
            addedItems << newFile;
            continue;
        }

        int oldRow = oldIndex.row();

        KrView::Item *item = _items[oldRow];

        *item = KrView::Item(newFile, _view);

        if(lastSortOrder() == KrViewProperties::NoColumn) {
            _view->redrawItem(oldIndex);
            continue;
        }

        emit layoutAboutToBeChanged();

        _items.removeAt(oldRow);

        KrSort::Sorter sorter(createSorter());

        QModelIndexList oldPersistentList = persistentIndexList();

        int newRow = sorter.insertIndex(item, item == _dummyItem, customSortData(item));
        if (newRow != _items.count()) {
            if (newRow > oldRow)
                newRow--;
            _items.insert(newRow, item);
        } else
            _items << item;


        for (int i = (oldRow < newRow) ? oldRow : newRow; i < _items.count(); ++i) {
            _itemIndex[ _items[ i ] ] = index(i, 0);
            _urlNdx[ _items[ i ]->url()] = index(i, 0);
        }

        QModelIndexList newPersistentList;
        foreach(const QModelIndex &mndx, oldPersistentList) {
            int row = mndx.row();
            if (mndx.row() == oldRow)
                row = newRow;
            else if (mndx.row() < oldRow && mndx.row() >= newRow)
                    row++;
            else if (mndx.row() > oldRow && mndx.row() <= newRow)
                    row--;
            newPersistentList << index(row, mndx.column());
        }

        changePersistentIndexList(oldPersistentList, newPersistentList);
        emit layoutChanged();

        //redraw the item in any case
        if(oldRow != newRow)
            _view->redraw();
        else
            _view->redrawItem(itemIndex(item));
    }

    if (addedItems.count())
        addItems(addedItems);
}

QVariant KrVfsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // ignore anything that's not display, and not horizontal
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();
    else
        return KrView::columnDescription(section);
}

const QModelIndex & KrVfsModel::itemIndex(const KrView::Item *item)
{
    return _itemIndex[item];
}

Qt::ItemFlags KrVfsModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags flags = QAbstractListModel::flags(index);

    const KrView::Item *item = itemAt(index);
    if (!item)
        return flags;

    if (item == _dummyItem) {
        flags = (flags & (~Qt::ItemIsSelectable)) | Qt::ItemIsDropEnabled;
    } else
        flags = flags | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    return flags;
}

const QModelIndex & KrVfsModel::indexFromUrl(const KUrl &url)
{
    if(!url.isValid()) {
        static QModelIndex invalidIndex;
        return invalidIndex;
    }
    return _urlNdx[url];
}

KrSort::Sorter KrVfsModel::createSorter()
{
    KrSort::Sorter sorter(_items.count(), properties(), lessThanFunc(), greaterThanFunc());
    for(int i = 0; i < _items.count(); i++)
        sorter.addItem(_items[i], _items[i] == _dummyItem, i, customSortData(_items[i]));
    return sorter;
}
