/***************************************************************************
                                krview.h
                             -------------------
    copyright            : (C) 2000-2002 by Shie Erlich & Rafi Yanai
                           (C) 2011 + by Jan Lepper <jan_lepper@gmx.de>
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

#include "abstractview.h"

#include <QtGui/QPixmap>
#include <QtCore/QVariant>
#include <QtCore/QHash>
#include <QtCore/QRegExp>
#include <QList>
#include <QTimer>

#include "../Filter/filtersettings.h"

#include <kdebug.h>

#define MAX_BRIEF_COLS 5

class QKeyEvent;

class KrView;
class KrViewInstance;
class KrPreviews;
class AbstractDirLister;
class KFileItemList;


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
        humanReadableSize()
    {}

    enum PropertyType { NoProperty = 0x0, PropIconSize = 0x1, PropShowPreviews = 0x2,
                        PropSortMode = 0x4, PropColumns = 0x8, PropFilter = 0x10, PropAutoResizeColumns = 0x20,
                        AllProperties = PropIconSize | PropShowPreviews | PropSortMode | PropColumns | PropFilter | PropAutoResizeColumns
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
class KrView : public AbstractView
{
    // instantiating a new view
    // 1. new KrView
    // 2. view->init()
    // notes: constructor does as little as possible, setup() does the rest. esp, note that
    // if you need something from operator or properties, move it into setup()

public:
    class Item;
    class Emitter;
    class Operator;

    class IconSizes : public QVector<int>
    {
    public:
        IconSizes() : QVector<int>() {
            *this << 12 << 16 << 22 << 32 << 48 << 64 << 128 << 256;
        }
    };

    friend class KrView::Operator;

    virtual ~KrView();

    /////////////////////////////////////////////////////////////
    // pure virtual functions                                  //
    /////////////////////////////////////////////////////////////
    virtual void invertSelection() = 0;
    virtual void selectRegion(KUrl item1, KUrl item2, bool select, bool clearFirst = false) = 0;

    /////////////////////////////////////////////////////////////
    // AbstractView implementation                                     //
    /////////////////////////////////////////////////////////////
    virtual ViewType *type();
    virtual AbstractView::Emitter *emitter();
    virtual void init(QWidget *mainWindow, KrQuickSearch *quickSearch, QuickFilter *quickFilter);
    virtual void setDirLister(AbstractDirLister *lister);
    virtual void saveSelection();
    virtual void clearSavedSelection();
    virtual QString statistics();
    virtual void renameCurrentItem();
    virtual void stopQuickFilter(bool refreshView = true);
    virtual KUrl urlToMakeCurrent() const {
        return _urlToMakeCurrent;
    }
    virtual void setUrlToMakeCurrent(KUrl url) {
        _urlToMakeCurrent = url;
    }
    virtual bool isDraggedOver() const = 0;
    virtual KFileItem getDragAndDropTarget() = 0;
    virtual void setDragState(bool isDraggedOver, KFileItem target) = 0;
    virtual void prepareForActive() {
        _focused = true;
    }
    virtual void prepareForPassive();
    virtual void saveSettings(KConfigGroup grp) {
        saveSettingsOfType(grp, KrViewProperties::AllProperties);
    }
    virtual void restoreSettings(KConfigGroup grp);
    virtual void disableSorting() {
        setSortMode(KrViewProperties::NoColumn, false);
    }
    virtual void startQuickFilter();
    virtual void enableUpdateDefaultSettings(bool enable);


    /////////////////////////////////////////////////////////////
    // virtual functions                                       //
    /////////////////////////////////////////////////////////////
    virtual void selectAllIncludingDirs() {
        changeSelection(KRQuery("*"), true, true);
    }
    virtual void showPreviews(bool show);
    virtual bool previewsShown() {
        return _previews != 0;
    }
    virtual void applySettingsToOthers();
    virtual void setSortMode(KrViewProperties::ColumnType sortColumn, bool descending) {
        sortModeUpdated(sortColumn, descending);
    }
    virtual void setFileIconSize(int size);
    // save this view's settings to be restored after restart
    virtual bool handleKeyEvent(QKeyEvent *e);

    /////////////////////////////////////////////////////////////
    // non-virtual functions                                   //
    /////////////////////////////////////////////////////////////

    inline int fileIconSize() const {
        return _fileIconSize;
    }
    inline bool isFocused() const {
        return _focused;
    }

    KrViewInstance *instance() {
        return &_instance;
    }

    // save this view's settings as default for new views of this type
    void saveDefaultSettings(KrViewProperties::PropertyType properties = KrViewProperties::AllProperties);
    // restore the default settings for this view type
    void restoreDefaultSettings();

    void restoreSelection();
    bool canRestoreSelection() {
        return !_savedSelection.isEmpty();
    }

    bool isFiltered(const KFileItem &item) const;
    const KRQuery& filterMask() const {
        return _properties->filterMask;
    }
    KrViewProperties::FilterSpec filter() const {
        return _properties->filter;
    }
    void setFilter(KrViewProperties::FilterSpec filter);
    void setFilter(KrViewProperties::FilterSpec filter, FilterSettings customFilter, bool applyToDirs);

    void customSelection(bool select);
    void selectRegion(KFileItem item1, KFileItem item2, bool select, bool clearFirst = false);
    QString itemDescription(KUrl url, bool itemIsUpUrl);

    const KrViewProperties* properties() const {
        return _properties;
    }
    int defaultFileIconSize() const;
    void setDefaultFileIconSize() {
        setFileIconSize(defaultFileIconSize());
    }
    void zoomIn();
    void zoomOut();

    // lowercase id for the column (untranslated)
    static QString columnId(int column);
    // translated column name
    static QString columnDescription(int column);

    static bool isShowHidden();
    static void showHidden(bool show);
    static bool isGlobalFilterEnabled();
    static bool isGlobalFilterSet();

    static void initGlobal();
    static void refreshAllViews();
    static void enableGlobalFilter(bool enable);
    static void setGlobalFilter();


    /////////////////////////////////////////////////////////////
    // deprecated functions start                              //
    /////////////////////////////////////////////////////////////
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


    static const IconSizes iconSizes;

protected:
    enum ItemSpec { First, Last, Prev, Next, UpUrl };

    KrView(KrViewInstance &instance, KConfig *cfg);

    virtual void setup() = 0;
    virtual void clear() = 0;
    virtual const Item *itemFromUrl(KUrl url) const = 0;
    virtual void copySettingsFrom(KrView *other) = 0;
    virtual void updatePreviews();
    virtual bool quickSearch(const QString &term, int direction) = 0;
    virtual void gotPreview(KFileItem item, QPixmap preview) = 0;
    virtual KIO::filesize_t calcSize() = 0;
    virtual KIO::filesize_t calcSelectedSize() = 0;
    virtual void setCurrentItemBySpec(ItemSpec spec) = 0;
    virtual const Item *dummyItem() const = 0;
    virtual void refreshIcons() = 0;
    virtual void newItems(const KFileItemList& items) = 0;
    virtual void itemsDeleted(const KFileItemList& items) = 0;
    virtual void refreshItems(const QList<QPair<KFileItem, KFileItem> >& items) = 0;
    virtual bool isCurrentItemSelected() = 0;
    virtual void selectCurrentItem(bool select) = 0;
    virtual void pageDown() = 0;
    virtual void pageUp() = 0;
    virtual void redraw() = 0;

    virtual KrViewProperties * createViewProperties() {
        return new KrViewProperties();
    }
    virtual void initProperties();
    virtual Operator *createOperator(KrQuickSearch *quickSearch, QuickFilter *quickFilter);
    virtual void saveSettingsOfType(KConfigGroup grp, KrViewProperties::PropertyType properties);
    virtual void doRestoreSettings(KConfigGroup grp);

    bool quickSearchMatch(const KFileItem &item, QString term);
    bool handleKeyEventInt(QKeyEvent *e);
    void sortModeUpdated(KrViewProperties::ColumnType sortColumn, bool descending);
    void saveSortMode(KConfigGroup &group);
    void restoreSortMode(KConfigGroup &group);
    Operator* op() const {
        return _operator;
    }

    static KRQuery _globalFilter;
    static FilterSettings _globalFilterSettings;

    KrViewInstance &_instance;
    AbstractDirLister *_dirLister;
    KConfig *_config;
    QWidget *_mainWindow;
    KUrl::List _savedSelection;
    KUrl _urlToMakeCurrent;
    QString _nameToMakeCurrentIfAdded;
    KrViewProperties *_properties;
    Operator *_operator;
    Emitter *_emitter;
    bool _focused;
    KrPreviews *_previews;
    int _fileIconSize;
    bool _updateDefaultSettings;
    QRegExp _quickFilterMask;
};

Q_DECLARE_INTERFACE (KrView, "org.krusader.KrView/0.1")


class KrView::Item : public KFileItem
{
public:
    Item();
    Item(const KFileItem &fileItem, KrView *view, bool isDummy = false);
    Item(const Item &other);
    Item &operator=(const Item &other);

    const QString &nameWithoutExtension() const;
    const QString &extension() const;
    const QString &krPermissions() const;
    bool isBrokenLink() const {
        return _brokenLink;
    }
    KIO::filesize_t size() const {
        return _calculatedSize ? _calculatedSize : KFileItem::size();
    }
    void setCalculatedSize(KIO::filesize_t s);
    const QString &sizeString() const;
    const QString &mtimeString() const;
    QString permissionsString() const;
    const QString &iconName() const;
    const QPixmap &icon() const;
    void clearIcon();
    void setIcon(QPixmap icon);

protected:
    void init(KrView *view, bool isDummy);
    QPixmap loadIcon(bool active) const;
    QPixmap processIcon(QPixmap icon, bool active) const;

    mutable QString _nameWithoutExtension;
    mutable QString _extension;
    mutable QString _krPermissions;
    mutable QString _sizeString;
    mutable QString _mtimeString;
    mutable QString _numericPermissions;
    mutable QString _iconName;
    bool _brokenLink;
    KIO::filesize_t _calculatedSize;
    mutable QPixmap _iconActive, _iconInactive;
    KrView *_view;
};


class KrView::Emitter : public AbstractView::Emitter
{
public:
    void emitSelectionChanged() {
        emit selectionChanged();
    }
    void emitCurrentChanged(KFileItem item) {
        emit currentChanged(item);
    }
    void emitGotDrop(QDropEvent *e) {
        emit gotDrop(e);
    }
    void emitDragMove(QDragMoveEvent *e) {
        emit dragMove(e);
    }
    void emitDragLeave(QDragLeaveEvent *e) {
        emit dragLeave(e);
    }
    void emitItemDescription(QString desc) {
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
    void emitHistoryBackward() {
        emit historyBackward();
    }
    void emitHistoryForward() {
        emit historyForward();
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
};


// operator can handle two ways of doing things:
// 1. if the view is a widget (inherits krview and klistview for example)
// 2. if the view HAS A widget (a krview-son has a member of klistview)
// this is done by specifying the view and the widget in the constructor,
// even if they are actually the same object (specify it twice in that case)
class KrView::Operator : public QObject
{
    Q_OBJECT
public:
    Operator(KrView *view, KrQuickSearch *quickSearch, QuickFilter *quickFilter);
    ~Operator();

    virtual bool eventFilter(QObject *watched, QEvent *event);

    KrView *view() const {
        return _view;
    }

    void prepareForPassive();

    KrQuickSearch * quickSearch() {
        return _quickSearch;
    }

    void widgetChanged();
    bool handleKeyEvent(QKeyEvent *e);
    void settingsChanged(KrViewProperties::PropertyType properties);

public slots:
    void quickSearch(const QString &, int = 0);
    void stopQuickSearch(QKeyEvent*);
    void handleQuickSearchEvent(QKeyEvent*);

    void startQuickFilter();
    void stopQuickFilter(bool refreshView = true);

    void startDrag();

protected slots:
    void quickFilterChanged(const QString &text);
    void saveDefaultSettings();
    void startUpdate() {
        _view->refresh();
    }
    void cleared() {
        _view->clear();
    }
    void newItems(const KFileItemList& items) {
        _view->newItems(items);
    }
    void itemsDeleted(const KFileItemList& items) {
        _view->itemsDeleted(items);
    }
    void refreshItems(const QList<QPair<KFileItem, KFileItem> >& items) {
        _view->refreshItems(items);
    }
    void colorSettingsChanged() {
        _view->refreshIcons();
    }
    void gotPreview(KFileItem item, QPixmap preview) {
        _view->gotPreview(item, preview);
    }

    /////////////////////////////////////////////////////////////
    // deprecated functions start                              //
    // can be removed when VFileDirLister is removed           //
    /////////////////////////////////////////////////////////////
protected slots:
    // for signals from vfs' dirwatch
    void fileDeleted(const QString& name);
    void refreshItem(KFileItem item);
    /////////////////////////////////////////////////////////////
    // deprecated functions end                                //
    /////////////////////////////////////////////////////////////


protected:
    KrView *_view;

private:
    KrQuickSearch *_quickSearch;
    QuickFilter *_quickFilter;
    QTimer _saveDefaultSettingsTimer;
    static KrViewProperties::PropertyType _changedProperties;
    static KrView *_changedView;
};

#endif /* KRVIEW_H */
