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

#ifndef __ABSTRACTVIEW_H__
#define __ABSTRACTVIEW_H__

#include "../defaults.h"
#include "../krglobal.h"
#include "../VFS/krquery.h"

#include <kfileitem.h>


class QDropEvent;
class QDragLeaveEvent;
class QDragMoveEvent;

class CalcSpaceClient;
class KrQuickSearch;
class QuickFilter;
class AbstractDirLister;
class ViewType;


class AbstractView
{
public:
    class Emitter;

    virtual ~AbstractView() {}

    /////////////////////////////////////////////////////////////
    // pure virtual functions                                  //
    /////////////////////////////////////////////////////////////
    virtual ViewType *type() = 0;
    virtual QObject *self() = 0;
    virtual Emitter *emitter() = 0;
    virtual CalcSpaceClient *calcSpaceClient() = 0;
    virtual void init(QWidget *mainWindow, KrQuickSearch *quickSearch, QuickFilter *quickFilter) = 0;
    virtual void setDirLister(AbstractDirLister *lister) = 0;
    virtual void refresh() = 0;
    virtual uint count() const = 0;
    virtual uint calcNumDirs() const = 0;
    virtual KFileItemList getItems(KRQuery mask = KRQuery(), bool dirs = true, bool files = true) = 0;
    virtual KFileItemList getSelectedItems(bool currentIfNoSelection) = 0;
    virtual KFileItemList getVisibleItems() = 0;
    virtual void changeSelection(KUrl::List urls, bool select, bool clearFirst) = 0;
    virtual void changeSelection(const KRQuery& filter, bool select,
                                 bool includeDirs = includeDirsInSelection()) = 0;
    virtual bool isItemSelected(KUrl url) = 0;
    virtual void saveSelection() = 0;
    virtual void clearSavedSelection() = 0;
    virtual bool isDraggedOver() const = 0;
    virtual KFileItem getDragAndDropTarget() = 0;
    virtual void setDragState(bool isDraggedOver, KFileItem target) = 0;
    // first item after ".."
    virtual KFileItem firstItem() = 0;
    virtual KFileItem lastItem() = 0;
    virtual KFileItem currentItem() = 0;
    // indicates that ".." is the current item
    // in this case currentItem() returns a null item
    virtual bool currentItemIsUpUrl() = 0;
    virtual void setCurrentItem(KUrl url) = 0;
    virtual bool isItemVisible(KUrl url) = 0;
    virtual QRect itemRectGlobal(KUrl url) = 0;
    virtual void makeItemVisible(KUrl url) = 0;
    virtual KFileItem itemAt(const QPoint &pos, bool *isUpUrl = 0, bool includingUpUrl = false) = 0;
    virtual bool itemIsUpUrl(KFileItem item) = 0;
    virtual QPixmap icon(KUrl url) = 0;
    virtual QString currentDescription() = 0;
    virtual void makeCurrentVisible() = 0;
    virtual uint numSelected() const = 0;
    virtual void refreshColors() = 0;
    virtual void showContextMenu() = 0;
    virtual QWidget *widget() = 0;
    virtual QString statistics() = 0;
    virtual void renameCurrentItem() = 0; // Rename current item. returns immediately
    virtual KUrl urlToMakeCurrent() const = 0;
    virtual void setUrlToMakeCurrent(KUrl url) = 0;
    virtual void stopQuickFilter(bool refreshView = true) = 0;
    virtual void prepareForActive() = 0;
    virtual void prepareForPassive() = 0;
    virtual void saveSettings(KConfigGroup grp) = 0;
    // call this to restore this view's settings after restart
    virtual void restoreSettings(KConfigGroup grp) = 0;
    virtual void disableSorting() = 0;
    virtual void startQuickFilter() = 0;
    virtual void enableUpdateDefaultSettings(bool enable) = 0;

    /////////////////////////////////////////////////////////////
    // helper functions                                        //
    /////////////////////////////////////////////////////////////
    KUrl currentUrl() {
        return currentItem().isNull() ? KUrl() : currentItem().url();
    }
    void setCurrentItem(KFileItem item) {
        setCurrentItem(item.isNull() ? KUrl() : item.url());
    }
    KUrl::List getSelectedUrls(bool currentUrlIfNoSelection) {
        return getSelectedItems(currentUrlIfNoSelection).urlList();
    }
    void select(const KRQuery& filter = KRQuery("*")) {
        changeSelection(filter, true);
    }
    void unselect(const KRQuery& filter = KRQuery("*")) {
        changeSelection(filter, false);
    }
    void setSelection(KUrl::List urls) {
        changeSelection(urls, true, true);
    }
    void setSelection(KUrl url) {
        setSelection(KUrl::List(url));
    }
    void setSelection(KFileItem item) {
        setSelection(item.isNull() ? KUrl() : item.url());
    }
    void selectItem(KFileItem item, bool select) {
        if(!item.isNull())
            selectItem(item.url(), select);
    }
    void selectItem(KUrl url, bool select) {
        changeSelection(KUrl::List(url), select, false);
    }
    void toggleSelected(KUrl url) {
        selectItem(url, !isItemSelected(url));
    }
    void toggleSelected(KFileItem item) {
        selectItem(item, !isItemSelected(item));
    }
    bool isItemSelected(KFileItem item) {
        return item.isNull() ? false : isItemSelected(item.url());
    }

    static bool includeDirsInSelection() {
        return KConfigGroup(KrGlobal::config, "Look&Feel").readEntry("Mark Dirs", _MarkDirs);
    }


    /////////////////////////////////////////////////////////////
    // deprecated functions start                              //
    /////////////////////////////////////////////////////////////
    virtual QString getCurrentItem() const = 0;
    //the following can be removed when VFileDirLister is removed
    virtual QString nameToMakeCurrentIfAdded() const = 0;
    virtual void setNameToMakeCurrentIfAdded(const QString name) = 0;
    /////////////////////////////////////////////////////////////
    // deprecated functions end                                //
    /////////////////////////////////////////////////////////////
};


class AbstractView::Emitter : public QObject
{
    Q_OBJECT

signals:
    void currentChanged(KFileItem item);
    void renameItem(KFileItem item, QString newName);
    void executed(KFileItem item);
    void goInside(KFileItem item);
    void middleButtonClicked(KFileItem item, bool itemIsUpUrl);
    void calcSpace(KFileItem item);
    void selectionChanged();
    void gotDrop(QDropEvent *e);
    void letsDrag(KUrl::List urls, QPixmap icon);
    void dragMove(QDragMoveEvent *e);
    void dragLeave(QDragLeaveEvent *e);
    void itemDescription(QString desc);
    void contextMenu(const QPoint &point);
    void emptyContextMenu(const QPoint& point);
    void needFocus();
    void previewJobStarted(KJob *job);
    void goHome();
    void deleteFiles(bool reallyDelete);
    void dirUp();
    void refreshActions();
};

#endif // __ABSTRACTVIEW_H__
