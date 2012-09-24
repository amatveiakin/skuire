/***************************************************************************
                         listpanel.cpp
                      -------------------
copyright            : (C) 2000 by Shie Erlich & Rafi Yanai
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

                                              S o u r c e    F i l e

***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "listpanel.h"

#include <unistd.h>
#include <sys/param.h>

#include <QtGui/QBitmap>
#include <QtCore/QStringList>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QList>
#include <QPixmap>
#include <QFrame>
#include <QDropEvent>
#include <QHideEvent>
#include <QEvent>
#include <QShowEvent>
#include <QDrag>
#include <QMimeData>
#include <QtCore/QTimer>
#include <QtCore/QRegExp>
#include <QtGui/QSplitter>
#include <QtGui/QImage>
#include <qtabbar.h>

#include <kmenu.h>
#include <kdiskfreespace.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kiconloader.h>
#include <kcursor.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <kdeversion.h>
#include <kdebug.h>
#include <kmountpoint.h>
#include <kcolorscheme.h>

//#ifdef __LIBKONQ__
//#include <konq_popupmenu.h>
//#include <kbookmarkmanager.h>
//#endif

#include "../defaults.h"
#include "../krusader.h"
#include "../krslots.h"
#include "../kicons.h"
#include "../resources.h"
#include "../krusaderview.h"
#include "../krservices.h"
#include "../VFS/krpermhandler.h"
#include "../VFS/abstractdirlister.h"
#include "../VFS/vfiledirlister.h"
#include "../MountMan/kmountman.h"
#include "../BookMan/krbookmarkbutton.h"
#include "../Dialogs/krdialogs.h"
#include "../Dialogs/krspwidgets.h"
#include "../Dialogs/krspecialwidgets.h"
#include "../Dialogs/percentalsplitter.h"
#include "../Dialogs/popularurls.h"
#include "../GUI/kcmdline.h"
#include "../GUI/dirhistorybutton.h"
#include "../GUI/mediabutton.h"
#include "../GUI/syncbrowsebutton.h"
#include "../UserAction/useractionpopupmenu.h"
#include "viewtype.h"
#include "viewfactory.h"

#include "listpanelactions.h"
#include "viewactions.h"
#include "krpreviewpopup.h"
#include "panelpopup.h"
#include "panelfunc.h"
#include "krpopupmenu.h"
#include "krcolorcache.h"
#include "krerrordisplay.h"
#include "krlayoutfactory.h"
#include "quickfilter.h"
#include "dirhistoryqueue.h"
#include "urlrequester.h"
#include "abstractview.h"


class ListPanel::ActionButton : public QToolButton
{
public:
    ActionButton(QWidget *parent, ListPanel *panel, QString actionName, QString text = QString()) :
            QToolButton(parent),  panel(panel), action(0) {
        setText(text);
        setAutoRaise(true);
        action = panel->manager()->mainWindow()->action(actionName);
        if (action) {
            if(KConfigGroup(krConfig, "ListPanelButtons").readEntry("Icons", false) || text.isEmpty())
                setIcon(action->icon());
            setToolTip(action->toolTip());
        } else
            kDebug()<<"no such action:"<<actionName;
    }

protected:
    virtual void mousePressEvent(QMouseEvent *) {
        panel->slotFocusOnMe();
        if (action)
            action->trigger();
    }

    ListPanel *panel;
    QAction *action;
};


/////////////////////////////////////////////////////
//      The list panel constructor       //
/////////////////////////////////////////////////////
ListPanel::ListPanel(QWidget *parent, AbstractPanelManager *manager, 
                     CurrentViewCallback *currentViewCb, KConfigGroup cfg) :
        AbstractListPanel(parent, manager),
        panelType(-1), colorMask(255), compareMode(false), statsAgent(0),
        previewJob(0), inlineRefreshJob(0), _currentViewCb(currentViewCb),
        quickSearch(0), cdRootButton(0), cdUpButton(0),
        popupBtn(0), popup(0), vfsError(0), _locked(false)
{
    if(cfg.isValid())
        panelType = cfg.readEntry("Type", -1);
    if (panelType == -1)
        panelType = defaultPanelType();
    func = new ListPanelFunc(this);

    setAcceptDrops(true);

    QHash<QString, QWidget*> widgets;

#define ADD_WIDGET(widget) widgets.insert(#widget, widget);

    // media button
    mediaButton = new MediaButton(this);
    connect(mediaButton, SIGNAL(aboutToShow()), this, SLOT(slotFocusOnMe()));
    connect(mediaButton, SIGNAL(openUrl(const KUrl&)), func, SLOT(openUrl(const KUrl&)));
    connect(mediaButton, SIGNAL(newTab(const KUrl&)), SLOT(newTab(const KUrl&)));
    ADD_WIDGET(mediaButton);

    // status bar
    status = new KrSqueezedTextLabel(this);
    KConfigGroup group(krConfig, "Look&Feel");
    status->setFont(group.readEntry("Filelist Font", _FilelistFont));
    status->setAutoFillBackground(false);
    status->setText("");          // needed for initialization code!
    status->setWhatsThis(i18n("The statusbar displays information about the filesystem "
                              "which holds your current directory: total size, free space, "
                              "type of filesystem, etc."));
    ADD_WIDGET(status);

    // back button
    backButton = new ActionButton(this, this, KStandardAction::name(KStandardAction::Back));
    ADD_WIDGET(backButton);

    // forward button
    forwardButton = new ActionButton(this, this, KStandardAction::name(KStandardAction::Back));
    ADD_WIDGET(forwardButton);


    // ... create the history button
    historyButton = new DirHistoryButton(func->history, this);
    connect(historyButton, SIGNAL(aboutToShow()), this, SLOT(slotFocusOnMe()));
    connect(historyButton, SIGNAL(gotoPos(int)), func, SLOT(historyGotoPos(int)));
    ADD_WIDGET(historyButton);

    // bookmarks button
    bookmarksButton = new KrBookmarkButton(this);
    connect(bookmarksButton, SIGNAL(aboutToShow()), this, SLOT(slotFocusOnMe()));
    connect(bookmarksButton, SIGNAL(openUrl(const KUrl&)), func, SLOT(openUrl(const KUrl&)));
    bookmarksButton->setWhatsThis(i18n("Open menu with bookmarks. You can also add "
                                       "current location to the list, edit bookmarks "
                                       "or add subfolder to the list."));
    ADD_WIDGET(bookmarksButton);

    // origin input field
    QuickNavLineEdit *qnle = new QuickNavLineEdit(this);
    origin = new UrlRequester(qnle, this);
    origin->setWhatsThis(i18n("Use superb KDE file dialog to choose location."));
    origin->lineEdit() ->setUrlDropsEnabled(true);
    origin->lineEdit() ->installEventFilter(this);
    origin->lineEdit()->setWhatsThis(i18n("Name of directory where you are. You can also "
                                          "enter name of desired location to move there. "
                                          "Use of Net protocols like ftp or fish is possible."));
    origin->setMode(KFile::Directory | KFile::ExistingOnly);
    connect(origin, SIGNAL(returnPressed(const QString&)), func, SLOT(urlEntered(const QString&)));
    connect(origin, SIGNAL(returnPressed(const QString&)), this, SLOT(slotFocusOnMe()));
    connect(origin, SIGNAL(urlSelected(const KUrl &)), func, SLOT(urlEntered(const KUrl &)));
    connect(origin, SIGNAL(urlSelected(const KUrl &)), this, SLOT(slotFocusOnMe()));
    ADD_WIDGET(origin);

    // toolbar
    QWidget * toolbar = new QWidget(this);
    QHBoxLayout * toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(0);
    ADD_WIDGET(toolbar);

    vfsError = new KrErrorDisplay(this);
    vfsError->setWordWrap(true);
    vfsError->hide();
    ADD_WIDGET(vfsError);

    // client area
    clientArea = new QWidget(this);
    QVBoxLayout *clientLayout = new QVBoxLayout(clientArea);
    clientLayout->setSpacing(0);
    clientLayout->setContentsMargins(0, 0, 0, 0);
    ADD_WIDGET(clientArea);

    // totals label
    totals = new KrSqueezedTextLabel(this);
    totals->setFont(group.readEntry("Filelist Font", _FilelistFont));
    totals->setAutoFillBackground(false);
    totals->setWhatsThis(i18n("The totals bar shows how many files exist, "
                              "how many selected and the bytes math"));
    ADD_WIDGET(totals);

    // free space label
    freeSpace = new KrSqueezedTextLabel(this);
    freeSpace->setFont(group.readEntry("Filelist Font", _FilelistFont));
    freeSpace->setAutoFillBackground(false);
    freeSpace->setText("");
    freeSpace->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ADD_WIDGET(freeSpace);

    // progress indicator for the preview job
    previewProgress = new QProgressBar(this);
    previewProgress->hide();
    ADD_WIDGET(previewProgress);

    // a cancel button for the inplace refresh mechanism
    inlineRefreshCancelButton = new QToolButton(this);
    inlineRefreshCancelButton->hide();
    inlineRefreshCancelButton->setIcon(krLoader->loadIcon("dialog-cancel", KIconLoader::Toolbar, 16));
    connect(inlineRefreshCancelButton, SIGNAL(clicked()), this, SLOT(inlineRefreshCancel()));
    ADD_WIDGET(inlineRefreshCancelButton);

    // a quick button to open the popup panel
    popupBtn = new QToolButton(this);
    popupBtn->setAutoRaise(true);
    popupBtn->setIcon(krLoader->loadIcon("arrow-up", KIconLoader::Toolbar, 16));
    connect(popupBtn, SIGNAL(clicked()), this, SLOT(togglePanelPopup()));
    popupBtn->setToolTip(i18n("Open the popup panel"));
    ADD_WIDGET(popupBtn);

#undef ADD_WIDGET

    // toolbar buttons

    cdOtherButton = new ActionButton(toolbar, this, "cd to other panel", "=");
    cdOtherButton->setFixedSize(20, origin->button()->height());
    toolbarLayout->addWidget(cdOtherButton);

    cdUpButton = new ActionButton(toolbar, this, KStandardAction::name(KStandardAction::Up), "..");
    toolbarLayout->addWidget(cdUpButton);

    cdHomeButton = new ActionButton(toolbar, this, KStandardAction::name(KStandardAction::Home), "~");
    toolbarLayout->addWidget(cdHomeButton);

    cdRootButton = new ActionButton(toolbar, this, "root", "/");
    toolbarLayout->addWidget(cdRootButton);

    // ... creates the button for sync-browsing
    syncBrowseButton = new SyncBrowseButton(toolbar);
    syncBrowseButton->setAutoRaise(true);
    toolbarLayout->addWidget(syncBrowseButton);

    setButtons();

    // create a splitter to hold the view and the popup
    splt = new PercentalSplitter(clientArea);
    splt->setChildrenCollapsible(true);
    splt->setOrientation(Qt::Vertical);
    clientLayout->addWidget(splt);

    // quicksearch
    quickSearch = new KrQuickSearch(clientArea);
    quickSearch->setFont(group.readEntry("Filelist Font", _FilelistFont));
    quickSearch->hide();
    // quickfilter
    quickFilter = new QuickFilter(clientArea);
    quickFilter->hide();

    if(group.readEntry("Quicksearch Position", "bottom") == "top") {
        clientLayout->insertWidget(0, quickSearch);
        clientLayout->insertWidget(0, quickFilter);
    } else {
        clientLayout->insertWidget(-1, quickSearch);
        clientLayout->insertWidget(-1, quickFilter);
    }

    // view
    createView();

    // create the layout
    KrLayoutFactory fact(this, widgets);
    QLayout *layout = fact.createLayout();

    if(!layout) { // fallback: create a layout by ourself
        QVBoxLayout *v = new QVBoxLayout;
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);

        QHBoxLayout *h = new QHBoxLayout;
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(0);
        h->addWidget(origin);
        h->addWidget(toolbar);
        v->addLayout(h);

        h = new QHBoxLayout;
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(0);
        h->addWidget(mediaButton);
        h->addWidget(status);
        h->addWidget(backButton);
        h->addWidget(forwardButton);
        h->addWidget(historyButton);
        h->addWidget(bookmarksButton);
        v->addLayout(h);

        v->addWidget(vfsError);
        v->addWidget(clientArea);

        h = new QHBoxLayout;
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(0);
        h->addWidget(totals);
        h->addWidget(freeSpace);
        h->addWidget(previewProgress);
        h->addWidget(inlineRefreshCancelButton);
        h->addWidget(popupBtn);
        v->addLayout(h);

        layout = v;
    }

    setLayout(layout);

    connect(&KrColorCache::getColorCache(), SIGNAL(colorsRefreshed()), this, SLOT(refreshColors()));
    connect(krApp, SIGNAL(shutdown()), SLOT(inlineRefreshCancel()));
}

ListPanel::~ListPanel()
{
    inlineRefreshCancel();
    // otherwise eventFilter() might be called during destruction and crash
    _view->widget()->removeEventFilter(this);
    delete _view;
    _view = 0;
    delete func;
    delete status;
    delete bookmarksButton;
    delete totals;
    delete quickSearch;
    delete origin;
    delete cdRootButton;
    delete cdHomeButton;
    delete cdUpButton;
    delete cdOtherButton;
    delete syncBrowseButton;
//     delete layout;
}

KUrl ListPanel::url() const
{
    return func->files()->vfs_getOrigin();
}

AbstractDirLister *ListPanel::dirLister()
{
    return func->dirLister;
}

void ListPanel::reparent(QWidget *parent, AbstractPanelManager *manager)
{
    setParent(parent);
    _manager = manager;
}

int ListPanel::defaultPanelType()
{
    KConfigGroup group(krConfig, "Look&Feel");
    return group.readEntry("Default Panel Type", ViewFactory::self()->defaultViewId());
}

void ListPanel::createView()
{
    _view = ViewFactory::self()->createView(panelType, splt, krConfig, krApp, quickSearch, quickFilter);

    // ViewFactory may create a different view type than requested
    panelType = _view->type()->id();

    if(isActive())
        _view->prepareForActive();
    else
        _view->prepareForPassive();
    _view->refreshColors();

    splt->insertWidget(0, _view->widget());

    _view->widget()->installEventFilter(this);

    connect(_view->emitter(), SIGNAL(calcSpace(KFileItem)), func, SLOT(calcSpace(KFileItem)));
    connect(_view->emitter(), SIGNAL(goHome()), func, SLOT(home()));
    connect(_view->emitter(), SIGNAL(dirUp()), func, SLOT(dirUp()));
    connect(_view->emitter(), SIGNAL(deleteFiles(bool)), func, SLOT(deleteFiles(bool)));
    connect(_view->emitter(), SIGNAL(middleButtonClicked(KFileItem, bool)), SLOT(newTab(KFileItem, bool)));
    connect(_view->emitter(), SIGNAL(currentChanged(KFileItem)), SLOT(updatePopupPanel(KFileItem)));
    connect(_view->emitter(), SIGNAL(renameItem(KFileItem, QString)),
            func, SLOT(rename(KFileItem, QString)));
    connect(_view->emitter(), SIGNAL(executed(KFileItem)), func, SLOT(execute(KFileItem)));
    connect(_view->emitter(), SIGNAL(goInside(KFileItem)), func, SLOT(goInside(KFileItem)));
    connect(_view->emitter(), SIGNAL(needFocus()), this, SLOT(slotFocusOnMe()));
    connect(_view->emitter(), SIGNAL(selectionChanged()), this, SLOT(slotUpdateTotals()));
    connect(_view->emitter(), SIGNAL(itemDescription(QString)), krApp, SLOT(statusBarUpdate(QString)));
    connect(_view->emitter(), SIGNAL(contextMenu(const QPoint &)), this, SLOT(popRightClickMenu(const QPoint &)));
    connect(_view->emitter(), SIGNAL(emptyContextMenu(const QPoint &)),
            this, SLOT(popEmptyRightClickMenu(const QPoint &)));
    connect(_view->emitter(), SIGNAL(letsDrag(KUrl::List, QPixmap)), this, SLOT(startDragging(KUrl::List, QPixmap)));
    connect(_view->emitter(), SIGNAL(gotDrop(QDropEvent *)), this, SLOT(handleDropOnView(QDropEvent *)));
    connect(_view->emitter(), SIGNAL(previewJobStarted(KJob*)), this, SLOT(slotPreviewJobStarted(KJob*)));
    connect(_view->emitter(), SIGNAL(currentChanged(KFileItem)), func->history, SLOT(saveCurrentItem()));

    _view->setDirLister(func->dirLister);

    _currentViewCb->onViewCreated(_view);
    _currentViewCb->onCurrentViewChanged(_view);
}

void ListPanel::changeType(int type)
{
    if (panelType != type) {
        KFileItem current = _view->currentItem();
        KUrl::List selection = _view->getSelectedUrls(false);
//FIXME
//         bool filterApplysToDirs = _view->properties()->filterApplysToDirs;
//         KrViewProperties::FilterSpec filter = _view->filter();
//         FilterSettings filterSettings = _view->properties()->filterSettings;

        quickSearch->setFocusProxy(0);

        // otherwise eventFilter() might be called during destruction and crash
        _view->widget()->removeEventFilter(this);
        delete _view;

        panelType = type;

        createView();
//FIXME
//         _view->setFilter(filter, filterSettings, filterApplysToDirs);
        _view->refresh();
        _view->setSelection(selection);
        _view->setCurrentItem(current);
        _view->makeItemVisible(_view->currentUrl());
    }
}

int ListPanel::getProperties()
{
    int props = 0;
    if (syncBrowseButton->state() == SYNCBROWSE_CD)
        props |= PROP_SYNC_BUTTON_ON;
    if (_locked)
        props |= PROP_LOCKED;
    return props;
}

void ListPanel::setProperties(int prop)
{
    syncBrowseButton->setChecked(prop & PROP_SYNC_BUTTON_ON);
    _locked = (prop & PROP_LOCKED);
}

bool ListPanel::eventFilter(QObject * watched, QEvent * e)
{
    if(_view && watched == _view->widget()) {
        if(e->type() == QEvent::FocusIn && !isActive() && !isHidden())
            slotFocusOnMe();
        else if(e->type() == QEvent::ShortcutOverride) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if(ke->key() == Qt::Key_Escape && ke->modifiers() == Qt::NoModifier) {
                // if the cancel refresh action has no shortcut assigned,
                // we need this event ourselves to cancel refresh
                QAction *actCancelRefresh = _manager->mainWindow()->action("cancel refresh");
                if (!actCancelRefresh || actCancelRefresh->shortcut().isEmpty()) {
                    e->accept();
                    return true;
                }
            }
        }
    }
    else if(origin->lineEdit() == watched && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = (QKeyEvent *)e;
        if ((ke->key() ==  Qt::Key_Down) && (ke->modifiers() == Qt::ControlModifier)) {
            slotFocusOnMe();
            return true;
        }
        else if ((ke->key() ==  Qt::Key_Escape) && (ke->modifiers() == Qt::NoModifier)) {
            slotFocusOnMe();
            return true;
        }
    }
    return false;
}

void ListPanel::togglePanelPopup()
{
    if(!popup) {
        popup = new PanelPopup(splt, isLeft(), krApp);
        connect(popup, SIGNAL(selection(const KUrl&)), SLOTS, SLOT(refresh(const KUrl&)));
        connect(popup, SIGNAL(hideMe()), this, SLOT(togglePanelPopup()));
    }

    if (popup->isHidden()) {
        if (popupSizes.count() > 0) {
            dynamic_cast<QSplitter*>(popup->parent())->setSizes(popupSizes);
        } else { // on the first time, resize to 50%
            QList<int> lst;
            lst << height() / 2 << height() / 2;
            dynamic_cast<QSplitter*>(popup->parent())->setSizes(lst);
        }

        popup->show();
        popupBtn->setIcon(krLoader->loadIcon("arrow-down", KIconLoader::Toolbar, 16));
        popupBtn->setToolTip(i18n("Close the popup panel"));
    } else {
        popupSizes.clear();
        popupSizes = dynamic_cast<QSplitter*>(popup->parent())->sizes();
        popup->hide();
        popupBtn->setIcon(krLoader->loadIcon("arrow-up", KIconLoader::Toolbar, 16));
        popupBtn->setToolTip(i18n("Open the popup panel"));

        QList<int> lst;
        lst << height() << 0;
        dynamic_cast<QSplitter*>(popup->parent())->setSizes(lst);
        if (ACTIVE_PANEL)
            ACTIVE_PANEL->view()->widget()->setFocus();
    }
}

QString ListPanel::realPath() const
{
    return _realPath.path();
}

void ListPanel::setButtons()
{
    KConfigGroup group(krConfig, "Look&Feel");

    mediaButton->setVisible(group.readEntry("Media Button Visible", true));
    backButton->setVisible(group.readEntry("Back Button Visible", false));
    forwardButton->setVisible(group.readEntry("Forward Button Visible", false));
    historyButton->setVisible(group.readEntry("History Button Visible", true));
    bookmarksButton->setVisible(group.readEntry("Bookmarks Button Visible", true));

    if (group.readEntry("Panel Toolbar visible", _PanelToolBar)) {
        cdRootButton->setVisible(group.readEntry("Root Button Visible", _cdRoot));
        cdHomeButton->setVisible(group.readEntry("Home Button Visible", _cdHome));
        cdUpButton->setVisible(group.readEntry("Up Button Visible", _cdUp));
        cdOtherButton->setVisible(group.readEntry("Equal Button Visible", _cdOther));
        origin->button()->setVisible(group.readEntry("Open Button Visible", _Open));
        syncBrowseButton->setVisible(group.readEntry("SyncBrowse Button Visible", _syncBrowseButton));
    } else {
        cdRootButton->hide();
        cdHomeButton->hide();
        cdUpButton->hide();
        cdOtherButton->hide();
        origin->button()->hide();
        syncBrowseButton->hide();
    }
}

void ListPanel::slotUpdateTotals()
{
    totals->setText(_view->statistics());
}

//TODO move this to panelfunc ?
void ListPanel::compareDirs(bool otherPanelToo)
{
    KConfigGroup pg(krConfig, "Private");
    int compareMode = pg.readEntry("Compare Mode", 0);
    KConfigGroup group(krConfig, "Look&Feel");
    bool selectDirs = group.readEntry("Mark Dirs", false);

    KFileItemList items = _view->getItems();
    KFileItemList otherItems = otherPanel()->_view->getItems();

    QHash<QString, KFileItem> otherItemsDict;
    foreach(KFileItem otherItem, otherItems) {
        // check for duplicate file names in the other panel
        if(otherItemsDict.contains(otherItem.name())) {
            KMessageBox::error(0,
                i18n("\"%1\" contains multiple files with the name \"%2\"",
                     otherPanel()->virtualPath().pathOrUrl(), otherItem.name()),
                i18n("Can't compare %1 with %2", virtualPath().pathOrUrl(),
                     otherPanel()->virtualPath().pathOrUrl()));
            return;
        } else
            otherItemsDict.insert(otherItem.name(), otherItem);
    }

    KUrl::List newSelection;

    foreach(KFileItem item, items) {
        KFileItem otherItem = otherItemsDict[item.name()];

        bool isSingle = otherItem.isNull(), isDifferent = false, isNewer = false;

        if (item.isDir() && !selectDirs)
            continue;

        if (!otherItem.isNull()) {
            if (!item.isDir())
                isDifferent = otherItem.size() != item.size();
            isNewer = item.time(KFileItem::ModificationTime) > otherItem.time(KFileItem::ModificationTime); //FIXME can this be made faster ?
        }

        KUrl url = item.url();

        //FIXME use a proper enumeration
        switch (compareMode) {
        case 0:
            if (isNewer || isSingle)
                newSelection << url;
            break;
        case 1:
            if (isNewer)
                newSelection << url;
            break;
        case 2:
            if (isSingle)
                newSelection << url;
            break;
        case 3:
            if (isDifferent || isSingle)
                newSelection << url;
            break;
        case 4:
            if (isDifferent)
                newSelection << url;
            break;
        }
    }

    _view->setSelection(newSelection);

    if(otherPanelToo)
        otherPanel()->compareDirs(false);
}

void ListPanel::refreshColors()
{
    _view->refreshColors();
    emit refreshColors(isActive());
}

void ListPanel::slotFocusOnMe()
{
    if(!isActive())
        emit activate();
    Q_ASSERT(isActive());

    onActiveStateChanged(); // can't hurt
}

void ListPanel::onActiveStateChanged()
{
    if(isActive()) {
        updatePopupPanel(_view->currentItem());
        _view->prepareForActive();
    } else {
        // in case a new url was entered but not refreshed to,
        // reset origin bar to the current url
        origin->setUrl(url().prettyUrl());
        _view->prepareForPassive();
    }

    origin->setActive(isActive());
    refreshColors();
    updateButtons();
}

// this is used to start the panel
//////////////////////////////////////////////////////////////////
void ListPanel::start(KUrl url, bool immediate)
{
    KUrl virt(url);

    if (!virt.isValid())
        virt = KUrl(ROOT_DIR);
    if (virt.isLocalFile())
        _realPath = virt;
    else
        _realPath = KUrl(ROOT_DIR);

    if (immediate)
        func->immediateOpenUrl(virt, true);
    else
        func->openUrl(virt);

    setJumpBack(virt);
}

void ListPanel::slotStartUpdate()
{
    if (inlineRefreshJob)
        inlineRefreshListResult(0);

    setCursor(Qt::BusyCursor);

    if (func->files() ->vfs_getType() == vfs::VFS_NORMAL)
        _realPath = virtualPath();
    this->origin->setUrl(virtualPath().pathOrUrl());

    onUrlRefreshed();

    slotGetStats(virtualPath());
    if (compareMode)
        otherPanel()->_view->refresh();

    // return cursor to normal arrow
    setCursor(Qt::ArrowCursor);
    slotUpdateTotals();
    krApp->popularUrls()->addUrl(virtualPath());
}

void ListPanel::slotGetStats(const KUrl& url)
{
    mediaButton->mountPointChanged(QString());
    freeSpace->setText(QString());

    if (!KConfigGroup(krConfig, "Look&Feel").readEntry("ShowSpaceInformation", true)) {
        if(func->files()->metaInformation().isEmpty())
            status->setText(i18n("Space information disabled"));
        else
            status->setText(func->files()->metaInformation());
        return ;
    }

    if (!url.isLocalFile()) {
        if(func->files()->metaInformation().isEmpty())
            status->setText(i18n("No space information on non-local filesystems"));
        else
            status->setText(func->files()->metaInformation());
        return ;
    }

    // check for special filesystems;
    QString path = url.path(); // must be local url
    if (path.left(4) == "/dev") {
        status->setText(i18n("No space information on [dev]"));
        return;
    }
#ifdef BSD
    if (path.left(5) == "/procfs") {     // /procfs is a special case - no volume information
        status->setText(i18n("No space information on [procfs]"));
        return;
    }
#else
    if (path.left(5) == "/proc") {     // /proc is a special case - no volume information
        status->setText(i18n("No space information on [proc]"));
        return;
    }
#endif

    status->setText(i18n("Mt.Man: working..."));
    statsAgent = KDiskFreeSpace::findUsageInfo(path);
    connect(statsAgent, SIGNAL(foundMountPoint(const QString &, quint64, quint64, quint64)),
            this, SLOT(gotStats(const QString &, quint64, quint64, quint64)));
}

void ListPanel::gotStats(const QString &mountPoint, quint64 kBSize,
                         quint64,  quint64 kBAvail)
{
    int perc = 0;
    if (kBSize != 0) { // make sure that if totalsize==0, then perc=0
        perc = (int)(((float)kBAvail / (float)kBSize) * 100.0);
    }
    // mount point information - find it in the list first
    KMountPoint::List lst = KMountPoint::currentMountPoints();
    QString fstype = i18nc("Unknown file system type", "unknown");
    for (KMountPoint::List::iterator it = lst.begin(); it != lst.end(); ++it) {
        if ((*it)->mountPoint() == mountPoint) {
            fstype = (*it)->mountType();
            break;
        }
    }

    QString stats = i18nc("%1=free space,%2=total space,%3=percentage of usage, "
                          "%4=mountpoint,%5=filesystem type",
                          "%1 free out of %2 (%3%) on %4 [(%5)]",
                          KIO::convertSizeFromKiB(kBAvail),
                          KIO::convertSizeFromKiB(kBSize), perc,
                          mountPoint, fstype);

    status->setText(stats);

    freeSpace->setText("    " + i18n("%1 free", KIO::convertSizeFromKiB(kBAvail)));
    mediaButton->mountPointChanged(mountPoint);
}

void ListPanel::handleDropOnView(QDropEvent *e, QWidget *widget)
{
    // if copyToPanel is true, then we call a simple vfs_addfiles
    bool copyToDirInPanel = false;
    bool dragFromThisPanel = false;
    bool isWritable = func->files() ->vfs_isWritable();

    vfs* tempFiles = func->files();
    KFileItem item;
    bool itemIsUpUrl = false;
    if (!widget) {
        item = _view->itemAt(e->pos(), &itemIsUpUrl);
        widget = this;
    }

    if (e->source() == this)
        dragFromThisPanel = true;

    if (!item.isNull() || itemIsUpUrl) {
        if (itemIsUpUrl) {   // trying to drop on the ".."
            copyToDirInPanel = true;
        } else if (!item.isNull()) {
            if (item.isDir()) {
                copyToDirInPanel = true;
                isWritable = item.isWritable();
                if (isWritable) {
                    // keep the folder_open icon until we're finished, do it only
                    // if the folder is writeable, to avoid flicker
                }
            } else if (e->source() == this)
                return ; // no dragging onto ourselves
        }
    } else    // if dragged from this panel onto an empty spot in the panel...
        if (dragFromThisPanel) {    // leave!
            e->ignore();
            return ;
        }

    if (!isWritable && getuid() != 0) {
        e->ignore();
        KMessageBox::sorry(0, i18n("Cannot drop here, no write permissions."));
        return ;
    }
    //////////////////////////////////////////////////////////////////////////////
    // decode the data
    KUrl::List URLs = KUrl::List::fromMimeData(e->mimeData());
    if (URLs.isEmpty()) {
        e->ignore(); // not for us to handle!
        return ;
    }

    bool isLocal = true;
    for (int u = 0; u != URLs.count(); u++)
        if (!URLs[ u ].isLocalFile()) {
            isLocal = false;
            break;
        }

    KIO::CopyJob::CopyMode mode = KIO::CopyJob::Copy;

    // the KUrl::List is finished, let's go
    // --> display the COPY/MOVE/LINK menu

    QMenu popup(this);
    QAction * act;

    act = popup.addAction(i18n("Copy Here"));
    act->setData(QVariant(1));
    if (func->files() ->vfs_isWritable()) {
        act = popup.addAction(i18n("Move Here"));
        act->setData(QVariant(2));
    }
    if (func->files() ->vfs_getType() == vfs::VFS_NORMAL && isLocal) {
        act = popup.addAction(i18n("Link Here"));
        act->setData(QVariant(3));
    }
    act = popup.addAction(i18n("Cancel"));
    act->setData(QVariant(4));

    QPoint tmp = widget->mapToGlobal(e->pos());

    int result = -1;
    QAction * res = popup.exec(tmp);
    if (res && res->data().canConvert<int> ())
        result = res->data().toInt();

    switch (result) {
    case 1 :
        mode = KIO::CopyJob::Copy;
        break;
    case 2 :
        mode = KIO::CopyJob::Move;
        break;
    case 3 :
        mode = KIO::CopyJob::Link;
        break;
    default :         // user pressed outside the menu
        return ;          // or cancel was pressed;
    }

    QString dir = "";
    if (copyToDirInPanel) {
        dir = item.name();
    }
    QWidget *notify = (!e->source() ? 0 : e->source());
    tempFiles->vfs_addFiles(&URLs, mode, notify, dir);
    if(KConfigGroup(krConfig, "Look&Feel").readEntry("UnselectBeforeOperation", _UnselectBeforeOperation)) {
        AbstractListPanel *p = (dragFromThisPanel ? this : otherPanel());
        p->view()->saveSelection();
        p->view()->unselect(KRQuery("*"));
    }
}

void ListPanel::vfs_refresh(KJob* /*job*/)
{
    if (func)
        func->refresh();
}

void ListPanel::startDragging(KUrl::List urls, QPixmap px)
{
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    drag->setPixmap(px);
    urls.populateMimeData(mimeData);
    drag->setMimeData(mimeData);

    drag->start();
}

// pops a right-click menu for items
void ListPanel::popRightClickMenu(const QPoint &loc)
{
    KrPopupMenu::run(loc, this);
}

void ListPanel::popEmptyRightClickMenu(const QPoint &loc)
{
    KrPopupMenu::run(loc, this);
}

QString ListPanel::getCurrentName()
{
    QString name = _view->getCurrentItem();
    if (name != "..")
        return name;
    else
        return QString();
}

void ListPanel::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Enter :
    case Qt::Key_Return :
        if (e->modifiers() & Qt::ControlModifier) {
            if (e->modifiers() & Qt::AltModifier) {
                KFileItem item = _view->currentItem();
                if (!item.isNull()) {
                    if (item.isDir())
                        newTab(item.url(), true);
                } else if (_view->currentItemIsUpUrl())
                    newTab(func->files()->vfs_getOrigin().upUrl(), true);
            } else {
                SLOTS->insertFileName((e->modifiers() & Qt::ShiftModifier) != 0);
            }
        } else e->ignore();
        break;
    case Qt::Key_Right :
    case Qt::Key_Left :
        if (e->modifiers() == Qt::ControlModifier) {
            // user pressed CTRL+Right/Left - refresh other panel to the selected path if it's a
            // directory otherwise as this one
            if ((isLeft() && e->key() == Qt::Key_Right) || (!isLeft() && e->key() == Qt::Key_Left)) {
                KUrl newPath;
                KFileItem item = _view->currentItem();
                if (!item.isNull()) {
                    if (item.isDir())
                        newPath = item.url();
                    else
                        newPath = func->files() ->vfs_getOrigin();
                } else if (_view->currentItemIsUpUrl())
                    newPath = func->files()->vfs_getOrigin().upUrl();
                otherPanel()->func->openUrl(newPath);
            } else
                func->openUrl(otherPanel()->func->files() ->vfs_getOrigin());
            return ;
        } else
            e->ignore();
        break;
    case Qt::Key_Down :
        if (e->modifiers() == Qt::ControlModifier) {   // give the keyboard focus to the command line
            if (MAIN_VIEW->cmdLine()->isVisible())
                MAIN_VIEW->cmdLineFocus();
            else
                MAIN_VIEW->focusTerminalEmulator();
            return ;
        } else if (e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {     // give the keyboard focus to TE
            MAIN_VIEW->focusTerminalEmulator();
        } else
            e->ignore();
        break;

    case Qt::Key_Up :
        if (e->modifiers() == Qt::ControlModifier) {   // give the keyboard focus to the command line
            origin->lineEdit()->setFocus();
            origin->lineEdit()->selectAll();
            return ;
        } else
            e->ignore();
        break;

    case Qt::Key_Escape:
        inlineRefreshCancel();
        break;

    default:
        // if we got this, it means that the view is not doing
        // the quick search thing, so send the characters to the commandline, if normal key
        if (e->modifiers() == Qt::NoModifier)
            MAIN_VIEW->cmdLine()->addText(e->text());

        //e->ignore();
    }
}

void ListPanel::showEvent(QShowEvent *e)
{
    panelActive();
    QWidget::showEvent(e);
}

void ListPanel::hideEvent(QHideEvent *e)
{
    panelInactive();
    QWidget::hideEvent(e);
}

void ListPanel::panelActive()
{
    // don't refresh when not active (ie: hidden, application isn't focused ...)
//     if (!
         func->files()->vfs_enableRefresh(true)
//        )
//         func->popErronousUrl()
                ;
}

void ListPanel::panelInactive()
{
    func->files()->vfs_enableRefresh(false);
}

void ListPanel::slotPreviewJobStarted(KJob *job)
{
    previewJob = job;
    previewProgress->setValue(0);
    previewProgress->setFormat(i18n("loading previews: %p%"));
    previewProgress->show();
    inlineRefreshCancelButton->show();
    previewProgress->setMaximumHeight(inlineRefreshCancelButton->height());
    connect(job, SIGNAL(percent(KJob*, unsigned long)), SLOT(slotPreviewJobPercent(KJob*, unsigned long)));
    connect(job, SIGNAL(result(KJob*)), SLOT(slotPreviewJobResult(KJob*)));
}

void ListPanel::slotPreviewJobPercent(KJob* /*job*/, unsigned long percent)
{
    previewProgress->setValue(percent);
}

void ListPanel::slotPreviewJobResult(KJob* /*job*/)
{
    previewJob = 0;
    previewProgress->hide();
    if(!inlineRefreshJob)
        inlineRefreshCancelButton->hide();
}

void ListPanel::slotJobStarted(KIO::Job* job)
{
    // disable the parts of the panel we don't want touched
    status->setEnabled(false);
    origin->setEnabled(false);
    cdRootButton->setEnabled(false);
    cdHomeButton->setEnabled(false);
    cdUpButton->setEnabled(false);
    cdOtherButton->setEnabled(false);
    popupBtn->setEnabled(false);
    if(popup)
        popup->setEnabled(false);
    bookmarksButton->setEnabled(false);
    historyButton->setEnabled(false);
    syncBrowseButton->setEnabled(false);

    // connect to the job interface to provide in-panel refresh notification
    connect(job, SIGNAL(infoMessage(KJob*, const QString &)),
            SLOT(inlineRefreshInfoMessage(KJob*, const QString &)));
    connect(job, SIGNAL(percent(KJob*, unsigned long)),
            SLOT(inlineRefreshPercent(KJob*, unsigned long)));
    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(inlineRefreshListResult(KJob*)));

    inlineRefreshJob = job;

    totals->setText(i18n(">> Reading..."));
    inlineRefreshCancelButton->show();
}

void ListPanel::inlineRefreshCancel()
{
    if (inlineRefreshJob) {
        disconnect(inlineRefreshJob, 0, this, 0);
        inlineRefreshJob->kill(KJob::EmitResult);
        inlineRefreshListResult(0);
    }
    if(previewJob) {
        disconnect(previewJob, 0, this, 0);
        previewJob->kill(KJob::EmitResult);
        slotPreviewJobResult(0);
    }
}

void ListPanel::inlineRefreshPercent(KJob*, unsigned long perc)
{
    QString msg = i18n(">> Reading: %1 % complete...", perc);
    totals->setText(msg);
}

void ListPanel::inlineRefreshInfoMessage(KJob*, const QString &msg)
{
    totals->setText(i18n(">> Reading: %1", msg));
}

void ListPanel::inlineRefreshListResult(KJob*)
{
    if(inlineRefreshJob)
        disconnect(inlineRefreshJob, 0, this, 0);
    inlineRefreshJob = 0;
    // reenable everything
    status->setEnabled(true);
    origin->setEnabled(true);
    cdRootButton->setEnabled(true);
    cdHomeButton->setEnabled(true);
    cdUpButton->setEnabled(true);
    cdOtherButton->setEnabled(true);
    popupBtn->setEnabled(true);
    if(popup)
        popup->setEnabled(true);
    bookmarksButton->setEnabled(true);
    historyButton->setEnabled(true);
    syncBrowseButton->setEnabled(true);

    if(!previewJob)
        inlineRefreshCancelButton->hide();
}

void ListPanel::jumpBack()
{
    func->openUrl(_jumpBackURL);
}

void ListPanel::setJumpBack(KUrl url)
{
    _jumpBackURL = url;
}

void ListPanel::slotVfsError(QString msg)
{
    refreshColors();
    vfsError->setText(i18n("Error: %1", msg));
    vfsError->show();
}

void ListPanel::showButtonMenu(QToolButton *b)
{
    Q_ASSERT(isActive());

    if(b->isHidden())
        b->menu()->exec(mapToGlobal(clientArea->pos()));
    else
        b->click();
}

void ListPanel::openBookmarks()
{
    showButtonMenu(bookmarksButton);
}

void ListPanel::openHistory()
{
    showButtonMenu(historyButton);
}

void ListPanel::openMedia()
{
    showButtonMenu(mediaButton);
}

void ListPanel::rightclickMenu()
{
    KrPopupMenu::run(this);
}

void ListPanel::openWithMenu()
{
    KrPopupMenu::run(this, true);
}

void ListPanel::toggleSyncBrowse()
{
    syncBrowseButton->toggle();
}

void ListPanel::editLocation()
{
    origin->lineEdit()->selectAll();
    origin->edit();
}

void ListPanel::saveSettings(KConfigGroup cfg, bool localOnly, bool saveHistory)
{
    KUrl tmpUrl = url();
    tmpUrl.setPass(QString()); // make sure no password is saved
    cfg.writeEntry("Url", tmpUrl.pathOrUrl());
    cfg.writeEntry("Type", type());
    cfg.writeEntry("Properties", getProperties());
    if(popup) {
        popup->saveSizes(); //FIXME use this cfg group
        cfg.writeEntry("PopupPage", popup->currentPage());
    }
    if(saveHistory)
        func->history->save(KConfigGroup(&cfg, "History"));
    _view->saveSettings(KConfigGroup(&cfg, "View"));
}

void ListPanel::restoreSettings(KConfigGroup cfg)
{
    changeType(cfg.readEntry("Type", defaultPanelType()));

    setProperties(cfg.readEntry("Properties", 0));
    _view->restoreSettings(KConfigGroup(&cfg, "View"));

    func->files()->vfs_enableRefresh(true);

    _realPath = KUrl(ROOT_DIR);

    if(func->history->restore(KConfigGroup(&cfg, "History")))
        func->refresh();
    else {
        KUrl url(cfg.readEntry("Url", ROOT_DIR));
        if (!url.isValid())
            url = KUrl(ROOT_DIR);
        func->openUrl(url);
    }

    setJumpBack(func->history->currentUrl());
}

void ListPanel::updatePopupPanel(KFileItem item)
{
    // which panel to display on?
    PanelPopup *p = 0;
    if(popup && !popup->isHidden())
        p = popup;
    else if(otherPanel()->popup && !otherPanel()->popup->isHidden())
        p = otherPanel()->popup;
    else
        return;

    p->update(item);
}

void ListPanel::onOtherPanelChanged()
{
    func->syncURL = KUrl();
}

void ListPanel::getFocusCandidates(QVector<QWidget*> &widgets)
{
    if(origin->lineEdit()->isVisible())
        widgets << origin->lineEdit();
    if(_view->widget()->isVisible())
        widgets << _view->widget();
    if(popup && popup->isVisible())
        widgets << popup;
}

void ListPanel::updateButtons()
{
    backButton->setEnabled(func->history->canGoBack());
    forwardButton->setEnabled(func->history->canGoForward());
    historyButton->setEnabled(func->history->count() > 1);
    cdRootButton->setEnabled(!virtualPath().equals(KUrl(ROOT_DIR),
                                    KUrl::CompareWithoutTrailingSlash));
    cdUpButton->setEnabled(!func->files()->isRoot());
    cdHomeButton->setEnabled(!func->atHome());
}

void ListPanel::newTab(KFileItem item, bool itemIsUpUrl)
{
    if(itemIsUpUrl)
        newTab(virtualPath().upUrl(), true);
    else if (item.isNull())
        return;
    else if (item.isDir())
        newTab(item.url(), true);
}

bool ListPanel::canDelete()
{
    return func && func->files() && !func->files()->vfs_canDelete();
}

void ListPanel::requestDelete()
{
    connect(func->files(), SIGNAL(deleteAllowed()), this, SLOT(deleteLater()));
    func->files()->vfs_requestDelete();
}

void ListPanel::onUrlRefreshed()
{
    emit pathChanged(this);
    updateButtons();
}
