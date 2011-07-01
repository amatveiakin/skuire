/*****************************************************************************
 * Copyright (C) 2002 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2002 Rafi Yanai <yanai@users.sourceforge.net>               *
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
#include "../VFS/vfile.h"
#include <klocale.h>
#include <QtDebug>
#include <QtAlgorithms>
#include "../VFS/krpermhandler.h"
#include "../defaults.h"
#include "../krglobal.h"
#include "krpanel.h"
#include "krcolorcache.h"
#include "krsort.h"


KrVfsModel::KrVfsModel(KrInterView * view): QAbstractListModel(0), _extensionEnabled(true), _view(view),
        _dummyItem(0), _ready(false), _justForSizeHint(false),
        _alternatingTable(false)
{
    KConfigGroup grpSvr(krConfig, "Look&Feel");
    _defaultFont = grpSvr.readEntry("Filelist Font", _FilelistFont);
}

void KrVfsModel::populate(const KFileItemList &items, bool addDummyItem)
{
#if QT_VERSION >= 0x040700
    _items.reserve(items.count());
#endif

    if(addDummyItem) {
        _dummyItem = new KrView::Item(KFileItem(KUrl(".."), "inode/directory", 0), true);
        _items << _dummyItem;
    }

    foreach(KFileItem fileItem, items) {
        //TODO: more efficient allocation
        _items << new KrView::Item(fileItem);
    }

    _ready = true;

    if(lastSortOrder() != KrViewProperties::NoColumn)
        sort();
    else {
        emit layoutAboutToBeChanged();
        for(int i = 0; i < _items.count(); i++) {
            _itemIndex[_items[i]] = index(i, 0);
            _nameNdx[_items[i]->name()] = index(i, 0);
        }
        emit layoutChanged();
    }
}

KrVfsModel::~KrVfsModel()
{
    clear();
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

    foreach(KrView::Item *item, _items)
        delete item;
    _items.clear();
    _itemIndex.clear();
    _nameNdx.clear();
    _dummyItem = 0;

    emit layoutChanged();
}

int KrVfsModel::rowCount(const QModelIndex& parent) const
{
    (void)parent;
    return _items.count();
}

int KrVfsModel::columnCount(const QModelIndex &parent) const
{
    (void)parent;
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
            return nameWithoutExtension(item, false);
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
            //FIXME: cache name/ext
            return nameWithoutExtension(item);
        case KrViewProperties::Ext: {
            //FIXME: cache name/ext
            QString nameOnly = nameWithoutExtension(item);
            return item->name().mid(nameOnly.length() + 1);
        }
        case KrViewProperties::Size: {
            if (item->isDir() && item->size() <= 0)
                return i18n("<DIR>");
            else //FIXME: cache this
                return (properties()->humanReadableSize) ?
                       KIO::convertSize(item->size()) + "  " :
                       KRpermHandler::parseSize(item->size()) + ' ';
        }
        case KrViewProperties::Type: {
            KMimeType::Ptr mt = KMimeType::mimeType(item->mimetype());
            if (mt)
                return mt->comment(); //FIXME: cache this
            return QVariant();
        }
        case KrViewProperties::Modified: {
            //FIXME: cache this
            //FIXME: this doesn't return the correct locale format
            return item->time(KFileItem::ModificationTime).toString(KDateTime::LocalDate);
        }
        case KrViewProperties::Permissions: {
            if (properties()->numericPermissions) {
                QString perm; //FIXME: cache this
                return perm.sprintf("%.4o", item->mode() & PERM_BITMASK);
            } else
                return item->permissionsString();
        }
        case KrViewProperties::KrPermissions:
            return item->krPermissionsString();
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
                else //FIXME: and maybe cache this too
                    return _view->getIcon(item);
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
        //FIXME: replace with isSelected(item);
        colorItemType.m_selectedItem = _view->isSelected(index);
        if (item->isLink()) {
            //FIXME
//             if (vf->vfile_isBrokenLink())
//                 colorItemType.m_fileType = KrColorItemType::InvalidSymlink;
//             else
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
        _view->op()->emitRenameItem(*item, value.toString());
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
    _nameNdx.clear();

#if QT_VERSION >= 0x040700
    _items.reserve(count);
    _itemIndex.reserve(count);
    _nameNdx.reserve(count);
#endif

    bool sortOrderChanged = false;
    QHash<int, int> changeMap;
    for (int i = 0; i < sorter.items().count(); ++i) {
        const KrSort::SortProps *props = sorter.items()[i];
        _items << const_cast<KrView::Item*>(props->viewItem());
        changeMap[ props->originalIndex() ] = i;
        if (i != props->originalIndex())
            sortOrderChanged = true;
        _itemIndex[ props->viewItem()] = index(i, 0);
        _nameNdx[ props->viewItem()->name()] = index(i, 0);
    }

    QModelIndexList newPersistentList;
    foreach(const QModelIndex &mndx, oldPersistentList)
        newPersistentList << index(changeMap[ mndx.row()], mndx.column());

    changePersistentIndexList(oldPersistentList, newPersistentList);

    emit layoutChanged();
    if (sortOrderChanged)
        _view->makeCurrentVisible();
}

QModelIndex KrVfsModel::addItem(KFileItem fileItem)
{
    //FIXME: make sure this wasn't  already added

    emit layoutAboutToBeChanged();

    KrView::Item *newItem = new KrView::Item(fileItem);

    if(lastSortOrder() == KrViewProperties::NoColumn) {
        int idx = _items.count();
        _items << newItem;
        _itemIndex[newItem] = index(idx, 0);
        _nameNdx[newItem->name()] = index(idx, 0);
        emit layoutChanged();
        return index(idx, 0);
    }

    QModelIndexList oldPersistentList = persistentIndexList();

    KrSort::Sorter sorter = createSorter();

    int insertIndex = sorter.insertIndex(newItem, false, customSortData(newItem));
    if (insertIndex != _items.count())
        _items.insert(insertIndex, newItem);
    else
        _items << newItem;

    for (int i = insertIndex; i < _items.count(); ++i) {
        _itemIndex[ _items[i] ] = index(i, 0);
        _nameNdx[ _items[ i ]->name()] = index(i, 0);
    }

    QModelIndexList newPersistentList;
    foreach(const QModelIndex &mndx, oldPersistentList) {
        int newRow = mndx.row();
        if (newRow >= insertIndex)
            newRow++;
        newPersistentList << index(newRow, mndx.column());
    }

    changePersistentIndexList(oldPersistentList, newPersistentList);
    emit layoutChanged();
    _view->makeCurrentVisible();

    return index(insertIndex, 0);
}

QModelIndex KrVfsModel::removeItem(KFileItem fileItem)
{
    QModelIndex currIndex = _view->getCurrentIndex();

    QModelIndex itemIdx = indexFromUrl(fileItem.url());
    if(!itemIdx.isValid())
        return currIndex;

    int removeIdx = itemIdx.row();
    KrView::Item *item = _items[removeIdx];

    emit layoutAboutToBeChanged();
    QModelIndexList oldPersistentList = persistentIndexList();
    QModelIndexList newPersistentList;

    _items.removeAt(removeIdx);

    if (currIndex.row() == removeIdx) {
        if (_items.count() == 0)
            currIndex = QModelIndex();
        else if (removeIdx >= _items.count())
            currIndex = index(_items.count() - 1, 0);
        else
            currIndex = index(removeIdx, 0);
    } else if (currIndex.row() > removeIdx) {
        currIndex = index(currIndex.row() - 1, 0);
    }

    _itemIndex.remove(item);
    _nameNdx.remove(item->name());
    // update model/name index for items following the removed item
    for (int i = removeIdx; i < _items.count(); i++) {
        _itemIndex[ _items[i] ] = index(i, 0);
        _nameNdx[ _items[i]->name() ] = index(i, 0);
    }

    foreach(const QModelIndex &mndx, oldPersistentList) {
        int newRow = mndx.row();
        if (newRow > removeIdx)
            newRow--;
        if (newRow != removeIdx)
            newPersistentList << index(newRow, mndx.column());
        else
            newPersistentList << QModelIndex();
    }
    changePersistentIndexList(oldPersistentList, newPersistentList);

    emit layoutChanged();
    _view->makeCurrentVisible();

    delete item;

    return currIndex;
}

void KrVfsModel::updateItem(KFileItem oldFile, KFileItem newFile)
{
    QModelIndex oldModelIndex = indexFromUrl(oldFile.url());

    if (!oldModelIndex.isValid()) {
        addItem(newFile);
        return;
    }

    int oldIndex = oldModelIndex.row();

    KrView::Item *item = _items[oldIndex];

    *item = newFile;

    if(lastSortOrder() == KrViewProperties::NoColumn) {
        _view->redrawItem(oldModelIndex);
        return;
    }

    emit layoutAboutToBeChanged();

    _items.removeAt(oldIndex);

    KrSort::Sorter sorter(createSorter());

    QModelIndexList oldPersistentList = persistentIndexList();

    int newIndex = sorter.insertIndex(item, item == _dummyItem, customSortData(item));
    if (newIndex != _items.count()) {
        if (newIndex > oldIndex)
            newIndex--;
        _items.insert(newIndex, item);
    } else
        _items << item;


    int i = newIndex;
    if (oldIndex < i)
        i = oldIndex;
    for (; i < _items.count(); ++i) {
        _itemIndex[ _items[ i ] ] = index(i, 0);
        _nameNdx[ _items[ i ]->name()] = index(i, 0);
    }

    QModelIndexList newPersistentList;
    foreach(const QModelIndex &mndx, oldPersistentList) {
        int newRow = mndx.row();
        if (newRow == oldIndex)
            newRow = newIndex;
        else {
            if (newRow >= oldIndex)
                newRow--;
            if (mndx.row() > newIndex)
                newRow++;
        }
        newPersistentList << index(newRow, mndx.column());
    }

    changePersistentIndexList(oldPersistentList, newPersistentList);
    emit layoutChanged();
    if (newIndex != oldIndex)
        _view->makeCurrentVisible();

    //redraw the item in any case
    if(oldIndex != newIndex)
        _view->redraw();
    else
        _view->redrawItem(itemIndex(item));
}

QVariant KrVfsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // ignore anything that's not display, and not horizontal
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
    case KrViewProperties::Name: return i18n("Name");
    case KrViewProperties::Ext: return i18n("Ext");
    case KrViewProperties::Size: return i18n("Size");
    case KrViewProperties::Type: return i18n("Type");
    case KrViewProperties::Modified: return i18n("Modified");
    case KrViewProperties::Permissions: return i18n("Perms");
    case KrViewProperties::KrPermissions: return i18n("rwx");
    case KrViewProperties::Owner: return i18n("Owner");
    case KrViewProperties::Group: return i18n("Group");
    }
    return QString();
}

const QModelIndex & KrVfsModel::itemIndex(const KrView::Item *item)
{
    return _itemIndex[item];
}

const QModelIndex & KrVfsModel::nameIndex(const QString & st)
{
    return _nameNdx[ st ];
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

QString KrVfsModel::nameWithoutExtension(const KrView::Item *item, bool checkEnabled) const
{
    //FIXME: cache name/ext
    if ((checkEnabled && !_extensionEnabled) || item->isDir())
        return item->name();
    // check if the file has an extension
    const QString& vfName = item->name();
    int loc = vfName.lastIndexOf('.');
    if (loc > 0) { // avoid mishandling of .bashrc and friend
        // check if it has one of the predefined 'atomic extensions'
        for (QStringList::const_iterator i = properties()->atomicExtensions.begin(); i != properties()->atomicExtensions.end(); ++i) {
            if (vfName.endsWith(*i) && vfName != *i) {
                loc = vfName.length() - (*i).length();
                break;
            }
        }
    } else
        return vfName;
    return vfName.left(loc);
}

const QModelIndex & KrVfsModel::indexFromUrl(const KUrl &url)
{
    //TODO: use url index instead of name index
    //HACK
    if(!url.isValid()) {
        static QModelIndex invalidIndex;
        return invalidIndex;
    }
    return nameIndex(url.fileName());
}

KrSort::Sorter KrVfsModel::createSorter()
{
    KrSort::Sorter sorter(_items.count(), properties(), lessThanFunc(), greaterThanFunc());
    for(int i = 0; i < _items.count(); i++)
        sorter.addItem(_items[i], _items[i] == _dummyItem, i, customSortData(_items[i]));
    return sorter;
}
