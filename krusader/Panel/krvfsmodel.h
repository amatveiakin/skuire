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

#ifndef KRVFSMODEL_H
#define KRVFSMODEL_H

#include <QAbstractListModel>
#include <QFont>

#include "krinterview.h"
#include "krsort.h"


class KrViewProperties;

class KrVfsModel: public QAbstractListModel
{
    Q_OBJECT

public:
    KrVfsModel(KrInterView *);
    virtual ~KrVfsModel();

    inline bool ready() const {
        return _ready;
    }
    void clear();
    void refresh();
    void addItems(KFileItemList items);
    void updateItems(const QList<QPair<KFileItem, KFileItem> >& items);
    QModelIndex removeItems(KFileItemList items);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    void setExtensionEnabled(bool exten) {
        _extensionEnabled = exten;
    }
    const QFont &font() {
        return _defaultFont;
    }
    inline const KrViewProperties * properties() const {
        return _view->properties();
    }
    virtual void sort() {
        sort(lastSortOrder(), lastSortDir());
    }
    const QList<KrView::Item*> items() {
        return _items;
    }
    const KrView::Item *dummyItem() {
        return _dummyItem;
    }
    const QModelIndex & itemIndex(const KrView::Item*);
    const QModelIndex & nameIndex(const QString &);
    const QModelIndex & indexFromUrl(const KUrl &url);
    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    void emitChanged() {
        emit layoutChanged();
    }
    Qt::SortOrder lastSortDir() const {
        return (properties()->sortOptions & KrViewProperties::Descending) ? Qt::DescendingOrder : Qt::AscendingOrder;
    }
    int lastSortOrder() const {
        return properties()->sortColumn;
    }
    void setAlternatingTable(bool altTable) {
        _alternatingTable = altTable;
    }
    const KrView::Item *itemAt(const QModelIndex &index) const {
        return const_cast<KrVfsModel*>(this)->itemAt(index);
    }
    KrView::Item *itemAt(const QModelIndex &index) {
        return index.isValid() ? itemAt(index.row()) : 0;
    }
    KrView::Item *itemAt(int row) {
        if(row >= _items.count() || row < 0)
            return 0;
        else
            return _items[row];
    }

signals:
    void renameItem(KFileItem item, QString newName);

public slots:
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

protected:
    virtual KrSort::LessThanFunc lessThanFunc() const {
        return KrSort::itemLessThan;
    }
    virtual KrSort::LessThanFunc greaterThanFunc() const {
        return KrSort::itemGreaterThan;
    }
    virtual QVariant customSortData(const KrView::Item*) const {
        return QVariant();
    }

    KrSort::Sorter createSorter();

    QList<KrView::Item*>                _items;
    QHash<const KrView::Item*, QModelIndex>   _itemIndex;
    QHash<KUrl, QModelIndex>    _urlNdx;
    bool                        _extensionEnabled;
    KrInterView                 * _view;
    KrView::Item                *_dummyItem;
    bool                        _ready;
    QFont                       _defaultFont;
    bool                        _justForSizeHint;
    bool                        _alternatingTable;
};

#endif // __krvfsmodel__
