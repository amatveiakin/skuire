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

#ifndef KRINTERVIEW_H
#define KRINTERVIEW_H

#include <QSet>
#include <QAbstractItemView>

#include "krview.h"
#include "calcspacethread.h"

class KrVfsModel;
class KrMouseHandler;


class ViewWidgetParent
{
public:
    virtual ~ViewWidgetParent() {}
    virtual const KrViewProperties *properties() = 0;
    virtual int maxTextHeight() = 0;
    virtual bool handleKeyEvent(QKeyEvent *e) = 0;
    virtual bool handleViewportEvent(QEvent *event) = 0;
    virtual void currentChanged(const QModelIndex &current) = 0;
    virtual void settingsChanged(KrViewProperties::PropertyType properties) = 0;
    virtual void visibleColumnsChanged() = 0;
    virtual void toggleSelected(QModelIndexList indexes) = 0;
    virtual void drawAdditionalDescorations(QAbstractItemView* view, QPainter& painter) = 0;
};


class ViewWidget
{
public:
    ViewWidget(ViewWidgetParent *parent, KrMouseHandler *mouseHandler) :
        _parent(parent),
        _mouseHandler(mouseHandler) {}

    virtual ~ViewWidget() {}

    virtual QAbstractItemView *itemView() = 0;
    virtual QRect itemRect(const QModelIndex &index) = 0;
    virtual int itemsPerPage() = 0;
    virtual bool isColumnHidden(int column) = 0;
    virtual void renameCurrentItem() = 0;
    virtual void showContextMenu(const QPoint &p) = 0;
    virtual void setSortMode(KrViewProperties::ColumnType sortColumn, bool descending) = 0;
    virtual void saveSettings(KConfigGroup grp, KrViewProperties::PropertyType properties) = 0;
    virtual void restoreSettings(KConfigGroup grp) = 0;
    virtual void copySettingsFrom(ViewWidget *other) = 0;

    virtual int elementWidth(const QModelIndex &index);
    virtual bool hasAlternatingTable() {
        return false;
    }

protected:
    const KrViewProperties *properties() const {
        return _parent->properties();
    }

    ViewWidgetParent *_parent;
    KrMouseHandler *_mouseHandler;
};


class KrInterView : public QObject, public KrView, public ViewWidgetParent, public CalcSpaceClient
{
    Q_OBJECT
    Q_INTERFACES(KrView)

public:
    enum
    {
        //TODO: figure out the optimal value
        MaxIncrementalRefreshNum = 10
    };

    KrInterView(QWidget *parent, KrViewInstance &instance, KConfig *cfg);
    virtual ~KrInterView();

    // AbstractView implementation
    virtual QObject *self() {
        return this;
    }

    // KrView implementation
    virtual CalcSpaceClient *calcSpaceClient() {
        return this;
    }
    virtual uint count() const;
    virtual uint calcNumDirs() const;
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
    virtual KFileItem itemAt(const QPoint &pos, bool *isUpUrl, bool includingUpUrl);
    virtual bool itemIsUpUrl(KFileItem item);
    virtual bool isItemVisible(KUrl url);
    virtual QRect itemRectGlobal(KUrl url);
    virtual QPixmap icon(KUrl url);
    virtual QString currentDescription();
    virtual bool quickSearch(const QString &term, int direction);
    virtual QWidget *widget() {
        return _itemView;
    }
    virtual int itemsPerPage();
    virtual QModelIndex getCurrentIndex() {
        return _itemView->currentIndex();
    }
    virtual uint numSelected() const {
        return _selection.count();
    }
    virtual bool isDraggedOver() const;
    virtual KFileItem getDragAndDropTarget();
    virtual void setDragState(bool isDraggedOver, KFileItem target);
    virtual QString getCurrentItem() const;
    virtual void renameCurrentItem();
    virtual void clear();
    virtual void refresh();
    virtual void refreshColors();
    virtual void redraw();
    virtual void prepareForActive();
    virtual void prepareForPassive();
    virtual void showContextMenu();
    virtual void makeCurrentVisible();
    virtual void setFileIconSize(int size);
    virtual void setSortMode(KrViewProperties::ColumnType sortColumn, bool descending);
    virtual void copySettingsFrom(KrView*);

    // ViewWidgetParent implementation
    virtual const KrViewProperties *properties() {
        return KrView::properties();
    }

    void sortModeUpdated(int column, Qt::SortOrder order);

    bool isSelected(const KrView::Item *item) {
        return _selection.contains(item);
    }
    void setSelected(const KrView::Item *item, bool select);

    void redrawItem(const QModelIndex &index) {
        _itemView->viewport()->update(itemRect(index));
    }

    AbstractDirLister *dirLister() {
        return _dirLister;
    }
    void drawAdditionalDescorations(QAbstractItemView* view, QPainter& painter);

protected:
    class DummySelectionModel : public QItemSelectionModel
    {
    public:
        DummySelectionModel(QAbstractItemModel *model, QObject *parent) :
            QItemSelectionModel(model, parent) {}
        // do nothing - selection is managed by KrInterView
        virtual void select (const QModelIndex & index, QItemSelectionModel::SelectionFlags command) { Q_UNUSED(index); Q_UNUSED(command); }
        virtual void select(const QItemSelection & selection, QItemSelectionModel::SelectionFlags command) { Q_UNUSED(selection); Q_UNUSED(command); }
    };

    // ViewWidgetParent implementation
    virtual int maxTextHeight();
    virtual bool handleKeyEvent(QKeyEvent *e) {
        return KrView::handleKeyEvent(e);
    }
    virtual bool handleViewportEvent(QEvent *event);
    virtual void settingsChanged(KrViewProperties::PropertyType properties) {
        op()->settingsChanged(properties);
    }
    virtual void visibleColumnsChanged();
    virtual void toggleSelected(QModelIndexList indexes);

    // KrCalcSpaceDialog::Client implementation
    virtual void updateItemSize(KUrl url, KIO::filesize_t newSize);

    // implementation of routines used internally by KrView
    virtual void setup();
    virtual void setCurrentItemBySpec(ItemSpec spec);
    virtual KIO::filesize_t calcSize();
    virtual KIO::filesize_t calcSelectedSize();
    virtual QRect itemRect(KUrl itemUrl);
    virtual const Item *dummyItem() const;
    virtual Item *itemFromUrl(KUrl url) const;
    virtual void refreshIcons();
    virtual void gotPreview(KFileItem item, QPixmap preview);
    virtual bool isCurrentItemSelected();
    virtual void selectCurrentItem(bool select);
    virtual void pageDown();
    virtual void pageUp();
    virtual void saveSettingsOfType(KConfigGroup grp, KrViewProperties::PropertyType properties);
    virtual void doRestoreSettings(KConfigGroup grp);
    virtual void newItems(const KFileItemList& items);
    virtual void itemsDeleted(const KFileItemList& items);
    virtual void refreshItems(const QList<QPair<KFileItem, KFileItem> >& items);

    QRect itemRect(const QModelIndex &index);

    void currentChanged(const QModelIndex &current);
    void setCurrentIndex(QModelIndex index);
    KrView::Item *currentViewItem();

    KrVfsModel *_model;
    QAbstractItemView *_itemView;
    KrMouseHandler *_mouseHandler;

    QWidget *_parentWidget;
    ViewWidget *_widget;

    bool _isDraggedOver;
    KFileItem _dragAndDropTarget;

private:
    QSet<const Item*> _selection;
};

#endif
