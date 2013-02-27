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

#include "krviewer.h"

#include <QDataStream>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QKeyEvent>
#include <QEvent>

#include <kmenubar.h>
#include <kmimetype.h>
#include <klocale.h>
#include <kparts/part.h>
#include <kparts/componentfactory.h>
#include <kmessagebox.h>
#include <klibloader.h>
#include <kio/netaccess.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <kstatusbar.h>
#include <kdebug.h>
#include <kde_file.h>
#include <khtml_part.h>
#include <kprocess.h>
#include <kfileitem.h>
#include <ktoolbar.h>
#include <kstandardaction.h>
#include <kshell.h>

#include "../krglobal.h"
#include "../defaults.h"
#include "../kicons.h"
#include "panelviewer.h"

#define VIEW_ICON     "zoom-original"
#define EDIT_ICON     "edit"
#define MODIFIED_ICON "document-save-as"

#define CHECK_MODFIED_INTERVAL 500

/*
NOTE: Currently the code expects PanelViewer::openUrl() to be called only once
      in the panel viewer's life time - otherwise unexpected things might happen.
*/

QList<KrViewer *> KrViewer::viewers;

KrViewer::KrViewer(QWidget *parent) :
        KParts::MainWindow(parent, (Qt::WindowFlags)KDE_DEFAULT_WINDOWFLAGS), manager(this, this), tabBar(this),
        reservedKeys(), reservedKeyActions(), sizeX(-1), sizeY(-1)
{
    //setWFlags(Qt::WType_TopLevel | WDestructiveClose);
    setXMLFile("krviewer.rc");   // kpart-related xml file
    setHelpMenuEnabled(false);

    connect(&manager, SIGNAL(activePartChanged(KParts::Part*)),
            this, SLOT(createGUI(KParts::Part*)));
    connect(&tabBar, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(tabChanged(QWidget*)));
    connect(&tabBar, SIGNAL(closeRequest(QWidget *)),
            this, SLOT(tabCloseRequest(QWidget*)));

    tabBar.setDocumentMode(true);
    tabBar.setMovable(true);
    tabBar.setTabReorderingEnabled(false);
    tabBar.setAutomaticResizeTabs(true);
// "edit"
// "document-save-as"
    setCentralWidget(&tabBar);

    printAction = KStandardAction::print(this, SLOT(print()), 0);
    copyAction = KStandardAction::copy(this, SLOT(copy()), 0);

    viewerMenu = new QMenu(this);
    viewerMenu->addAction(i18n("&Generic Viewer"), this, SLOT(viewGeneric()))->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_G);
    viewerMenu->addAction(i18n("&Text Viewer"), this, SLOT(viewText()))->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_T);
    viewerMenu->addAction(i18n("&Hex Viewer"), this, SLOT(viewHex()))->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_H);
    viewerMenu->addAction(i18n("&Lister"), this, SLOT(viewLister()))->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_L);
    viewerMenu->addSeparator();
    viewerMenu->addAction(i18n("Text &Editor"), this, SLOT(editText()))->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_E);
    viewerMenu->addSeparator();
    viewerMenu->addAction(i18n("&Next Tab"), this, SLOT(nextTab()))->setShortcut(Qt::ALT + Qt::Key_Right);
    viewerMenu->addAction(i18n("&Previous Tab"), this, SLOT(prevTab()))->setShortcut(Qt::ALT + Qt::Key_Left);

    detachAction = viewerMenu->addAction(i18n("&Detach Tab"), this, SLOT(detachTab()));
    detachAction->setShortcut(Qt::META + Qt::Key_D);
    //no point in detaching only one tab..
    detachAction->setEnabled(false);
    viewerMenu->addSeparator();
    QList<QAction *> actList = menuBar()->actions();
    bool hasPrint = false, hasCopy = false;
    foreach(QAction *a, actList) {
        if (a->shortcut().matches(printAction->shortcut().primary()) != QKeySequence::NoMatch)
            hasPrint = true;
        if (a->shortcut().matches(copyAction->shortcut().primary()) != QKeySequence::NoMatch)
            hasCopy = true;
    }
    QAction *printAct = viewerMenu->addAction(printAction->icon(), printAction->text(), this, SLOT(print()));
    if (hasPrint)
        printAct->setShortcut(printAction->shortcut().primary());
    QAction *copyAct = viewerMenu->addAction(copyAction->icon(), copyAction->text(), this, SLOT(copy()));
    if (hasCopy)
        copyAct->setShortcut(copyAction->shortcut().primary());
    viewerMenu->addSeparator();
    (tabClose = viewerMenu->addAction(i18n("&Close Current Tab"), this, SLOT(tabCloseRequest())))->setShortcut(Qt::Key_Escape);
    (closeAct = viewerMenu->addAction(i18n("&Quit"), this, SLOT(close())))->setShortcut(Qt::CTRL + Qt::Key_Q);

    tabBar.setHoverCloseButton(true);

    checkModified();

    KConfigGroup group(krConfig, "KrViewerWindow");
    int sx = group.readEntry("Window Width", -1);
    int sy = group.readEntry("Window Height", -1);

    if (sx != -1 && sy != -1)
        resize(sx, sy);
    else
        resize(900, 700);

    if (group.readEntry("Window Maximized",  false)) {
        setWindowState(windowState() | Qt::WindowMaximized);
    }

    // filtering out the key events
    menuBar() ->installEventFilter(this);
}

KrViewer::~KrViewer()
{
    disconnect(&manager, SIGNAL(activePartChanged(KParts::Part*)),
               this, SLOT(createGUI(KParts::Part*)));

    viewers.removeAll(this);

    // close tabs before deleting tab bar - this avoids Qt bug 26115
    // https://bugreports.qt-project.org/browse/QTBUG-26115
    while(tabBar.count())
        tabCloseRequest(tabBar.currentWidget(), true);

    delete printAction;
    delete copyAction;
}

void KrViewer::configureDeps()
{
    PanelEditor::configureDeps();
}

void KrViewer::createGUI(KParts::Part* part)
{
    KParts::MainWindow::createGUI(part);

    updateActions();

    toolBar() ->show();
    statusBar() ->show();

    // the KParts part may override the viewer shortcuts. We prevent it
    // by installing an event filter on the menuBar() and the part
    reservedKeys.clear();
    reservedKeyActions.clear();

    QList<QAction *> list = viewerMenu->actions();
    // getting the key sequences of the viewer menu
    for (int w = 0; w != list.count(); w++) {
        QAction *act = list[ w ];
        QList<QKeySequence> sequences = act->shortcuts();
        if (!sequences.isEmpty()) {
            for (int i = 0; i != sequences.count(); i++) {
                reservedKeys.push_back(sequences[ i ]);
                reservedKeyActions.push_back(act);
            }
        }
    }

    // and "fix" the menubar
    QList<QAction *> actList = menuBar()->actions();
    viewerMenu->setTitle(i18n("&KrViewer"));
    QAction * act = menuBar() ->addMenu(viewerMenu);
    act->setData(QVariant(70));
    menuBar() ->show();
}

bool KrViewer::eventFilter(QObject * /* watched */, QEvent * e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent* ke = (QKeyEvent*) e;
        if (reservedKeys.contains(ke->key())) {
            ke->accept();

            QAction *act = reservedKeyActions[ reservedKeys.indexOf(ke->key())];
            if (act != 0) {
                // don't activate the close functions immediately!
                // it can cause crash
                if (act == tabClose)
                    QTimer::singleShot(0, this, SLOT(tabCloseRequest()));
                else if (act == closeAct)
                    QTimer::singleShot(0, this, SLOT(close()));
                else {
                    act->activate(QAction::Trigger);
                }
            }
            return true;
        }
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = (QKeyEvent*) e;
        if (reservedKeys.contains(ke->key())) {
            ke->accept();
            return true;
        }
    }
    return false;
}
void KrViewer::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_F10:
        close();
        break;
    case Qt::Key_Escape:
        tabCloseRequest();
        break;
    default:
        e->ignore();
        break;
    }
}

KrViewer* KrViewer::getViewer(bool new_window)
{
    if (!new_window) {
        if (viewers.isEmpty()) {
            viewers.prepend(new KrViewer());   // add to first (active)
        } else {
            if (viewers.first()->isMinimized()) { // minimized? -> show it again
                viewers.first()->setWindowState((viewers.first()->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
                viewers.first()->show();
            }
            viewers.first()->raise();
            viewers.first()->activateWindow();
        }
        return viewers.first();
    } else {
        KrViewer *newViewer = new KrViewer();
        viewers.prepend(newViewer);
        return newViewer;
    }
}

void KrViewer::view(KUrl url, QWidget * parent)
{
    KConfigGroup group(krConfig, "General");
    bool defaultWindow = group.readEntry("View In Separate Window", _ViewInSeparateWindow);

    view(url, Default, defaultWindow, parent);
}

void KrViewer::view(KUrl url, Mode mode,  bool new_window, QWidget * parent)
{
    KrViewer* viewer = getViewer(new_window);
    viewer->viewInternal(url, mode, parent);
    viewer->show();
}

void KrViewer::edit(KUrl url, QWidget * parent)
{
    edit(url, Text, -1, parent);
}

void KrViewer::edit(KUrl url, Mode mode, int new_window, QWidget * parent)
{
    KConfigGroup group(krConfig, "General");
    QString editor = group.readEntry("Editor", _Editor);

    if (new_window == -1)
        new_window = group.readEntry("View In Separate Window", _ViewInSeparateWindow);

    if (editor != "internal editor" && !editor.isEmpty()) {
        KProcess proc;
        QStringList cmdArgs = KShell::splitArgs(editor, KShell::TildeExpand);
        if (cmdArgs.isEmpty()) {
            KMessageBox::error(krMainWindow,
                               i18nc("Arg is a string containing the bad quoting.",
                                     "Bad quoting in editor command:\n%1", editor));
            return;
        }
        // if the file is local, pass a normal path and not a url. this solves
        // the problem for editors that aren't url-aware
        proc << cmdArgs << url.pathOrUrl();
        if (!proc.startDetached())
            KMessageBox::sorry(krMainWindow, i18n("Can not open \"%1\"", editor));
        return ;
    }

    KrViewer* viewer = getViewer(new_window);
    viewer->editInternal(url, mode, parent);
    viewer->show();
}

void KrViewer::addTab(PanelViewerBase *pvb)
{
    tabBar.addTab(pvb, makeTabIcon(pvb), makeTabText(pvb));
    tabBar.setCurrentIndex(tabBar.indexOf(pvb));
    tabBar.setTabToolTip(tabBar.indexOf(pvb), makeTabToolTip(pvb));

    updateActions();

    // now we can offer the option to detach tabs (we have more than one)
    if (tabBar.count() > 1) {
        detachAction->setEnabled(true);
    }

    tabBar.show();

    connect(pvb, SIGNAL(openUrlFinished(PanelViewerBase*, bool)),
                 SLOT(openUrlFinished(PanelViewerBase*, bool)));

    connect(pvb, SIGNAL(urlChanged(PanelViewerBase *, const KUrl &)),
            this,  SLOT(tabURLChanged(PanelViewerBase *, const KUrl &)));
}

void KrViewer::tabURLChanged(PanelViewerBase *pvb, const KUrl & url)
{
    Q_UNUSED(url)
    refreshTab(pvb);
}

void KrViewer::openUrlFinished(PanelViewerBase *pvb, bool success)
{
    if (success) {
        KParts::ReadOnlyPart *part = pvb->part();
        if (part) {
            if (!isPartAdded(part))
                addPart(part);
            if (tabBar.currentWidget() == pvb) {
                manager.setActivePart(part);
                if (part->widget())
                    part->widget()->setFocus();
            }
        }
    }
}

void KrViewer::tabChanged(QWidget* w)
{
    if (w == 0)
        return;

    KParts::ReadOnlyPart *part = static_cast<PanelViewerBase*>(w)->part();
    if (part && isPartAdded(part)) {
        manager.setActivePart(part);
        if (part->widget())
            part->widget()->setFocus();
    } else
        manager.setActivePart(0);


    // set this viewer to be the main viewer
    if (viewers.removeAll(this)) viewers.prepend(this);      // move to first
}

void KrViewer::tabCloseRequest(QWidget *w, bool force)
{
    if (!w) return;

    // important to save as returnFocusTo will be cleared at removePart
    QWidget * returnFocusToThisWidget = returnFocusTo;

    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(w);

    if (!force && !pvb->queryClose())
        return;

    if (pvb->part() && isPartAdded(pvb->part()))
        removePart(pvb->part());

    disconnect(pvb, 0, this, 0);

    pvb->closeUrl();

    tabBar.removePage(w);

    delete pvb;
    w = pvb = 0;

    if (tabBar.count() <= 0) {
        if (returnFocusToThisWidget) {
            returnFocusToThisWidget->raise();
            returnFocusToThisWidget->activateWindow();
        } else {
            krMainWindow->raise();
            krMainWindow->activateWindow();
        }

        QTimer::singleShot(0, this, SLOT(close()));

        return;
    } else if (tabBar.count() == 1) {
        //no point in detaching only one tab..
        detachAction->setEnabled(false);
    }

    if (returnFocusToThisWidget) {
        returnFocusToThisWidget->raise();
        returnFocusToThisWidget->activateWindow();
    }
}

void KrViewer::tabCloseRequest()
{
    tabCloseRequest(tabBar.currentWidget());
}

bool KrViewer::queryClose()
{
    KConfigGroup group(krConfig, "KrViewerWindow");

    group.writeEntry("Window Width", sizeX);
    group.writeEntry("Window Height", sizeY);
    group.writeEntry("Window Maximized", isMaximized());

    for (int i = 0; i != tabBar.count(); i++) {
        PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.widget(i));
        if (!pvb)
            continue;

        tabBar.setCurrentIndex(i);

        if (!pvb->queryClose())
            return false;
    }
    return true;
}

bool KrViewer::queryExit()
{
    return true; // don't let the reference counter reach zero
}

void KrViewer::viewGeneric()
{
    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.currentWidget());
    if (pvb)
        viewInternal(pvb->url(), Generic);
}

void KrViewer::viewText()
{
    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.currentWidget());
    if (pvb)
        viewInternal(pvb->url(), Text);
}

void KrViewer::viewLister()
{
    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.currentWidget());
    if (pvb)
        viewInternal(pvb->url(), Lister);
}

void KrViewer::viewHex()
{
    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.currentWidget());
    if (pvb)
        viewInternal(pvb->url(), Hex);
}

void KrViewer::editText()
{
    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.currentWidget());
    if (pvb)
        editInternal(pvb->url(), Text);
}

void KrViewer::checkModified()
{
    QTimer::singleShot(CHECK_MODFIED_INTERVAL, this, SLOT(checkModified()));

    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.currentWidget());
    if (pvb)
        refreshTab(pvb);
}

void KrViewer::refreshTab(PanelViewerBase* pvb)
{
    int ndx = tabBar.indexOf(pvb);
    tabBar.setTabText(ndx, makeTabText(pvb));
    tabBar.setTabIcon(ndx, makeTabIcon(pvb));
    tabBar.setTabToolTip(ndx, makeTabToolTip(pvb));
}

void KrViewer::nextTab()
{
    int index = (tabBar.currentIndex() + 1) % tabBar.count();
    tabBar.setCurrentIndex(index);
}

void KrViewer::prevTab()
{
    int index = (tabBar.currentIndex() - 1) % tabBar.count();
    while (index < 0) index += tabBar.count();
    tabBar.setCurrentIndex(index);
}

void KrViewer::detachTab()
{
    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.currentWidget());
    if (!pvb) return;

    KrViewer* viewer = getViewer(true);

    bool wasPartAdded = false;
    KParts::ReadOnlyPart *part = pvb->part();

    if (part && isPartAdded(part)) {
        wasPartAdded = true;
        removePart(part);
    }

    disconnect(pvb, 0, this, 0);

    tabBar.removePage(pvb);

    if (tabBar.count() == 1) {
        //no point in detaching only one tab..
        detachAction->setEnabled(false);
    }

    pvb->setParent(&viewer->tabBar);
    pvb->move(QPoint(0, 0));

    viewer->addTab(pvb);

    if (wasPartAdded) {
        viewer->addPart(part);
        if (part->widget())
            part->widget()->setFocus();
    }

    viewer->show();
}

void KrViewer::windowActivationChange(bool /* oldActive */)
{
    if (isActiveWindow())
        if (viewers.removeAll(this)) viewers.prepend(this);      // move to first
}

void KrViewer::print()
{
    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.currentWidget());
    if (!pvb || !pvb->part() || !isPartAdded(pvb->part()))
        return;

    KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject(pvb->part());
    if (ext && ext->isActionEnabled("print"))
        Invoker(ext, SLOT(print())).invoke();
}

void KrViewer::copy()
{
    PanelViewerBase* pvb = static_cast<PanelViewerBase*>(tabBar.currentWidget());
    if (!pvb || !pvb->part() || !isPartAdded(pvb->part()))
        return;

    KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject(pvb->part());
    if (ext && ext->isActionEnabled("copy"))
        Invoker(ext, SLOT(copy())).invoke();
}

void KrViewer::updateActions()
{
    QList<QAction *> actList = toolBar()->actions();
    bool hasPrint = false, hasCopy = false;
    foreach(QAction *a, actList) {
        if (a->text() == printAction->text())
            hasPrint = true;
        if (a->text() == copyAction->text())
            hasCopy = true;
    }
    if (!hasPrint)
        toolBar()->addAction(printAction->icon(), printAction->text(), this, SLOT(print()));
    if (!hasCopy)
        toolBar()->addAction(copyAction->icon(), copyAction->text(), this, SLOT(copy()));
}

bool KrViewer::isPartAdded(KParts::Part* part)
{
    return manager.parts().contains(part);
}

void KrViewer::resizeEvent(QResizeEvent *e)
{
    if (!isMaximized()) {
        sizeX = e->size().width();
        sizeY = e->size().height();
    }

    KParts::MainWindow::resizeEvent(e);
}

QString KrViewer::makeTabText(PanelViewerBase *pvb)
{
    QString fileName = pvb->url().fileName();
    if (pvb->isModified())
        fileName.prepend("*");

    return pvb->isEditor() ?
            i18nc("filename (filestate)", "%1 (Editing)", fileName) :
            i18nc("filename (filestate)", "%1 (Viewing)", fileName);
}

QString KrViewer::makeTabToolTip(PanelViewerBase *pvb)
{
    QString url = pvb->url().pathOrUrl();
    return pvb->isEditor() ?
            i18nc("filestate: filename", "Editing: %1", url) :
            i18nc("filestate: filename", "Viewing: %1", url);
}

QIcon KrViewer::makeTabIcon(PanelViewerBase *pvb)
{
    QString iconName;
    if (pvb->isModified())
        iconName = MODIFIED_ICON;
    else if (pvb->isEditor())
        iconName = EDIT_ICON;
    else
        iconName = VIEW_ICON;

    return QIcon(krLoader->loadIcon(iconName, KIconLoader::Small));
}

void KrViewer::addPart(KParts::ReadOnlyPart *part)
{
    Q_ASSERT(part);
    Q_ASSERT(!isPartAdded(part));

    if (isPartAdded(part)) {
        kDebug()<<"part already added:"<<part;
        return;
    }

    connect(part, SIGNAL(setStatusBarText(const QString&)),
            this, SLOT(slotSetStatusBarText(const QString&)));
    // filtering out the key events
    part->installEventFilter(this);

    manager.addPart(part, false); // don't automatically set active part
}

void KrViewer::removePart(KParts::ReadOnlyPart *part)
{
    Q_ASSERT(part);
    Q_ASSERT(isPartAdded(part));

    if (isPartAdded(part)) {
        disconnect(part, 0, this, 0);
        part->removeEventFilter(this);
        manager.removePart(part);
    } else
        kDebug()<<"part hasn't been added:"<<part;
}

void KrViewer::viewInternal(KUrl url, Mode mode, QWidget *parent)
{
    returnFocusTo = parent;

    PanelViewerBase* viewWidget = new PanelViewer(&tabBar, mode);

    addTab(viewWidget);
    viewWidget->openUrl(url);
}

void KrViewer::editInternal(KUrl url, Mode mode, QWidget * parent)
{
    returnFocusTo = parent;

    PanelViewerBase* editWidget = new PanelEditor(&tabBar, mode);

    addTab(editWidget);
    editWidget->openUrl(url);
}

#include "krviewer.moc"
