/*****************************************************************************
 * Copyright (C) 2002 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2002 Rafi Yanai <yanai@users.sourceforge.net>               *
 * Copyright (C) 2010 Jan Lepper <dehtris@yahoo.de>                          *
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

#ifndef KRINTERVIEW_H
#define KRINTERVIEW_H

#include <QSet>
#include <QAbstractItemView>

#include "krview.h"
#include "calcspacethread.h"

class KrVfsModel;
class KrMouseHandler;

class KrInterView : public KrView, public CalcSpaceClient
{
public:
    KrInterView(KrViewInstance &instance, KConfig *cfg, QAbstractItemView *itemView);
    virtual ~KrInterView();

    virtual CalcSpaceClient *calcSpaceClient() {
        return this;
    }

    virtual KFileItemList getItems(KRQuery mask = KRQuery(), bool dirs = true, bool files = true);
    virtual KFileItemList getSelectedItems(bool currentIfNoSelection);
    virtual KFileItemList getVisibleItems();
    virtual void changeSelection(KUrl::List urls, bool select, bool clearFirst);
    virtual void changeSelection(const KRQuery& filter, bool select, bool includeDirs);
    virtual void invertSelection();
    virtual bool isItemSelected(KUrl url);
    virtual KFileItem firstItem();
    virtual KFileItem lastItem();
    virtual KFileItem currentItem();
    virtual bool currentItemIsUpUrl();
    virtual void setCurrentItem(KUrl url);
    virtual void makeItemVisible(KUrl url);
    virtual void selectRegion(KUrl item1, KUrl item2, bool select, bool clearFirst);
    virtual KFileItem itemAt(const QPoint &vp, bool *isUpUrl);
    virtual bool isItemVisible(KUrl url);
    virtual QRect itemRectGlobal(KUrl url);
    virtual QPixmap icon(KUrl url);
    virtual bool isCurrentItemSelected();
    virtual void selectCurrentItem(bool select);
    virtual void pageDown();
    virtual void pageUp();
    virtual QString currentDescription();

    virtual QModelIndex getCurrentIndex() {
        return _itemView->currentIndex();
    }
    virtual bool isSelected(const QModelIndex &ndx);
    virtual uint numSelected() const {
        return _selection.count();
    }
    virtual KFileItem findItemByName(const QString &name);
    virtual QString getCurrentItem() const;
    virtual void setCurrentItem(const QString& name);
    virtual void clear();
    virtual void sort();
    virtual void refreshColors();
    virtual void redraw();
    virtual void prepareForActive();
    virtual void prepareForPassive();
    virtual void showContextMenu();
    virtual void makeCurrentVisible();

    void sortModeUpdated(int column, Qt::SortOrder order);

    void redrawItem(vfile *vf) {
        _itemView->viewport()->update(itemRect(vf));
    }

protected:
    class DummySelectionModel : public QItemSelectionModel
    {
    public:
        DummySelectionModel(QAbstractItemModel *model, QObject *parent) :
            QItemSelectionModel(model, parent) {}
        // do nothing - selection is managed by KrInterView
        virtual void select (const QModelIndex & index, QItemSelectionModel::SelectionFlags command) {}
        virtual void select(const QItemSelection & selection, QItemSelectionModel::SelectionFlags command) {}
    };

    // KrCalcSpaceDialog::Client implementation
    virtual void updateItemSize(KUrl url, KIO::filesize_t newSize);

    virtual void setCurrentItem(ItemSpec item);
    virtual KIO::filesize_t calcSize();
    virtual KIO::filesize_t calcSelectedSize();
    virtual void populate(const QList<vfile*> &vfiles, vfile *dummy);
    virtual void intAddItem(KFileItem item);
    virtual void intDelItem(KFileItem item);
    virtual void intUpdateItem(KFileItem item);
    virtual void intSetSelected(const KrView::Item *item, bool select);
    virtual void showContextMenu(const QPoint & p) = 0;
    virtual QRect itemRect(KUrl itemUrl);
    virtual const Item *dummyItem() const;
    virtual const Item *itemFromUrl(KUrl url) const;

    virtual QRect itemRect(const vfile *vf) = 0;

    bool isSelected(const KrView::Item *item) {
        return _selection.contains(item);
    }
    void currentChanged(const QModelIndex &current);
    void setCurrentIndex(QModelIndex index);
    inline KrView::Item *currentViewItem();


    KrVfsModel *_model;
    QAbstractItemView *_itemView;
    KrMouseHandler *_mouseHandler;
    QSet<const Item*> _selection;
};

#endif
