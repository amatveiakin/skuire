/***************************************************************************
                                 krview.cpp
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

                                                    S o u r c e    F i l e

***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "krview.h"

#include "viewactions.h"
#include "krviewfactory.h"
#include "krselectionmode.h"
#include "krcolorcache.h"
#include "krpreviews.h"
#include "quickfilter.h"
#include "../kicons.h"
#include "../defaults.h"
#include "../VFS/krpermhandler.h"
#include "../VFS/abstractdirlister.h"
#include "../Dialogs/krspecialwidgets.h"
#include "../Filter/filterdialog.h"

#include <qnamespace.h>
#include <qpixmapcache.h>
#include <QtCore/QDir>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>
#include <QPixmap>
#include <QAction>
#include <kmimetype.h>
#include <klocale.h>
#include <kinputdialog.h>


// ----------------------------- operator

KrView *KrViewOperator::_changedView = 0;
KrViewProperties::PropertyType KrViewOperator::_changedProperties = KrViewProperties::NoProperty;


KrViewOperator::KrViewOperator(KrView *view, QWidget *widget) :
        _view(view), _widget(widget), _quickSearch(0), _quickFilter(0), _massSelectionUpdate(false)
{
    _saveDefaultSettingsTimer.setSingleShot(true);
    connect(&_saveDefaultSettingsTimer, SIGNAL(timeout()), SLOT(saveDefaultSettings()));
    connect(&KrColorCache::getColorCache(), SIGNAL(colorsRefreshed()), SLOT(colorSettingsChanged()));
    _widget->installEventFilter(this);
}

KrViewOperator::~KrViewOperator()
{
    if(_changedView == _view)
        saveDefaultSettings();
}

void KrViewOperator::fileDeleted(const QString& name)
{
    KFileItem it;
    foreach(KFileItem item, _view->getItems())
        if(item.name() == name)
            it = item;

    if(it.isNull())
        return;

    KFileItemList list;
    list << it;

    _view->itemsDeleted(list);
}

void KrViewOperator::refreshItem(KFileItem item)
{
    QList< QPair<KFileItem, KFileItem> > list;
    list << QPair<KFileItem, KFileItem> (item, item);

    _view->refreshItems(list);
}

void KrViewOperator::startDrag()
{
    KUrl::List urls = _view->getSelectedUrls(true);
    if (urls.empty())
        return ; // don't drag an empty thing
    QPixmap px;
    if (urls.count() > 1 || _view->currentItem().isNull())
        px = FL_LOADICON("queue");   // how much are we dragging
    else
        px = _view->icon(_view->currentUrl());
    emit letsDrag(urls, px);
}

void KrViewOperator::setQuickSearch(KrQuickSearch *quickSearch)
{
    _quickSearch = quickSearch;

    _quickSearch->setFocusProxy(_view->widget());

    connect(quickSearch, SIGNAL(textChanged(const QString&)), this, SLOT(quickSearch(const QString&)));
    connect(quickSearch, SIGNAL(otherMatching(const QString&, int)), this, SLOT(quickSearch(const QString& , int)));
    connect(quickSearch, SIGNAL(stop(QKeyEvent*)), this, SLOT(stopQuickSearch(QKeyEvent*)));
    connect(quickSearch, SIGNAL(process(QKeyEvent*)), this, SLOT(handleQuickSearchEvent(QKeyEvent*)));
}

void KrViewOperator::handleQuickSearchEvent(QKeyEvent * e)
{
    switch (e->key()) {
    case Qt::Key_Insert: {
        KFileItem item = _view->currentItem();
        if(!item.isNull()) {
            _view->selectCurrentItem(!_view->isCurrentItemSelected());
            quickSearch(_quickSearch->text(), 1);
        }
        break;
    }
    case Qt::Key_Home:
        _view->setCurrentItem(KrView::Last);
        if (!_view->currentItem().isNull())
            quickSearch(_quickSearch->text(), 1);
        break;
    case Qt::Key_End:
        _view->setCurrentItem(KrView::First);
        if (!_view->currentItem().isNull())
            quickSearch(_quickSearch->text(), -1);
        break;
    }
}

void KrViewOperator::quickSearch(const QString & str, int direction)
{
    _quickSearch->setMatch(_view->quickSearch(str, direction));
}

void KrViewOperator::stopQuickSearch(QKeyEvent * e)
{
    if (_quickSearch) {
        _quickSearch->hide();
        _quickSearch->clear();
        if (e)
            _view->handleKeyEvent(e);
    }
}

void KrViewOperator::setQuickFilter(QuickFilter *quickFilter)
{
    _quickFilter = quickFilter;
    _quickFilter->lineEdit()->installEventFilter(this);
    connect(_quickFilter, SIGNAL(stop()), SLOT(stopQuickFilter()));
    connect(_quickFilter->lineEdit(), SIGNAL(textEdited(const QString&)), SLOT(quickFilterChanged(const QString&)));
    connect(_quickFilter->lineEdit(), SIGNAL(returnPressed(const QString&)), _view->widget(), SLOT(setFocus()));
}

void KrViewOperator::quickFilterChanged(const QString &text)
{
    KConfigGroup grpSvr(_view->_config, "Look&Feel");
    bool caseSensitive = grpSvr.readEntry("Case Sensitive Quicksearch", _CaseSensitiveQuicksearch);

    _view->_quickFilterMask = QRegExp(text, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive, QRegExp::Wildcard);
    _view->refresh();
    _quickFilter->setMatch(_view->count() || !_view->_dirLister->numItems());
}

void KrViewOperator::startQuickFilter()
{
    _quickFilter->show();
    _quickFilter->lineEdit()->setFocus();
}

void KrViewOperator::stopQuickFilter(bool refreshView)
{
    if(_quickFilter->lineEdit()->hasFocus())
        _widget->setFocus();
    _quickFilter->hide();
    _quickFilter->lineEdit()->clear();
    _quickFilter->setMatch(true);
    _view->_quickFilterMask = QRegExp();
    if(refreshView)
        _view->refresh();
}

void KrViewOperator::prepareForPassive()
{
    if (_quickSearch && !_quickSearch->isHidden()) {
        stopQuickSearch(0);
    }
}

bool KrViewOperator::handleKeyEvent(QKeyEvent * e)
{
    if (!_quickSearch->isHidden()) {
        _quickSearch->myKeyPressEvent(e);
        return true;
    }
    return false;
}

void KrViewOperator::setMassSelectionUpdate(bool upd)
{
    _massSelectionUpdate = upd;
    if (!upd) {
        emit selectionChanged();
        _view->redraw();
    }
}

void KrViewOperator::settingsChanged(KrViewProperties::PropertyType properties)
{
    if(_view->_updateDefaultSettings) {
        if(_changedView != _view)
            saveDefaultSettings();
        _changedView = _view;
        _changedProperties = static_cast<KrViewProperties::PropertyType>(_changedProperties | properties);
        _saveDefaultSettingsTimer.start(100);
    }
}

void KrViewOperator::saveDefaultSettings()
{
    _saveDefaultSettingsTimer.stop();
    if(_changedView)
        _changedView->saveDefaultSettings(_changedProperties);
    _changedProperties = KrViewProperties::NoProperty;
    _changedView = 0;
}

bool KrViewOperator::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if(ke->key() == Qt::Key_Escape && ke->modifiers() == Qt::NoModifier) {
            if(!_quickSearch->isHidden())
                stopQuickSearch(0);
            else if(!_quickFilter->isHidden())
                stopQuickFilter();
            else
                return false;
            event->accept();
            return true;
        }
    }
    else if(event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if(ke->key() == Qt::Key_Escape && ke->modifiers() == Qt::NoModifier &&
                (!_quickSearch->isHidden() || !_quickFilter->isHidden())) {
            event->accept();
            return true;
        }
        else if(watched == _widget && !_quickSearch->isHidden())
            return _quickSearch->shortcutOverride(ke);
    }
    return false;
}


// ----------------------------- KrView::Item

KrView::Item::Item(const KFileItem &fileItem, KrView *view, bool isDummy) :
    KFileItem(fileItem)
{
    init(view, isDummy);
}

KrView::Item &KrView::Item::operator=(const Item  &other)
{
    KFileItem::operator=(other);
    init(other._view, false);
    return *this;
}

void KrView::Item::init(KrView *view, bool isDummy)
{
    clearIcon();
    _iconName.clear();
    _krPermissionsString.clear();
    _brokenLink = false;
    _calculatedSize = 0;
    _view = view;

    if(isDummy)
        _iconName = "go-up";

    if(isLink() && url().isLocalFile()) {
        //FIXME: dirs are not recognized

        QString dest = linkDest();

        if(QDir::isRelativePath(dest)) {
            KUrl destUrl = url().upUrl();
            destUrl.addPath(dest);
            dest = destUrl.path();
        }
        if(!QFile::exists(dest)) {
            _brokenLink = true;
            _iconName = "file-broken";
        }
    }

}

void KrView::Item::getIconName() const
{
    _iconName = KFileItem::iconName();
}

void KrView::Item::clearIcon()
{
    _iconActive = _iconInactive = QPixmap();
}

void KrView::Item::setIcon(QPixmap icon)
{
    _iconActive = processIcon(icon, true);

    if (KrColorCache::getColorCache().dimInactive())
        _iconInactive = processIcon(icon, false);
    else
        _iconInactive = _iconActive;
}

void KrView::Item::loadIcon() const
{
    _iconActive = loadIcon(true);
    if (KrColorCache::getColorCache().dimInactive())
        _iconInactive = loadIcon(false);
    else
        _iconInactive = _iconActive;
}

QPixmap KrView::Item::loadIcon(bool active) const
{
    QString cacheName;
    cacheName.append(QString::number(_view->fileIconSize()));
    if (isLink())
        cacheName.append("LINK_");
    if (!active)
        cacheName.append("DIM_");
    cacheName.append(iconName());

    //FIXME
    //QPixmapCache::setCacheLimit( ag.readEntry("Icon Cache Size",_IconCacheSize) );

    QPixmap icon;

    // first try the cache
    if (!QPixmapCache::find(cacheName, icon)) {
        icon = processIcon(
            krLoader->loadIcon(iconName(), KIconLoader::Desktop, _view->fileIconSize()),
            active);
        // insert it into the cache
        QPixmapCache::insert(cacheName, icon);
    }

    return icon;
}

QPixmap KrView::Item::processIcon(QPixmap icon, bool active) const
{
    QColor dimColor;
    int dimFactor;
    bool dim = !active && KrColorCache::getColorCache().getDimSettings(dimColor, dimFactor);

    if (isLink()) {
        QPixmap link(link_xpm);
        QPainter painter(&icon);
        painter.drawPixmap(0, icon.height() - 11, link, 0, 21, 10, 11);
    }

    if(!dim)
        return icon;

    QImage dimmed = icon.toImage();

    QPainter p(&dimmed);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(0, 0, icon.width(), icon.height(), dimColor);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.setOpacity((qreal)dimFactor / (qreal)100);
    p.drawPixmap(0, 0, icon.width(), icon.height(), icon);

    return QPixmap::fromImage(dimmed, Qt::ColorOnly | Qt::ThresholdDither |
                                Qt::ThresholdAlphaDither | Qt::NoOpaqueDetection );

    return icon;
}

void KrView::Item::getKrPermissionsString() const
{
    QString tmp;

    // char ? UGLY !
    char readable = UNKNOWN_PERM;
    if (!url().user().isEmpty())
        readable = KRpermHandler::ftpReadable(user(), url().user(), permissionsString());
    else if (url().isLocalFile())
        readable = KRpermHandler::readable(permissionsString(),
                                            KRpermHandler::group2gid(group()),
                                            KRpermHandler::user2uid(user()), -1);

    char writeable = UNKNOWN_PERM;
    if (!url().user().isEmpty())
        writeable = KRpermHandler::ftpWriteable(user(), url().user(), permissionsString());
    else if (url().isLocalFile())
        writeable = KRpermHandler::writeable(permissionsString(),
                                            KRpermHandler::group2gid(group()),
                                            KRpermHandler::user2uid(user()), -1);

    char executable = UNKNOWN_PERM;
    if (!url().user().isEmpty())
        executable = KRpermHandler::ftpExecutable(user(), url().user(), permissionsString());
    else if (url().isLocalFile())
        executable = KRpermHandler::executable(permissionsString(),
                                            KRpermHandler::group2gid(group()),
                                            KRpermHandler::user2uid(user()), -1);

    switch (readable) {
    case ALLOWED_PERM: tmp+='r'; break;
    case UNKNOWN_PERM: tmp+='?'; break;
    case NO_PERM:      tmp+='-'; break;
    }
    switch (writeable) {
    case ALLOWED_PERM: tmp+='w'; break;
    case UNKNOWN_PERM: tmp+='?'; break;
    case NO_PERM:      tmp+='-'; break;
    }
    switch (executable) {
    case ALLOWED_PERM: tmp+='x'; break;
    case UNKNOWN_PERM: tmp+='?'; break;
    case NO_PERM:      tmp+='-'; break;
    }

    _krPermissionsString = tmp;
}


// ----------------------------- KrView

const KrView::IconSizes KrView::iconSizes;


KrView::KrView(KrViewInstance &instance, KConfig *cfg) :
    _instance(instance), _dirLister(0), _config(cfg), _mainWindow(0), _widget(0),
    _properties(0), _focused(false),
    _previews(0), _fileIconSize(0), _updateDefaultSettings(false)
{
}

KrView::~KrView()
{
    _instance.m_objects.removeOne(this);
    delete _previews;
    _previews = 0;
    if (_properties)
        qFatal("A class inheriting KrView didn't delete _properties!");
    if (_operator)
        qFatal("A class inheriting KrView didn't delete _operator!");
}

void KrView::init()
{
    // sanity checks:
    if (!_widget)
        qFatal("_widget must be set during construction of KrView inheritors");
    // ok, continue
    initProperties();
    _operator = createOperator();
    setup();
    restoreDefaultSettings();
    KConfigGroup grp(_config, _instance.name());
    enableUpdateDefaultSettings(true);
    _instance.m_objects.append(this);
}

void KrView::initProperties()
{
    _properties = createViewProperties();

    KConfigGroup grpSvr(_config, "Look&Feel");
    KConfigGroup grpInstance(_config, _instance.name());

    _properties->displayIcons = grpInstance.readEntry("With Icons", _WithIcons);
    _properties->numericPermissions = grpSvr.readEntry("Numeric permissions", _NumericPermissions);

    int sortOptions = _properties->sortOptions;
    if (grpSvr.readEntry("Show Directories First", true))
        sortOptions |= KrViewProperties::DirsFirst;
    if(grpSvr.readEntry("Always sort dirs by name", false))
        sortOptions |=  KrViewProperties::AlwaysSortDirsByName;
    if (!grpSvr.readEntry("Case Sensative Sort", _CaseSensativeSort))
        sortOptions |= KrViewProperties::IgnoreCase;
    if (grpSvr.readEntry("Locale Aware Sort", true))
        sortOptions |= KrViewProperties::LocaleAwareSort;
    _properties->sortOptions = static_cast<KrViewProperties::SortOptions>(sortOptions);

    _properties->sortMethod = static_cast<KrViewProperties::SortMethod>(
                                  grpSvr.readEntry("Sort method", (int) _DefaultSortMethod));
    _properties->humanReadableSize = grpSvr.readEntry("Human Readable Size", _HumanReadableSize);

    _properties->localeAwareCompareIsCaseSensitive = QString("a").localeAwareCompare("B") > 0;     // see KDE bug #40131
    QStringList defaultAtomicExtensions;
    defaultAtomicExtensions += ".tar.gz";
    defaultAtomicExtensions += ".tar.bz2";
    defaultAtomicExtensions += ".tar.lzma";
    defaultAtomicExtensions += ".tar.xz";
    defaultAtomicExtensions += ".moc.cpp";
    QStringList atomicExtensions = grpSvr.readEntry("Atomic Extensions", defaultAtomicExtensions);
    for (QStringList::iterator i = atomicExtensions.begin(); i != atomicExtensions.end();) {
        QString & ext = *i;
        ext = ext.trimmed();
        if (!ext.length()) {
            i = atomicExtensions.erase(i);
            continue;
        }
        if (!ext.startsWith('.'))
            ext.insert(0, '.');
        ++i;
    }
    _properties->atomicExtensions = atomicExtensions;
}

void KrView::enableUpdateDefaultSettings(bool enable)
{
    if(enable) {
        KConfigGroup grpStartup(_config, "Startup");
        _updateDefaultSettings = grpStartup.readEntry("Update Default Panel Settings", _RememberPos)
                                        || grpStartup.readEntry("UI Save Settings", _UiSave);
    } else
        _updateDefaultSettings  = false;
}

void KrView::showPreviews(bool show)
{
    if(show) { 
        if(!_previews) {
            _previews = new KrPreviews(this);
            QObject::connect(_previews, SIGNAL(gotPreview(KFileItem, QPixmap)),
                             op(), SLOT(gotPreview(KFileItem, QPixmap)));
            _previews->update();
        }
    } else {
        delete _previews;
        _previews = 0;
        refreshIcons();
    }
    redraw();
//     op()->settingsChanged(KrViewProperties::PropShowPreviews);
    op()->emitRefreshActions();
}

void KrView::updatePreviews()
{
    if(_previews)
        _previews->update();
}

QString KrView::statistics()
{
    KIO::filesize_t size = calcSize();
    KIO::filesize_t selectedSize = calcSelectedSize();
    QString tmp;
    KConfigGroup grp(_config, "Look&Feel");
    if(grp.readEntry("Show Size In Bytes", true)) {
        tmp = i18nc("%1=number of selected items,%2=total number of items, \
                    %3=filesize of selected items,%4=filesize in Bytes, \
                    %5=filesize of all items in directory,%6=filesize in Bytes",
                    "%1 out of %2, %3 (%4) out of %5 (%6)",
                    numSelected(), count(), KIO::convertSize(selectedSize),
                    KRpermHandler::parseSize(selectedSize),
                    KIO::convertSize(size),
                    KRpermHandler::parseSize(size));
    } else {
        tmp = i18nc("%1=number of selected items,%2=total number of items, \
                    %3=filesize of selected items,%4=filesize of all items in directory",
                    "%1 out of %2, %3 out of %4",
                    numSelected(), count(), KIO::convertSize(selectedSize),
                    KIO::convertSize(size));
    }

    // notify if we're running a filtered view
    if (filter() != KrViewProperties::All)
        tmp = ">> [ " + filterMask().nameFilter() + " ]  " + tmp;
    return tmp;
}

void KrView::changeSelection(const KRQuery& filter, bool select)
{
    KConfigGroup grpSvr(_config, "Look&Feel");
    changeSelection(filter, select, grpSvr.readEntry("Mark Dirs", _MarkDirs));
}


void KrView::clear()
{
    if(_previews)
        _previews->clear();

    redraw();
}

// good old dialog box
void KrView::renameCurrentItem()
{
    KFileItem item = currentItem();

    if (item.isNull())
        return;

    bool ok = false;
    QString newName = KInputDialog::getText(i18n("Rename"), i18n("Rename %1 to:", item.name()),
                                    item.name(), &ok, _mainWindow);
    // if the user canceled - quit
    if (!ok || newName == item.name())
        return ;
    op()->emitRenameItem(item, newName);
}

bool KrView::handleKeyEvent(QKeyEvent *e)
{
    if (op()->handleKeyEvent(e))
        return true;
    bool res = handleKeyEventInt(e);

    // emit the new item description
    QString desc = currentDescription();
    if(!desc.isEmpty())
        op()->emitItemDescription(desc);

    return res;
}

bool KrView::handleKeyEventInt(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Enter :
    case Qt::Key_Return : {
        if (e->modifiers() & Qt::ControlModifier)           // let the panel handle it
            e->ignore();
        else {
            KFileItem item = currentItem();
            if (!item.isNull())
                op()->emitExecuted(item);
            else if (currentItemIsUpUrl())
                op()->emitDirUp();
        }
        return true;
    }
    case Qt::Key_QuoteLeft :          // Terminal Emulator bugfix
        if (e->modifiers() == Qt::ControlModifier) {   // let the panel handle it
            e->ignore();
        } else {          // a normal click - do a lynx-like moving thing
            op()->emitGoHome(); // ask krusader to move to the home directory
        }
        return true;
    case Qt::Key_Delete :                   // kill file
        op()->emitDeleteFiles(e->modifiers() == Qt::ShiftModifier || e->modifiers() == Qt::ControlModifier);
        return true;
    case Qt::Key_Insert: {
        selectCurrentItem(!isCurrentItemSelected());
        if (KrSelectionMode::getSelectionHandler()->insertMovesDown())
            setCurrentItem(Next);
        op()->emitSelectionChanged();
        return true;
    }
    case Qt::Key_Space: {
        KFileItem item = currentItem();
        if (!item.isNull()) {
            selectCurrentItem(!isCurrentItemSelected());

            if (item.isDir() && item.size() <= 0 &&
                    KrSelectionMode::getSelectionHandler()->spaceCalculatesDiskSpace()) {
                op()->emitCalcSpace(item);
            }
            if (KrSelectionMode::getSelectionHandler()->spaceMovesDown())
                setCurrentItem(Next);
            op()->emitSelectionChanged();
        }
        return true;
    }
    case Qt::Key_Backspace :                         // Terminal Emulator bugfix
    case Qt::Key_Left :
        if (e->modifiers() == Qt::ControlModifier || e->modifiers() == Qt::ShiftModifier || 
                e->modifiers() == Qt::AltModifier) {   // let the panel handle it
            e->ignore();
        } else {          // a normal click - do a lynx-like moving thing
            op()->emitDirUp(); // ask krusader to move up a directory
        }
        return true;         // safety
    case Qt::Key_Right :
        if (e->modifiers() == Qt::ControlModifier || e->modifiers() == Qt::ShiftModifier ||
                e->modifiers() == Qt::AltModifier) {   // let the panel handle it
            e->ignore();
        } else { // just a normal click - do a lynx-like moving thing
            KFileItem item = currentItem();
            if (!item.isNull())
                op()->emitGoInside(item);
            else if (currentItemIsUpUrl())
                op()->emitDirUp();
        }
        return true;
    case Qt::Key_Up :
        if (e->modifiers() == Qt::ControlModifier) {   // let the panel handle it - jump to the Location Bar
            e->ignore();
        } else {
            if (e->modifiers() == Qt::ShiftModifier) {
                selectCurrentItem(!isCurrentItemSelected());
                op()->emitSelectionChanged();
            }
            setCurrentItem(Prev);
        }
        return true;
    case Qt::Key_Down :
        if (e->modifiers() == Qt::ControlModifier || e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {     // let the panel handle it - jump to command line
            e->ignore();
        } else {
            if (e->modifiers() == Qt::ShiftModifier) {
                selectCurrentItem(!isCurrentItemSelected());
                op()->emitSelectionChanged();
            }
            setCurrentItem(Next);
        }
        return true;
    case Qt::Key_Home: {
        if ((e->modifiers() & Qt::ShiftModifier) && !currentItemIsUpUrl()) /* Shift+Home */
            selectRegion(firstItem(), currentItem(), true, true);
        setCurrentItem(First);
    }
    return true;
    case Qt::Key_End:
        if (e->modifiers() & Qt::ShiftModifier) {
            KFileItem first = currentItemIsUpUrl() ? firstItem() : currentItem();
            selectRegion(first, lastItem(), true, true);
        } else
            setCurrentItem(Last);
        return true;
    case Qt::Key_PageDown:
        pageDown();
        return true;
    case Qt::Key_PageUp:
        pageUp();
        return true;
    case Qt::Key_Escape:
        e->ignore();
        return true; // otherwise the selection gets lost??!??
                     // also it is needed by the panel
    case Qt::Key_A :                 // mark all
        if (e->modifiers() == Qt::ControlModifier) {
            //FIXME: shouldn't there also be a shortcut for unselecting everything ?
            selectAllIncludingDirs();
            return true;
        }
        // default continues here !!!!!!!!!!!
    default:
        // if the key is A..Z or 1..0 do quick search otherwise...
        if (e->text().length() > 0 && e->text()[ 0 ].isPrint())    // better choice. Otherwise non-ascii characters like  can not be the first character of a filename
            // are we doing quicksearch? if not, send keys to panel
            //if ( _config->readBoolEntry( "Do Quicksearch", _DoQuicksearch ) ) {
            // are we using krusader's classic quicksearch, or wincmd style?
        {
            KConfigGroup grpSv(_config, "Look&Feel");
            if (!grpSv.readEntry("New Style Quicksearch", _NewStyleQuicksearch))
                return false;
            else {
                // first, show the quicksearch if its hidden
                if (op()->quickSearch()->isHidden()) {
                    op()->quickSearch()->show();
                    // HACK: if the pressed key requires a scroll down, the selected
                    // item is "below" the quick search window, as the icon view will
                    // realize its new size after the key processing. The following line
                    // will resize the icon view immediately.
                    // ACTIVE_PANEL->gui->layout->activate();
                    // UPDATE: it seems like this isn't needed anymore, in case I'm wrong
                    // do something like op()->emitQuickSearchStartet()
                    // -----------------------------
                }
                // now, send the key to the quicksearch
                op()->quickSearch()->myKeyPressEvent(e);
                return true;
            }
        } else {
            if (!op()->quickSearch()->isHidden()) {
                op()->quickSearch()->hide();
                op()->quickSearch()->clear();
            }
        }
    }
    return false;
}

void KrView::zoomIn()
{
    int idx = iconSizes.indexOf(_fileIconSize);
    if(idx >= 0 && (idx+1) < iconSizes.count())
        setFileIconSize(iconSizes[idx+1]);
}

void KrView::zoomOut()
{
    int idx = iconSizes.indexOf(_fileIconSize);
    if(idx > 0)
        setFileIconSize(iconSizes[idx-1]);
}

void KrView::setFileIconSize(int size)
{
    if(iconSizes.indexOf(size) < 0)
        return;
    _fileIconSize = size;

    refreshIcons();
    updatePreviews();
    redraw();
    op()->emitRefreshActions();
}

int KrView::defaultFileIconSize()
{
    KConfigGroup grpSvr(_config, _instance.name());
    return grpSvr.readEntry("IconSize", _FilelistIconSize).toInt();
}

void KrView::saveDefaultSettings(KrViewProperties::PropertyType properties)
{
    saveSettings(KConfigGroup(_config, _instance.name()), properties);
    op()->emitRefreshActions();
}

void KrView::restoreDefaultSettings()
{
    restoreSettings(KConfigGroup(_config, _instance.name()));
}

void KrView::saveSettings(KConfigGroup group, KrViewProperties::PropertyType properties)
{
    if(properties & KrViewProperties::PropIconSize)
        group.writeEntry("IconSize", fileIconSize());
    if(properties & KrViewProperties::PropShowPreviews)
        group.writeEntry("ShowPreviews", previewsShown());
    if(properties & KrViewProperties::PropSortMode)
        saveSortMode(group);
    if(properties & KrViewProperties::PropFilter) {
        group.writeEntry("Filter", static_cast<int>(_properties->filter));
        group.writeEntry("FilterApplysToDirs", _properties->filterApplysToDirs);
        if(_properties->filterSettings.isValid())
            _properties->filterSettings.save(KConfigGroup(&group, "FilterSettings"));
    }
}

void KrView::restoreSettings(KConfigGroup group)
{
    bool tmp = _updateDefaultSettings;
    _updateDefaultSettings = false;
    doRestoreSettings(group);
    _updateDefaultSettings = tmp;
    refresh();
}

void KrView::doRestoreSettings(KConfigGroup group)
{
    restoreSortMode(group);
    setFileIconSize(group.readEntry("IconSize", defaultFileIconSize()));
    showPreviews(group.readEntry("ShowPreviews", false));
    _properties->filter = static_cast<KrViewProperties::FilterSpec>(group.readEntry("Filter",
                                                    static_cast<int>(KrViewProperties::All)));
    _properties->filterApplysToDirs = group.readEntry("FilterApplysToDirs", false);
    _properties->filterSettings.load(KConfigGroup(&group, "FilterSettings"));
    _properties->filterMask = _properties->filterSettings.toQuery();
}

void KrView::applySettingsToOthers()
{
    for(int i = 0; i < _instance.m_objects.length(); i++) {
        KrView *view = _instance.m_objects[i];
        if(this != view) {
            bool tmp = view->_updateDefaultSettings;
            view->_updateDefaultSettings = false;
            _instance.m_objects[i]->copySettingsFrom(this);
            view->_updateDefaultSettings = tmp;
        }
    }
}

void KrView::sortModeUpdated(KrViewProperties::ColumnType sortColumn, bool descending)
{
    if(sortColumn == _properties->sortColumn && descending == (bool) (_properties->sortOptions & KrViewProperties::Descending))
        return;

    int options = _properties->sortOptions;
    if(descending)
        options |= KrViewProperties::Descending;
    else
        options &= ~KrViewProperties::Descending;
    _properties->sortColumn = sortColumn;
    _properties->sortOptions = static_cast<KrViewProperties::SortOptions>(options);

//     op()->settingsChanged(KrViewProperties::PropSortMode);
}

void KrView::saveSortMode(KConfigGroup &group)
{
    group.writeEntry("Sort Column", static_cast<int>(_properties->sortColumn));
    group.writeEntry("Descending Sort Order", _properties->sortOptions & KrViewProperties::Descending);
}

void KrView::restoreSortMode(KConfigGroup &group)
{
    int column = group.readEntry("Sort Column", static_cast<int>(KrViewProperties::Name));
    bool isDescending = group.readEntry("Descending Sort Order", false);
    setSortMode(static_cast<KrViewProperties::ColumnType>(column), isDescending);
}

bool KrView::isFiltered(const KFileItem &item)
{
    if (_quickFilterMask.isValid() && _quickFilterMask.indexIn(item.name()) == -1)
        return true;

    bool filteredOut = false;
    bool isDir = item.isDir();
    if (!isDir || (isDir && properties()->filterApplysToDirs)) {
        switch (properties()->filter) {
        case KrViewProperties::All :
            break;
        case KrViewProperties::Custom :
            if (!properties()->filterMask.match(item))
                filteredOut = true;
            break;
        case KrViewProperties::Dirs:
            if (!isDir)
                filteredOut = true;
            break;
        case KrViewProperties::Files:
            if (isDir)
                filteredOut = true;
            break;
        default:
            break;
        }
    }
    return filteredOut;
}

void KrView::setDirLister(AbstractDirLister *lister)
{
    if(_dirLister != lister) {
        clear();
        if(_dirLister)
            QObject::disconnect(_dirLister, 0, op(), 0);
        _dirLister = lister;
    }

    if(!_dirLister)
        return;

    //FIXME - connect all signals, remove obsolete signals
    QObject::disconnect(_dirLister, 0, op(), 0);
//     QObject::connect(_dirLister, SIGNAL(started()), op(), SLOT(started()));
    QObject::connect(_dirLister, SIGNAL(completed()), op(), SLOT(startUpdate()));
    QObject::connect(_dirLister, SIGNAL(clear()), op(), SLOT(cleared()));
    QObject::connect(_dirLister, SIGNAL(newItems(const KFileItemList&)), op(), SLOT(newItems(const KFileItemList&)));
    QObject::connect(_dirLister, SIGNAL(itemsDeleted(const KFileItemList&)), op(), SLOT(itemsDeleted(const KFileItemList&)));
    QObject::connect(_dirLister, SIGNAL(refreshItems(const QList<QPair<KFileItem, KFileItem>>&)), op(), SLOT(refreshItems(const QList<QPair<KFileItem, KFileItem>>&)));
    QObject::connect(_dirLister, SIGNAL(itemDeleted(const QString&)), op(), SLOT(fileDeleted(const QString&)));
    QObject::connect(_dirLister, SIGNAL(refreshItem(KFileItem)), op(), SLOT(refreshItem(KFileItem)));
}

void KrView::setFilter(KrViewProperties::FilterSpec filter, FilterSettings customFilter, bool applyToDirs)
{
    _properties->filter = filter;
    _properties->filterSettings = customFilter;
    _properties->filterMask = customFilter.toQuery();
    _properties->filterApplysToDirs = applyToDirs;
    refresh();
}

void KrView::setFilter(KrViewProperties::FilterSpec filter)
{
    KConfigGroup cfg(_config, "Look&Feel");
    bool rememberSettings = cfg.readEntry("FilterDialogRemembersSettings", _FilterDialogRemembersSettings);
    bool applyToDirs = rememberSettings ? _properties->filterApplysToDirs : false;
    switch (filter) {
    case KrViewProperties::All :
        break;
    case KrViewProperties::Custom :
        {
            FilterDialog dialog(_widget, i18n("Filter Files"), QStringList(i18n("Apply filter to directories")), false);
            dialog.checkExtraOption(i18n("Apply filter to directories"), applyToDirs);
            if(rememberSettings)
                dialog.applySettings(_properties->filterSettings);
            dialog.exec();
            FilterSettings s(dialog.getSettings());
            if(!s.isValid()) // if the user canceled - quit
                return;
            _properties->filterSettings = s;
            _properties->filterMask = s.toQuery();
            applyToDirs = dialog.isExtraOptionChecked(i18n("Apply filter to directories"));
        }
        break;
    default:
        return;
    }
    _properties->filterApplysToDirs = applyToDirs;
    _properties->filter = filter;
    refresh();
}

void KrView::customSelection(bool select)
{
    KConfigGroup grpSvr(_config, "Look&Feel");
    bool includeDirs = grpSvr.readEntry("Mark Dirs", _MarkDirs);

    FilterDialog dialog(0, i18n("Select Files"), QStringList(i18n("Apply selection to directories")), false);
    dialog.checkExtraOption(i18n("Apply selection to directories"), includeDirs);
    dialog.exec();
    KRQuery query = dialog.getQuery();
    // if the user canceled - quit
    if (query.isNull())
        return ;
    includeDirs = dialog.isExtraOptionChecked(i18n("Apply selection to directories"));

    changeSelection(query, select, includeDirs);
}

void KrView::refresh()
{
    KUrl current = currentUrl();
    KUrl::List selection = getSelectedUrls(false);

    clear();

    if(!_dirLister)
        return;

    KFileItemList items;

    foreach(KFileItem item, _dirLister->items()) {
        if(isFiltered(item))
            continue;
        items << item;
    }

    populate(items, !_dirLister->isRoot());

    if(!selection.isEmpty())
        setSelection(selection);

    if (!urlToMakeCurrent().isEmpty())
        setCurrentItem(urlToMakeCurrent());
    else if (!current.isEmpty())
        setCurrentItem(current);

    updatePreviews();

    redraw();

    op()->emitSelectionChanged();
}

void KrView::setSelected(const Item *item, bool select)
{
    if(item == dummyItem())
        return;

    if(select)
        clearSavedSelection();
    intSetSelected(item, select);
}

void KrView::saveSelection()
{
    _savedSelection = getSelectedUrls(false);
    op()->emitRefreshActions();
}

void KrView::restoreSelection()
{
    if(canRestoreSelection())
        setSelection(_savedSelection);
}

void KrView::clearSavedSelection() {
    _savedSelection.clear();
    op()->emitRefreshActions();
}

void KrView::selectRegion(KFileItem item1, KFileItem item2, bool select, bool clearFirst)
{
    KUrl url1, url2;
    if(!item1.isNull())
        url1 = item1.url();
    if(!item2.isNull())
        url2 = item2.url();
    selectRegion(url1, url2, select, clearFirst);
}

QString KrView::itemDescription(KUrl url, bool itemIsUpUrl)
{
    if (itemIsUpUrl)
        return i18n("Climb up the directory tree");

    const Item *item = itemFromUrl(url);

    if(!item)
        return QString();

    QString text = item->name();
    QString comment;
    KMimeType::Ptr mt = KMimeType::mimeType(item->mimetype());
    if (mt)
        comment = mt->comment(url);
    QString myLinkDest = item->linkDest();
    KIO::filesize_t mySize = item->size();

    QString text2 = text;
    mode_t m_fileMode = item->mode();

    if (item->isLink()) {
        QString tmp;
        if (item->isBrokenLink())
            tmp = i18n("(Broken Link!)");
        else if (comment.isEmpty())
            tmp = i18n("Symbolic Link") ;
        else
            tmp = i18n("%1 (Link)", comment);

        text += "->";
        text += myLinkDest;
        text += "  ";
        text += tmp;
    } else if (S_ISREG(m_fileMode)) {
        text = QString("%1").arg(text2) + QString(" (%1)").arg(properties()->humanReadableSize ?
                KRpermHandler::parseSize(item->size()) : KIO::convertSize(mySize));
        text += "  ";
        text += comment;
    } else if (S_ISDIR(m_fileMode)) {
        text += "/  ";
        if (item->size() != 0) {
            text += '(' +
                    (properties()->humanReadableSize ? KRpermHandler::parseSize(item->size()) : KIO::convertSize(mySize)) + ") ";
        }
        text += comment;
    } else {
        text += "  ";
        text += comment;
    }
    return text;
}

bool KrView::quickSearchMatch(const KFileItem &item, QString term)
{
    KConfigGroup grpSvr(_config, "Look&Feel");
    bool caseSensitive = grpSvr.readEntry("Case Sensitive Quicksearch", _CaseSensitiveQuicksearch);

    QRegExp rx(term, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive, QRegExp::Wildcard);

    return (rx.indexIn(item.name()) == 0);
}

KrViewOperator *KrView::createOperator()
{
    return new KrViewOperator(this, _widget);
}

void KrView::prepareForPassive()
{
    _focused = false;
    _operator->prepareForPassive();
}
