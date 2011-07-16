/***************************************************************************
                                krview.h
                             -------------------
    copyright            : (C) 2000-2002 by Shie Erlich & Rafi Yanai
    e-mail               : krusader@users.sourceforge.net
    web site             : http://krusader.sourceforge.net
 ---------------------------------------------------------------------------
  Description
 ***************************************************************************

  A

     db   dD d8888b. db    db .d8888.  .d8b.  d8888b. d88888b d8888b.
     88 ,8P' 88  `8D 88    88 88'  YP d8' `8b 88  `8D 88'     88  `8D
     88,8P   88oobY' 88    88 `8bo.   88ooo88 88   88 88ooooo 88oobY'
     88`8b   88`8b   88    88   `Y8b. 88~~~88 88   88 88~~~~~ 88`8b
     88 `88. 88 `88. 88b  d88 db   8D 88   88 88  .8D 88.     88 `88.
     YP   YD 88   YD ~Y8888P' `8888Y' YP   YP Y8888D' Y88888P 88   YD

                                                     H e a d e r    F i l e

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef KRVIEW_H
#define KRVIEW_H

#include <QtGui/QPixmap>
#include <QtCore/QVariant>
#include <QtCore/QHash>
#include <QtCore/QRegExp>
#include <QDropEvent>
#include <QList>
#include <QTimer>
#include <kfileitem.h>
#include "../krglobal.h"
#include "../VFS/krquery.h"
#include "../Filter/filtersettings.h"

#include <kdebug.h>

#define MAX_BRIEF_COLS 5

class KrView;
class KrQuickSearch;
class KrPreviews;
class KrViewInstance;
class QuickFilter;
class AbstractDirLister;
class CalcSpaceClient;

class vfile;


// KrViewProperties
// This class is an interface class between KrView and KrViewItem
// In order for KrViewItem to be as independent as possible, KrView holds
// an instance of this class, and fills it with the correct data. A reference
// to this should be given to each KrViewItem, which then queries it for
// information regarding how things should be displayed in the current view.
//
// Every property that the item needs to know about the view must be here!
class KrViewProperties
{
public:
    KrViewProperties() :
        numericPermissions(false),
        displayIcons(false),
        sortColumn(Name),
        sortOptions(static_cast<SortOptions>(0)),
        sortMethod(Alphabetical),
        filter(KrViewProperties::All),
        filterMask(KRQuery("*")),
        filterApplysToDirs(false),
        localeAwareCompareIsCaseSensitive(false),
        humanReadableSize(),
        numberOfColumns(1)
    {}

    enum PropertyType { NoProperty = 0x0, PropIconSize = 0x1, PropShowPreviews = 0x2,
                        PropSortMode = 0x4, PropColumns = 0x8, PropFilter = 0x10,
                        AllProperties = PropIconSize | PropShowPreviews | PropSortMode | PropColumns | PropFilter
                      };
    enum ColumnType { NoColumn = -1, Name = 0x0, Ext = 0x1, Size = 0x2, Type = 0x3, Modified = 0x4,
                      Permissions = 0x5, KrPermissions = 0x6, Owner = 0x7, Group = 0x8, MAX_COLUMNS = 0x09
                    };
    enum SortOptions { Descending = 0x200, DirsFirst = 0x400, IgnoreCase = 0x800,
                       AlwaysSortDirsByName = 0x1000, LocaleAwareSort = 0x2000
                     };
    enum SortMethod { Alphabetical = 0x1, AlphabeticalNumbers = 0x2,
                      CharacterCode = 0x4, CharacterCodeNumbers = 0x8, Krusader = 0x10
                    };
    enum FilterSpec { Dirs = 0x1, Files = 0x2, All = 0x3, Custom = 0x4 };

    bool numericPermissions;        // show full permission column as octal numbers
    bool displayIcons; // true if icons should be displayed in this view
    ColumnType sortColumn;
    SortOptions sortOptions;
    SortMethod sortMethod;  // sort method for names and extensions
    FilterSpec filter; // what items to show (all, custom, exec)
    KRQuery filterMask; // what items to show (*.cpp, *.h etc)
    FilterSettings filterSettings;
    bool filterApplysToDirs;
    bool localeAwareCompareIsCaseSensitive; // mostly, it is not! depends on LC_COLLATE
    bool humanReadableSize;  // display size as KB, MB or just as a long number
    QStringList atomicExtensions; // list of strings, which will be treated as one extension. Must start with a dot.
    int numberOfColumns;    // the number of columns in the brief view
};

// operator can handle two ways of doing things:
// 1. if the view is a widget (inherits krview and klistview for example)
// 2. if the view HAS A widget (a krview-son has a member of klistview)
// this is done by specifying the view and the widget in the constructor,
// even if they are actually the same object (specify it twice in that case)
class KrViewOperator: public QObject
{
    Q_OBJECT
public:
    KrViewOperator(KrView *view, QWidget *widget);
    ~KrViewOperator();

    virtual bool eventFilter(QObject *watched, QEvent *event);

    KrView *view() const {
        return _view;
    }
    QWidget *widget() const {
        return _widget;
    }
    void startDrag();

    void emitCurrentChanged(KFileItem item) {
        emit currentChanged(item);
    }
    void emitGotDrop(QDropEvent *e) {
        emit gotDrop(e);
    }
    void emitItemDescription(QString &desc) {
        emit itemDescription(desc);
    }
    void emitContextMenu(const QPoint &point) {
        emit contextMenu(point);
    }
    void emitEmptyContextMenu(const QPoint &point) {
        emit emptyContextMenu(point);
    }
    void emitNeedFocus() {
        emit needFocus();
    }
    void emitPreviewJobStarted(KJob *job) {
        emit previewJobStarted(job);
    }
    void emitGoHome() {
        emit goHome();
    }
    void emitDirUp() {
        emit dirUp();
    }
    void emitDeleteFiles(bool reallyDelete) {
        emit deleteFiles(reallyDelete);
    }
    void emitMiddleButtonClicked(KFileItem item, bool itemIsUpUrl) {
        emit middleButtonClicked(item, itemIsUpUrl);
    }
    void emitCalcSpace(KFileItem item) {
        emit calcSpace(item);
    }
    void emitRenameItem(KFileItem item, QString newName) {
        emit renameItem(item, newName);
    }
    void emitExecuted(KFileItem item) {
        emit executed(item);
    }
    void emitGoInside(KFileItem item) {
        emit goInside(item);
    }
    void emitRefreshActions() {
        emit refreshActions();
    }
    void emitLetsDrag(KUrl::List urls, QPixmap icon) {
        emit letsDrag(urls, icon);
    }

    void prepareForPassive();

    void setQuickSearch(KrQuickSearch *quickSearch);
    KrQuickSearch * quickSearch() {
        return _quickSearch;
    }

    void setQuickFilter(QuickFilter*);

    bool handleKeyEvent(QKeyEvent *e);
    void setMassSelectionUpdate(bool upd);
    bool isMassSelectionUpdate() {
        return _massSelectionUpdate;
    }
    void settingsChanged(KrViewProperties::PropertyType properties);

public slots:
    void emitSelectionChanged() {
        if (!_massSelectionUpdate)
            emit selectionChanged();
    }
    void quickSearch(const QString &, int = 0);
    void stopQuickSearch(QKeyEvent*);
    void handleQuickSearchEvent(QKeyEvent*);

    void startQuickFilter();
    void stopQuickFilter(bool refreshView = true);

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
    void itemDescription(QString &desc);
    void contextMenu(const QPoint &point);
    void emptyContextMenu(const QPoint& point);
    void needFocus();
    void previewJobStarted(KJob *job);
    void goHome();
    void deleteFiles(bool reallyDelete);
    void dirUp();
    void refreshActions();

protected slots:
    void quickFilterChanged(const QString &text);
    void saveDefaultSettings();
    void startUpdate();
    void cleared();
    void newItems(const KFileItemList& items);
    void itemsDeleted(const KFileItemList& items);
    void refreshItems(const QList<QPair<KFileItem, KFileItem> >& items);
    void colorSettingsChanged();
    void gotPreview(KFileItem item, QPixmap preview);

    /////////////////////////////////////////////////////////////
    // deprecated functions start                              //
    /////////////////////////////////////////////////////////////
protected slots:
    // for signals from vfs' dirwatch
    void fileDeleted(const QString& name);
    void refreshItem(KFileItem item);
    /////////////////////////////////////////////////////////////
    // deprecated functions end                                //
    /////////////////////////////////////////////////////////////


protected:
    // never delete those
    KrView *_view;
    QWidget *_widget;

private:
    KrQuickSearch *_quickSearch;
    QuickFilter *_quickFilter;
    bool _massSelectionUpdate;
    QTimer _saveDefaultSettingsTimer;
    static KrViewProperties::PropertyType _changedProperties;
    static KrView *_changedView;
};

/****************************************************************************
 * READ THIS FIRST: Using the view
 *
 * You always hold a pointer to KrView, thus you can only use functions declared
 * in this class. If you need something else, either this class is missing something
 * or you are ;-)
 *
 * The functions you'd usually want:
 * 1) getSelectedItems - returns all selected items, or (if none) the current item.
 *    it never returns anything which includes the "..", and thus can return an empty list!
 * 2) getSelectedKrViewItems - the same as (1), but returns a QValueList with KrViewItems
 * 3) getCurrentItem, setCurrentItem - work with QString
 * 4) getFirst, getNext, getPrev, getCurrentKrViewItem - all work with KrViewItems, and
 *    used to iterate through a list of items. note that getNext and getPrev accept a pointer
 *    to the current item (used in detailedview for safe iterating), thus your loop should be:
 *       for (KrViewItem *it = view->getFirst(); it!=0; it = view->getNext(it)) { blah; }
 * 5) nameToMakeCurrent(), setNameToMakeCurrent() - work with QString
 *
 * IMPORTANT NOTE: every one who subclasses this must call initProperties() in the constructor !!!
 */
class KrView
{
    friend class KrViewOperator;

public:
    class IconSizes : public QVector<int>
    {
    public:
        IconSizes() : QVector<int>() {
            *this << 12 << 16 << 22 << 32 << 48 << 64 << 128 << 256;
        }
    };

    class Item : public KFileItem
    {
    public:
        Item();
        Item(const KFileItem &fileItem, KrView *view, bool isDummy = false);
        Item(const Item &other);
        Item &operator=(const Item &other);

        const QString &iconName() const {
            if(_iconName.isNull())
                getIconName();
            return _iconName;
        }
        QString krPermissionsString() const {
            if(_krPermissionsString.isNull())
                getKrPermissionsString();
            return _krPermissionsString;
        }
        bool isBrokenLink() const {
            return _brokenLink;
        }
        KIO::filesize_t size() const {
            return _calculatedSize ? _calculatedSize : KFileItem::size();
        }
        void setCalculatedSize(KIO::filesize_t s) {
            _calculatedSize = s;
        }
        const QPixmap &icon() const {
            if(_iconActive.isNull())
                loadIcon();
            return _view->isFocused() ? _iconActive : _iconInactive;
        }
        void clearIcon();
        void setIcon(QPixmap icon);

    protected:
        void init(KrView *view, bool isDummy);
        void getIconName() const;
        void getKrPermissionsString() const;
        void loadIcon() const;
        QPixmap loadIcon(bool active) const;
        QPixmap processIcon(QPixmap icon, bool active) const;

        mutable QString _iconName;
        mutable QString  _krPermissionsString;
        bool _brokenLink;
        KIO::filesize_t _calculatedSize;
        mutable QPixmap _iconActive, _iconInactive;
        KrView *_view;
    };

    virtual ~KrView();

    // instantiating a new view
    // 1. new KrView
    // 2. view->init()
    // notes: constructor does as little as possible, setup() does the rest. esp, note that
    // if you need something from operator or properties, move it into setup()
    virtual void init();
    KrViewInstance *instance() {
        return &_instance;
    }

    static const IconSizes iconSizes;

protected:
    virtual void initProperties();
    virtual KrViewProperties * createViewProperties() {
        return new KrViewProperties();
    }
    virtual KrViewOperator *createOperator() {
        return new KrViewOperator(this, _widget);
    }
    virtual void setup() = 0;

    ///////////////////////////////////////////////////////
    // Every view must implement the following functions //
    ///////////////////////////////////////////////////////
public:
    virtual CalcSpaceClient *calcSpaceClient() = 0;

    virtual uint count() const = 0;
    virtual uint calcNumDirs() const = 0;
    virtual KFileItemList getItems(KRQuery mask = KRQuery(), bool dirs = true, bool files = true) = 0;
    virtual KFileItemList getSelectedItems(bool currentIfNoSelection) = 0;
    virtual KFileItemList getVisibleItems() = 0;
    virtual void changeSelection(KUrl::List urls, bool select, bool clearFirst) = 0;
    virtual void changeSelection(const KRQuery& filter, bool select, bool includeDirs) = 0;
    virtual void invertSelection() = 0;
    virtual bool isItemSelected(KUrl url) = 0;
    // first item after ".."
    virtual KFileItem firstItem() = 0;
    virtual KFileItem lastItem() = 0;
    virtual KFileItem currentItem() = 0;
    // indicates that ".." is the current item
    // in this case currentItem() return a null item
    virtual bool currentItemIsUpUrl() = 0;
    virtual void setCurrentItem(KUrl url) = 0;
    virtual bool isItemVisible(KUrl url) = 0;
    virtual QRect itemRectGlobal(KUrl url) = 0;
    virtual void makeItemVisible(KUrl url) = 0;
    virtual void selectRegion(KUrl item1, KUrl item2, bool select, bool clearFirst = false) = 0;
    virtual KFileItem itemAt(const QPoint &pos, bool *isUpUrl = 0, bool includingUpUrl = false) = 0;
    virtual QPixmap icon(KUrl url) = 0;
    virtual bool isCurrentItemSelected() = 0;
    virtual void selectCurrentItem(bool select) = 0;
    virtual void pageDown() = 0;
    virtual void pageUp() = 0;
    virtual QString currentDescription() = 0;
    virtual void makeCurrentVisible() = 0;
    virtual uint numSelected() const = 0;
    virtual void updateView() = 0;
    virtual void sort() = 0;
    virtual void refreshColors() = 0;
    virtual void redraw() = 0;
    virtual bool handleKeyEvent(QKeyEvent *e);
    virtual void prepareForActive() {
        _focused = true;
    }
    virtual void prepareForPassive() {
        _focused = false;
        _operator->prepareForPassive();
    }
    virtual void renameCurrentItem(); // Rename current item. returns immediately
    virtual int  itemsPerPage() {
        return 0;
    }
    virtual void showContextMenu() = 0;

protected:
    virtual const Item *itemFromUrl(KUrl url) const = 0;
    virtual void copySettingsFrom(KrView *other) = 0;
    virtual void intSetSelected(const Item *item, bool select) = 0;
    virtual void updatePreviews();
    virtual void clear();
    virtual void populate(const KFileItemList &items, bool addDummyItem) = 0;
    virtual bool quickSearch(const QString &term, int direction) = 0;
    virtual void gotPreview(KFileItem item, QPixmap preview) = 0;

public:
    //////////////////////////////////////////////////////
    // the following functions are already implemented, //
    // and normally - should NOT be re-implemented.     //
    //////////////////////////////////////////////////////
    virtual void selectAllIncludingDirs() {
        changeSelection(KRQuery("*"), true, true);
    }
    virtual void select(const KRQuery& filter = KRQuery("*")) {
        changeSelection(filter, true);
    }
    virtual void unselect(const KRQuery& filter = KRQuery("*")) {
        changeSelection(filter, false);
    }
    virtual QString statistics();
    virtual const KrViewProperties* properties() const {
        return _properties;
    }
    virtual KrViewOperator* op() const {
        return _operator;
    }
    virtual void showPreviews(bool show);
    virtual bool previewsShown() {
        return _previews != 0;
    }
    virtual void applySettingsToOthers();

    virtual void setDirLister(AbstractDirLister *lister);
    virtual void refresh();

    void changeSelection(const KRQuery& filter, bool select);
    void enableUpdateDefaultSettings(bool enable);
    KUrl currentUrl() {
        return currentItem().isNull() ? KUrl() : currentItem().url();
    }
    void setCurrentItem(KFileItem item) {
        setCurrentItem(item.isNull() ? KUrl() : item.url());
    }
    KUrl::List getSelectedUrls(bool currentUrlIfNoSelection) {
        return getSelectedItems(currentUrlIfNoSelection).urlList();
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
    void selectRegion(KFileItem item1, KFileItem item2, bool select, bool clearFirst = false);
    QString itemDescription(KUrl url, bool itemIsUpUrl);

    KUrl urlToMakeCurrent() const {
        return _urlToMakeCurrent;
    }
    void setUrlToMakeCurrent(KUrl url) {
        _urlToMakeCurrent = url;
    }

    /////////////////////////////////////////////////////////////
    // the following functions have a default and minimalistic //
    // implementation, and may be re-implemented if needed     //
    /////////////////////////////////////////////////////////////
    virtual void setSortMode(KrViewProperties::ColumnType sortColumn, bool descending) {
        sortModeUpdated(sortColumn, descending);
    }
    virtual const KRQuery& filterMask() const {
        return _properties->filterMask;
    }
    virtual KrViewProperties::FilterSpec filter() const {
        return _properties->filter;
    }
    virtual void setFilter(KrViewProperties::FilterSpec filter);
    virtual void setFilter(KrViewProperties::FilterSpec filter, FilterSettings customFilter, bool applyToDirs);
    virtual void customSelection(bool select);
    virtual int defaultFileIconSize();
    virtual void setFileIconSize(int size);
    virtual void setDefaultFileIconSize() {
        setFileIconSize(defaultFileIconSize());
    }
    virtual void zoomIn();
    virtual void zoomOut();

    // save this view's settings to be restored after restart
    virtual void saveSettings(KConfigGroup grp,
        KrViewProperties::PropertyType properties = KrViewProperties::AllProperties);

    inline QWidget *widget() {
        return _widget;
    }
    inline int fileIconSize() const {
        return _fileIconSize;
    }
    inline bool isFocused() const {
        return _focused;
    }

    void setMainWindow(QWidget *mainWindow) {
        _mainWindow = mainWindow;
    }

    // save this view's settings as default for new views of this type
    void saveDefaultSettings(KrViewProperties::PropertyType properties = KrViewProperties::AllProperties);
    // restore the default settings for this view type
    void restoreDefaultSettings();
    // call this to restore this view's settings after restart
    void restoreSettings(KConfigGroup grp);

    void saveSelection();
    void restoreSelection();
    bool canRestoreSelection() {
        return !_savedSelection.isEmpty();
    }
    void clearSavedSelection();

    /////////////////////////////////////////////////////////////
    // deprecated functions start                              //
    /////////////////////////////////////////////////////////////
public:
    virtual QString getCurrentItem() const = 0;

    //the following can be removed when VFileDirLister is removed
    virtual QString nameToMakeCurrentIfAdded() const {
        return _nameToMakeCurrentIfAdded;
    }
    virtual void setNameToMakeCurrentIfAdded(const QString name) {
        _nameToMakeCurrentIfAdded = name;
    }
    /////////////////////////////////////////////////////////////
    // deprecated functions end                                //
    /////////////////////////////////////////////////////////////

protected:
    enum ItemSpec { First, Last, Prev, Next, UpUrl };

    KrView(KrViewInstance &instance, KConfig *cfg);

    virtual void doRestoreSettings(KConfigGroup grp);
    virtual KIO::filesize_t calcSize() = 0;
    virtual KIO::filesize_t calcSelectedSize() = 0;
    virtual void setCurrentItem(ItemSpec item) = 0;
    virtual const Item *dummyItem() const = 0;
    virtual void refreshIcons() = 0;

    virtual void newItems(const KFileItemList& items) = 0;
    virtual void itemsDeleted(const KFileItemList& items) = 0;
    virtual void refreshItems(const QList<QPair<KFileItem, KFileItem> >& items) = 0;

    bool isFiltered(const KFileItem &item);
    bool quickSearchMatch(const KFileItem &item, QString term);
    void setSelected(const Item *item, bool select);
    bool handleKeyEventInt(QKeyEvent *e);
    void sortModeUpdated(KrViewProperties::ColumnType sortColumn, bool descending);
    void saveSortMode(KConfigGroup &group);
    void restoreSortMode(KConfigGroup &group);
    inline void setWidget(QWidget *w) {
        _widget = w;
    }

    KrViewInstance &_instance;
    AbstractDirLister *_dirLister;
    KConfig *_config;
    QWidget *_mainWindow;
    QWidget *_widget;
    KUrl::List _savedSelection;
    KUrl _urlToMakeCurrent;
    QString _nameToMakeCurrentIfAdded;
    KrViewProperties *_properties;
    KrViewOperator *_operator;
    bool _focused;
    KrPreviews *_previews;
    int _fileIconSize;
    bool _updateDefaultSettings;
    QRegExp _quickFilterMask;
};


#endif /* KRVIEW_H */
