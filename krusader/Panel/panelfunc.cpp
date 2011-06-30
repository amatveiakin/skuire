/***************************************************************************
                            panelfunc.cpp
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

#include "panelfunc.h"

#include <unistd.h>

#include <QtCore/QEventLoop>
#include <QtGui/QClipboard>
#include <QList>
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

#include <kuiserverjobtracker.h>
#include <klocale.h>
#include <kprocess.h>
#include <kpropertiesdialog.h>
#include <kmessagebox.h>
#include <kcursor.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kurl.h>
#include <ktar.h>
#include <krun.h>
#include <kinputdialog.h>
#include <kdebug.h>
#include <kio/netaccess.h>
#include <kio/jobuidelegate.h>
#include <ktempdir.h>
#include <kdesktopfile.h>
#include <ktoggleaction.h>
#include <kurlcompletion.h>
#include <kmimetypetrader.h>
#include <kopenwithdialog.h>
#include <kshell.h>

#include "dirhistoryqueue.h"
#include "krcalcspacedialog.h"
#include "listpanel.h"
#include "krerrordisplay.h"
#include "listpanelactions.h"
#include "quickfilter.h"
#include "urlrequester.h"
#include "../resources.h"
#include "../krglobal.h"
#include "../krslots.h"
#include "../kractions.h"
#include "../defaults.h"
#include "../abstractpanelmanager.h"
#include "../krservices.h"
#include "../VFS/vfiledirlister.h"
#include "../VFS/vfile.h"
#include "../VFS/vfs.h"
#include "../VFS/virt_vfs.h"
#include "../VFS/krarchandler.h"
#include "../VFS/krpermhandler.h"
#include "../VFS/krvfshandler.h"
#include "../VFS/preservingcopyjob.h"
#include "../VFS/virtualcopyjob.h"
#include "../VFS/packjob.h"
#include "../Dialogs/packgui.h"
#include "../Dialogs/krdialogs.h"
#include "../Dialogs/krpleasewait.h"
#include "../Dialogs/krspwidgets.h"
#include "../Dialogs/checksumdlg.h"
#include "../KViewer/krviewer.h"
#include "../GUI/syncbrowsebutton.h"
#include "../Queue/queue_mgr.h"

ListPanelFunc::ListPanelFunc(ListPanel *parent) : QObject(parent),
        panel(parent), vfsP(0), urlManuallyEntered(false)
{
    dirLister = new VfileDirLister();
    history = new DirHistoryQueue(panel);
    delayTimer.setSingleShot(true);
    connect(&delayTimer, SIGNAL(timeout()), this, SLOT(doRefresh()));
}

ListPanelFunc::~ListPanelFunc()
{
    if (vfsP) {
        if (vfsP->vfs_canDelete())
            delete vfsP;
        else {
            connect(vfsP, SIGNAL(deleteAllowed()), vfsP, SLOT(deleteLater()));
            vfsP->vfs_requestDelete();
        }
    }
    delete history;
    delete dirLister;
}

void ListPanelFunc::urlEntered(const QString &url)
{
    urlEntered(KUrl(
                // KUrlRequester is buggy: it should return a string containing "/home/shie/downloads"
                // but it returns "~/downloads" which is parsed incorrectly by KUrl.
                // replacedPath should replace ONLY $HOME and environment variables
                panel->origin->completionObject()->replacedPath(url)));
}

void ListPanelFunc::urlEntered(const KUrl &url)
{
    openUrl(url, QString(), true);
}

bool ListPanelFunc::isSyncing(const KUrl &url)
{
    if(otherFunc()->otherFunc() == this &&
       panel->otherPanel()->gui->syncBrowseButton->state() == SYNCBROWSE_CD &&
       !otherFunc()->syncURL.isEmpty() &&
       otherFunc()->syncURL == url)
        return true;

    return false;
}
#if 0
//FIXME: see if this is still needed
void ListPanelFunc::popErronousUrl()
{
    KUrl current = urlStack.last();
    while (urlStack.count() != 0) {
        KUrl url = urlStack.takeLast();
        if (!current.equals(url)) {
            immediateOpenUrl(url, true);
            return;
        }
    }
    immediateOpenUrl(KUrl(ROOT_DIR), true);
}
#endif
KUrl ListPanelFunc::cleanPath(const KUrl &urlIn)
{
    KUrl url = urlIn;
    url.cleanPath();
    if (!url.isValid() || url.isRelative()) {
        if (url.url() == "~")
            url = KUrl(QDir::homePath());
        else if (!url.url().startsWith('/')) {
            // possible relative URL - translate to full URL
            url = files()->vfs_getOrigin();
            url.addPath(urlIn.url());
        }
    }
    url.cleanPath();
    return url;
}

void ListPanelFunc::openUrl(const KUrl& url, const QString& nameToMakeCurrent,
                            bool manuallyEntered)
{
    if (panel->syncBrowseButton->state() == SYNCBROWSE_CD) {
        //do sync-browse stuff....
        if(syncURL.isEmpty())
            syncURL = panel->otherPanel()->virtualPath();

        syncURL.addPath(KUrl::relativeUrl(panel->virtualPath().url() + '/', url.url()));
        syncURL.cleanPath();
        panel->otherPanel()->gui->setLocked(false);
        otherFunc()->openUrlInternal(syncURL, nameToMakeCurrent, false, false, false);
    }
    openUrlInternal(url, nameToMakeCurrent, false, false, manuallyEntered);
}

void ListPanelFunc::immediateOpenUrl(const KUrl& url, bool disableLock)
{
    openUrlInternal(url, QString(), true, disableLock, false);
}

void ListPanelFunc::openUrlInternal(const KUrl& url, const QString& nameToMakeCurrent,
                                    bool immediately, bool disableLock, bool manuallyEntered)
{
    KUrl cleanUrl = cleanPath(url);

    if (!disableLock && panel->isLocked() &&
            !files()->vfs_getOrigin().equals(cleanUrl, KUrl::CompareWithoutTrailingSlash)) {
        panel->_manager->newTab(url);
        urlManuallyEntered = false;
        return;
    }

    urlManuallyEntered = manuallyEntered;

    history->add(cleanUrl, nameToMakeCurrent);

    if(immediately)
        doRefresh();
    else
        refresh();
}

void ListPanelFunc::refresh()
{
    panel->inlineRefreshCancel();
    delayTimer.start(0); // to avoid qApp->processEvents() deadlock situaltion
}

void ListPanelFunc::doRefresh()
{
    delayTimer.stop();

    KUrl url = history->currentUrl();

    if(!url.isValid()) {
        //FIXME go back in history here ?
        panel->slotStartUpdate();  // refresh the panel
        urlManuallyEntered = false;
        return ;
    }

    panel->inlineRefreshCancel();

    // if we are not refreshing to current URL
    bool isEqualUrl = files()->vfs_getOrigin().equals(url, KUrl::CompareWithoutTrailingSlash);

    if (!isEqualUrl) {
        panel->setCursor(Qt::WaitCursor);
        panel->view->op()->stopQuickFilter(false);
        panel->view->clearSavedSelection();
    }

    if(panel->vfsError)
        panel->vfsError->hide();

    bool refreshFailed = false;
    while (true) {
        KUrl u = history->currentUrl();

        isEqualUrl = files()->vfs_getOrigin().equals(u, KUrl::CompareWithoutTrailingSlash);

        vfs* v = KrVfsHandler::getVfs(u, panel, files());
        v->setParentWindow(krMainWindow);
        v->setMountMan(&krMtMan);
        if (v != vfsP) {
            dirLister->setFiles(0);

            // disconnect older signals
            disconnect(vfsP, 0, panel, 0);

            if (vfsP->vfs_canDelete())
                delete vfsP;
            else {
                connect(vfsP, SIGNAL(deleteAllowed()), vfsP, SLOT(deleteLater()));
                vfsP->vfs_requestDelete();
            }
            vfsP = v; // v != 0 so this is safe
        } else {
            if (vfsP->vfs_isBusy()) {
                delayTimer.start(100);    /* if vfs is busy try refreshing later */
                return;
            }
        }
        // (re)connect vfs signals
        disconnect(files(), 0, panel, 0);
        connect(files(), SIGNAL(startUpdate()), panel, SLOT(slotStartUpdate()));
        connect(files(), SIGNAL(incrementalRefreshFinished(const KUrl&)),
                panel, SLOT(slotGetStats(const KUrl&)));
        connect(files(), SIGNAL(startJob(KIO::Job*)),
                panel, SLOT(slotJobStarted(KIO::Job*)));
        connect(files(), SIGNAL(error(QString)),
                panel, SLOT(slotVfsError(QString)));
        connect(files(), SIGNAL(trashJobStarted(KIO::Job*)),
                this, SLOT(trashJobStarted(KIO::Job*)));

        dirLister->setFiles(files());

        if(isSyncing(url))
            vfsP->vfs_setQuiet(true);

        if(!history->currentItem().isEmpty() && isEqualUrl) {
            // if the url we're refreshing into is the current one, then the
            // partial refresh will not generate the needed signals to actually allow the
            // view to use nameToMakeCurrent. do it here instead (patch by Thomas Jarosch)
            panel->view->setCurrentItem(history->currentItem());
            panel->view->makeItemVisible(panel->view->currentUrl());
        }
        panel->view->setNameToMakeCurrent(history->currentItem());

        int savedHistoryState = history->state();

        if (vfsP->vfs_refresh(u)) {
            // update the history, as the actual url might differ from the one requested
            history->setCurrentUrl(vfsP->vfs_getOrigin());
            break; // we have a valid refreshed URL now
        }

        refreshFailed = true;

        panel->view->setNameToMakeCurrent(QString());

        if(history->state() != savedHistoryState) // don't go back if the history was touched
            break;
        // prevent repeated error messages
        if (vfsP->vfs_isDeleting())
            break;
        if(!history->goBack())
            break;
        vfsP->vfs_setQuiet(true);
    }
    vfsP->vfs_setQuiet(false);
    panel->view->setNameToMakeCurrent(QString());

    // on local file system change the working directory
    if (files() ->vfs_getType() == vfs::VFS_NORMAL)
        QDir::setCurrent(KrServices::getPath(files()->vfs_getOrigin()));

    // see if the open url operation failed, and if so,
    // put the attempted url in the origin bar and let the user change it
    if (refreshFailed) {
        if(isSyncing(url))
            panel->otherPanel()->gui->syncBrowseButton->setChecked(false);
        else if(urlManuallyEntered) {
            panel->origin->setUrl(url.prettyUrl());
            if(panel == ACTIVE_PANEL)
                panel->origin->edit();
        }
    }

    if(otherFunc()->otherFunc() == this)  // not true if our tab is not active
        otherFunc()->syncURL = KUrl();

    urlManuallyEntered = false;

    refreshActions();
}

void ListPanelFunc::redirectLink()
{
    if (files() ->vfs_getType() != vfs::VFS_NORMAL) {
        KMessageBox::sorry(krMainWindow, i18n("You can edit links only on local file systems"));
        return ;
    }

    vfile *vf = files() ->vfs_search(panel->getCurrentName());
    if (!vf)
        return ;

    QString file = files() ->vfs_getFile(vf->vfile_getName()).path(KUrl::RemoveTrailingSlash);
    QString currentLink = vf->vfile_getSymDest();
    if (currentLink.isEmpty()) {
        KMessageBox::sorry(krMainWindow, i18n("The current file is not a link, so it cannot be redirected."));
        return ;
    }

    // ask the user for a new destination
    bool ok = false;
    QString newLink =
        KInputDialog::getText(i18n("Link Redirection"),
                              i18n("Please enter the new link destination:"), currentLink, &ok, krMainWindow);

    // if the user canceled - quit
    if (!ok || newLink == currentLink)
        return ;
    // delete the current link
    if (unlink(file.toLocal8Bit()) == -1) {
        KMessageBox::sorry(krMainWindow, i18n("Cannot remove old link: %1", file));
        return ;
    }
    // try to create a new symlink
    if (symlink(newLink.toLocal8Bit(), file.toLocal8Bit()) == -1) {
        KMessageBox:: /* --=={ Patch by Heiner <h.eichmann@gmx.de> }==-- */sorry(krMainWindow, i18n("Failed to create a new link: %1", file));
        return ;
    }
}

void ListPanelFunc::krlink(bool sym)
{
    if (files() ->vfs_getType() != vfs::VFS_NORMAL) {
        KMessageBox::sorry(krMainWindow, i18n("You can create links only on local file systems"));
        return ;
    }

    QString name = panel->getCurrentName();

    // ask the new link name..
    bool ok = false;
    QString linkName =
        KInputDialog::getText(i18n("New Link"), i18n("Create a new link to: %1", name), name, &ok, krMainWindow);

    // if the user canceled - quit
    if (!ok || linkName == name)
        return ;

    // if the name is already taken - quit
    if (files() ->vfs_search(linkName) != 0) {
        KMessageBox::sorry(krMainWindow, i18n("A directory or a file with this name already exists."));
        return ;
    }

    if (linkName.left(1) != "/")
        linkName = files() ->vfs_workingDir() + '/' + linkName;

    if (linkName.contains("/"))
        name = files() ->vfs_getFile(name).path(KUrl::RemoveTrailingSlash);

    if (sym) {
        if (symlink(name.toLocal8Bit(), linkName.toLocal8Bit()) == -1)
            KMessageBox::sorry(krMainWindow, i18n("Failed to create a new symlink '%1' to: '%2'",
                               linkName, name));
    } else {
        if (link(name.toLocal8Bit(), linkName.toLocal8Bit()) == -1)
            KMessageBox::sorry(krMainWindow, i18n("Failed to create a new link '%1' to '%2'",
                               linkName, name));
    }
}

void ListPanelFunc::view()
{
    QString fileName = panel->getCurrentName();
    if (fileName.isNull())
        return ;

    // if we're trying to view a directory, just exit
    vfile * vf = files() ->vfs_search(fileName);
    if (!vf || vf->vfile_isDir())
        return ;
    if (!vf->vfile_isReadable()) {
        KMessageBox::sorry(0, i18n("No permissions to view this file."));
        return ;
    }
    // call KViewer.
    KrViewer::view(files() ->vfs_getFile(fileName));
    // nothing more to it!
}

void ListPanelFunc::viewDlg()
{
    // ask the user for a url to view
    KUrl dest = KChooseDir::getDir(i18n("Enter a URL to view:"), panel->virtualPath(), panel->virtualPath());
    if (dest.isEmpty())
        return ;   // the user canceled

    KrViewer::view(dest);   // view the file
}

void ListPanelFunc::terminal()
{
    SLOTS->runTerminal(panel->realPath(), QStringList());
}

void ListPanelFunc::edit()
{
    QString name = panel->getCurrentName();
    if (name.isNull())
        return ;

    if (files() ->vfs_search(name) ->vfile_isDir()) {
        KMessageBox::sorry(krMainWindow, i18n("You cannot edit a directory"));
        return ;
    }

    if (!files() ->vfs_search(name) ->vfile_isReadable()) {
        KMessageBox::sorry(0, i18n("No permissions to edit this file."));
        return ;
    }

    KrViewer::edit(files() ->vfs_getFile(name));
}

void ListPanelFunc::editNew()
{
    if(!fileToCreate.isEmpty())
        return;

    // ask the user for the filename to edit
    fileToCreate = KChooseDir::getDir(i18n("Enter the filename to edit:"), panel->virtualPath(), panel->virtualPath());
    if(fileToCreate.isEmpty())
        return ;   // the user canceled

    KTemporaryFile *tempFile = new KTemporaryFile;
    tempFile->open();

    KIO::CopyJob *job = KIO::move(tempFile->fileName(), fileToCreate);
    job->setUiDelegate(0);
    connect(job, SIGNAL(result(KJob*)), SLOT(slotFileCreated(KJob*)));
    connect(job, SIGNAL(result(KJob*)), tempFile, SLOT(deleteLater()));
}

void ListPanelFunc::slotFileCreated(KJob *job)
{
    if(!job->error() || job->error() == KIO::ERR_FILE_ALREADY_EXIST) {
        KrViewer::edit(fileToCreate);

        if(fileToCreate.upUrl().equals(panel->virtualPath(), KUrl::CompareWithoutTrailingSlash))
            refresh();
        else if(fileToCreate.upUrl().equals(panel->otherPanel()->virtualPath(), KUrl::CompareWithoutTrailingSlash))
            otherFunc()->refresh();
    }
    else
        KMessageBox::sorry(krMainWindow, job->errorString());

    fileToCreate = KUrl();
}

void ListPanelFunc::moveFiles(bool enqueue)
{
    PreserveMode pmode = PM_DEFAULT;
    bool queue = enqueue;

    KUrl::List urls = panel->view->getSelectedUrls(true);
    if(urls.isEmpty())
        return ;  // safety

    QStringList fileNames; //TODO: remove this
    foreach(const KUrl &url, urls)
        fileNames << url.fileName();

    KUrl dest = panel->otherPanel()->virtualPath();
    KUrl virtualBaseURL;

    QString destProtocol = dest.protocol();
    if (destProtocol == "krarc" || destProtocol == "tar" || destProtocol == "zip") {
        KMessageBox::sorry(krMainWindow, i18n("Moving into archive is disabled"));
        return ;
    }

    KConfigGroup group(krConfig, "Advanced");
    if (group.readEntry("Confirm Move", _ConfirmMove)) {
        bool preserveAttrs = group.readEntry("PreserveAttributes", _PreserveAttributes);
        QString s;

        if (urls.count() == 1)
            s = i18n("Move %1 to:", urls.first().fileName());
        else
            s = i18np("Move %1 file to:", "Move %1 files to:", urls.count());

        // ask the user for the copy dest
        virtualBaseURL = getVirtualBaseURL();
        dest = KChooseDir::getDir(s, dest, panel->virtualPath(), queue, preserveAttrs, virtualBaseURL);
        if (dest.isEmpty())
            return ;   // the user canceled
        if (preserveAttrs)
            pmode = PM_PRESERVE_ATTR;
        else
            pmode = PM_NONE;
    }

    if (queue) {
        KIOJobWrapper *job = 0;
        if (!virtualBaseURL.isEmpty()) {
            job = KIOJobWrapper::virtualMove(&fileNames, files(), dest, virtualBaseURL, pmode, true);
            job->connectTo(SIGNAL(result(KJob*)), this, SLOT(refresh()));
            if (dest.equals(panel->otherPanel()->virtualPath(), KUrl::CompareWithoutTrailingSlash))
                job->connectTo(SIGNAL(result(KJob*)), panel->otherPanel()->func, SLOT(refresh()));
        } else {
            // you can rename only *one* file not a batch,
            // so a batch dest must alwayes be a directory
            if (urls.count() > 1) dest.adjustPath(KUrl::AddTrailingSlash);
            job = KIOJobWrapper::move(pmode, urls, dest, true);
            job->setAutoErrorHandlingEnabled(true);
            // refresh our panel when done
            job->connectTo(SIGNAL(result(KJob*)), this, SLOT(refresh()));
            if (dest.equals(panel->virtualPath(), KUrl::CompareWithoutTrailingSlash) ||
                    dest.upUrl().equals(panel->virtualPath(), KUrl::CompareWithoutTrailingSlash))
                // refresh our panel when done
                job->connectTo(SIGNAL(result(KJob*)), this, SLOT(refresh()));
        }
        QueueManager::currentQueue()->enqueue(job);
    } else if (!virtualBaseURL.isEmpty()) {
        // keep the directory structure for virtual paths
        VirtualCopyJob *vjob = new VirtualCopyJob(&fileNames, files(), dest, virtualBaseURL, pmode, KIO::CopyJob::Move, true);
        connect(vjob, SIGNAL(result(KJob*)), this, SLOT(refresh()));
        if (dest.equals(panel->otherPanel()->virtualPath(), KUrl::CompareWithoutTrailingSlash))
            connect(vjob, SIGNAL(result(KJob*)), panel->otherPanel()->func, SLOT(refresh()));
    }
    // if we are not moving to the other panel :
    else if (!dest.equals(panel->otherPanel()->virtualPath(), KUrl::CompareWithoutTrailingSlash)) {
        // you can rename only *one* file not a batch,
        // so a batch dest must alwayes be a directory
        if (urls.count() > 1) dest.adjustPath(KUrl::AddTrailingSlash);
        KIO::Job* job = PreservingCopyJob::createCopyJob(pmode, urls, dest, KIO::CopyJob::Move, false, true);
        job->ui()->setAutoErrorHandlingEnabled(true);
        // refresh our panel when done
        connect(job, SIGNAL(result(KJob*)), this, SLOT(refresh()));
        // and if needed the other panel as well
        if (dest.equals(panel->otherPanel()->virtualPath(), KUrl::CompareWithoutTrailingSlash))
            connect(job, SIGNAL(result(KJob*)), panel->otherPanel()->func, SLOT(refresh()));

    } else { // let the other panel do the dirty job
        //check if copy is supported
        if (!otherFunc() ->files() ->vfs_isWritable()) {
            KMessageBox::sorry(krMainWindow, i18n("You cannot move files to this file system"));
            return ;
        }
        // finally..
        otherFunc() ->files() ->vfs_addFiles(&urls, KIO::CopyJob::Move, files(), "", pmode);
    }
    if(KConfigGroup(krConfig, "Look&Feel").readEntry("UnselectBeforeOperation", _UnselectBeforeOperation)) {
        panel->view->saveSelection();
        panel->view->unselect(KRQuery("*"));
    }
}

// called from SLOTS to begin the renaming process
void ListPanelFunc::rename()
{
    panel->view->renameCurrentItem();
}

// called by signal itemRenamed() from the view to complete the renaming process
void ListPanelFunc::rename(KFileItem item, QString newname)
{
    if (item.name() == newname)
        return ; // do nothing
    panel->view->setNameToMakeCurrentIfAdded(newname);
    // as always - the vfs do the job
    files() ->vfs_rename(item.name(), newname);
}

void ListPanelFunc::mkdir()
{
    // ask the new dir name..
    bool ok = false;
    QString dirName =
        KInputDialog::getText(i18n("New directory"), i18n("Directory's name:"), "", &ok, krMainWindow);

    // if the user canceled - quit
    if (!ok || dirName.isEmpty())
        return ;

    QStringList dirTree = dirName.split('/');

    for (QStringList::Iterator it = dirTree.begin(); it != dirTree.end(); ++it) {
        if (*it == ".")
            continue;
        if (*it == "..") {
            immediateOpenUrl(*it);
            continue;
        }
        // check if the name is already taken
        if (files() ->vfs_search(*it)) {
            // if it is the last dir to be created - quit
            if (*it == dirTree.last()) {
                KMessageBox::sorry(krMainWindow, i18n("A directory or a file with this name already exists."));
                return ;
            }
            // else go into this dir
            else {
                immediateOpenUrl(*it);
                continue;
            }
        }

        panel->view->setNameToMakeCurrent(*it);
        // as always - the vfs do the job
        files() ->vfs_mkdir(*it);
        if (dirTree.count() > 1)
            immediateOpenUrl(*it);
    } // for
}

KUrl ListPanelFunc::getVirtualBaseURL()
{
    if (files()->vfs_getType() != vfs::VFS_VIRT || otherFunc()->files()->vfs_getType() == vfs::VFS_VIRT)
        return KUrl();

    KUrl::List urls = panel->view->getSelectedUrls(true);
    if (urls.isEmpty())
        return KUrl();

    KUrl base = urls[0].upUrl();

    if (base.protocol() == "virt")  // is it a virtual subfolder?
        return KUrl();          // --> cannot keep the directory structure

    for (int i = 1; i < urls.count(); i++) {
        if (base.isParentOf(urls[i]))
            continue;
        if (base.protocol() != urls[i].protocol())
            return KUrl();

        do {
            KUrl oldBase = base;
            base = base.upUrl();
            if (oldBase.equals(base, KUrl::CompareWithoutTrailingSlash))
                return KUrl();
            if (base.isParentOf(urls[i]))
                break;
        } while (true);
    }
    return base;
}

void ListPanelFunc::copyFiles(bool enqueue)
{
    PreserveMode pmode = PM_DEFAULT;
    bool queue = enqueue;

    KUrl::List urls = panel->view->getSelectedUrls(true);
    if (urls.isEmpty())
        return;

    QStringList fileNames; //TODO: remove this
    foreach(const KUrl &url, urls)
        fileNames << url.fileName();

    KUrl dest = panel->otherPanel()->virtualPath();
    KUrl virtualBaseURL;

    // confirm copy
    KConfigGroup group(krConfig, "Advanced");
    if (group.readEntry("Confirm Copy", _ConfirmCopy)) {
        bool preserveAttrs = group.readEntry("PreserveAttributes", _PreserveAttributes);
        QString s;

        if (fileNames.count() == 1)
            s = i18n("Copy %1 to:", fileNames.first());
        else
            s = i18np("Copy %1 file to:", "Copy %1 files to:", fileNames.count());

        // ask the user for the copy dest
        virtualBaseURL = getVirtualBaseURL();
        dest = KChooseDir::getDir(s, dest, panel->virtualPath(), queue, preserveAttrs, virtualBaseURL);
        if (dest.isEmpty())
            return ;   // the user canceled
        if (preserveAttrs)
            pmode = PM_PRESERVE_ATTR;
        else
            pmode = PM_NONE;
    }

    if (queue) {
        KIOJobWrapper *job = 0;
        if (!virtualBaseURL.isEmpty()) {
            job = KIOJobWrapper::virtualCopy(&fileNames, files(), dest, virtualBaseURL, pmode, true);
            job->connectTo(SIGNAL(result(KJob*)), this, SLOT(refresh()));
            if (dest.equals(panel->otherPanel()->virtualPath(), KUrl::CompareWithoutTrailingSlash))
                job->connectTo(SIGNAL(result(KJob*)), panel->otherPanel()->func, SLOT(refresh()));
        } else {
            // you can rename only *one* file not a batch,
            // so a batch dest must alwayes be a directory
            if (fileNames.count() > 1) dest.adjustPath(KUrl::AddTrailingSlash);
            job = KIOJobWrapper::copy(pmode, urls, dest, true);
            job->setAutoErrorHandlingEnabled(true);
            if (dest.equals(panel->virtualPath(), KUrl::CompareWithoutTrailingSlash) ||
                    dest.upUrl().equals(panel->virtualPath(), KUrl::CompareWithoutTrailingSlash))
                // refresh our panel when done
                job->connectTo(SIGNAL(result(KJob*)), this, SLOT(refresh()));
        }
        QueueManager::currentQueue()->enqueue(job);
    } else if (!virtualBaseURL.isEmpty()) {
        // keep the directory structure for virtual paths
        VirtualCopyJob *vjob = new VirtualCopyJob(&fileNames, files(), dest, virtualBaseURL, pmode, KIO::CopyJob::Copy, true);
        connect(vjob, SIGNAL(result(KJob*)), this, SLOT(refresh()));
        if (dest.equals(panel->otherPanel()->virtualPath(), KUrl::CompareWithoutTrailingSlash))
            connect(vjob, SIGNAL(result(KJob*)), panel->otherPanel()->func, SLOT(refresh()));
    }
    // if we are  not copying to the other panel :
    else if (!dest.equals(panel->otherPanel()->virtualPath(), KUrl::CompareWithoutTrailingSlash)) {
        // you can rename only *one* file not a batch,
        // so a batch dest must alwayes be a directory
        if (fileNames.count() > 1) dest.adjustPath(KUrl::AddTrailingSlash);
        KIO::Job* job = PreservingCopyJob::createCopyJob(pmode, urls, dest, KIO::CopyJob::Copy, false, true);
        job->ui()->setAutoErrorHandlingEnabled(true);
        if (dest.equals(panel->virtualPath(), KUrl::CompareWithoutTrailingSlash) ||
                dest.upUrl().equals(panel->virtualPath(), KUrl::CompareWithoutTrailingSlash))
            // refresh our panel when done
            connect(job, SIGNAL(result(KJob*)), this, SLOT(refresh()));
        // let the other panel do the dirty job
    } else {
        //check if copy is supported
        if (!otherFunc() ->files() ->vfs_isWritable()) {
            KMessageBox::sorry(krMainWindow, i18n("You cannot copy files to this file system"));
            return ;
        }
        // finally..
        otherFunc() ->files() ->vfs_addFiles(&urls, KIO::CopyJob::Copy, 0, "", pmode);
    }
    if(KConfigGroup(krConfig, "Look&Feel").readEntry("UnselectBeforeOperation", _UnselectBeforeOperation)) {
        panel->view->saveSelection();
        panel->view->unselect(KRQuery("*"));
    }
}

void ListPanelFunc::deleteFiles(bool reallyDelete)
{
    // check that the you have write perm
    if (!files() ->vfs_isWritable()) {
        KMessageBox::sorry(krMainWindow, i18n("You do not have write permission to this directory"));
        return ;
    }

    KFileItemList items = panel->view->getSelectedItems(true);
    if (items.isEmpty())
        return;

    KConfigGroup gg(krConfig, "General");
    bool trash = gg.readEntry("Move To Trash", _MoveToTrash);
    // now ask the user if he want to delete:
    KConfigGroup group(krConfig, "Advanced");
    if (group.readEntry("Confirm Delete", _ConfirmDelete)) {
        QString s, b;

        if (!reallyDelete && trash && files() ->vfs_getType() == vfs::VFS_NORMAL) {
            s = i18np("Do you really want to move this item to the trash?", "Do you really want to move these %1 items to the trash?", items.count());
            b = i18n("&Trash");
        } else if (files() ->vfs_getType() == vfs::VFS_VIRT && files()->vfs_getOrigin().equals(KUrl("virt:/"), KUrl::CompareWithoutTrailingSlash)) {
            s = i18np("Do you really want to delete this virtual item (physical files stay untouched)?", "Do you really want to delete these %1 virtual items (physical files stay untouched)?", items.count());
            b = i18n("&Delete");
        } else if (files() ->vfs_getType() == vfs::VFS_VIRT) {
            s = i18np("<qt>Do you really want to delete this item <b>physically</b> (not just removing it from the virtual items)?</qt>", "<qt>Do you really want to delete these %1 items <b>physically</b> (not just removing them from the virtual items)?</qt>", items.count());
            b = i18n("&Delete");
        } else {
            s = i18np("Do you really want to delete this item?", "Do you really want to delete these %1 items?", items.count());
            b = i18n("&Delete");
        }

        // show message
        // note: i'm using continue and not yes/no because the yes/no has cancel as default button
        QStringList names;
        foreach(const KFileItem &item, items)
            names << item.name();
        if (KMessageBox::warningContinueCancelList(krMainWindow, s, names,
                i18n("Warning"), KGuiItem(b)) != KMessageBox::Continue)
            return ;
    }
    //we want to warn the user about non empty dir
    // and files he don't have permission to delete
    bool emptyDirVerify = group.readEntry("Confirm Unempty Dir", _ConfirmUnemptyDir);
    emptyDirVerify = ((emptyDirVerify) && (files() ->vfs_getType() == vfs::VFS_NORMAL));

    QDir dir;
    for(KFileItemList::Iterator it = items.begin(); it != items.end(); ) {
        KFileItem &item = *it;
        // verify non-empty dirs delete... (only for normal vfs)
        if (emptyDirVerify && item.isDir() && !item.isLink()) {
            dir.setPath(panel->virtualPath().path() + '/' + (item.name()));
            if (dir.entryList(QDir::TypeMask | QDir::System | QDir::Hidden).count() > 2) {
                switch (KMessageBox::warningYesNoCancel(krMainWindow,
                                                        i18n("<qt><p>Directory <b>%1</b> is not empty!</p><p>Skip this one or Delete All?</p></qt>", item.name()),
                                                        QString(), KGuiItem(i18n("&Skip")), KGuiItem(i18n("&Delete All")))) {
                case KMessageBox::Cancel :
                    return ;
                case KMessageBox::No :
                    emptyDirVerify = false;
                    break;
                case KMessageBox::Yes :
                    it = items.erase(it);
                    continue;
                }
            }
        }
        it++;
    }

    if (items.count() == 0)
        return ;  // nothing to delete

    //TODO: call KIO directly
    // let the vfs do the job...
    QStringList names;
    foreach(const KFileItem &item, items)
            names << item.name();
    files()->vfs_delFiles(&names, reallyDelete);
}

void ListPanelFunc::goInside(KFileItem item)
{
    if(item.isNull())
        return;

    KUrl url = item.url();

    if (item.isDir()) {               // we create a return-pressed event,
        execute(item); // thereby emulating a chdir
    } else if (url.isLocalFile()) {
        bool encrypted;
        QString mime = item.mimetype();
        QString type = KRarcHandler::getType(encrypted, url.path(), mime, false);

        if (KRarcHandler::arcSupported(type)) {    // archive autodetection
            // here we check whether KDE supports tar
            if (type == "-tlz") {
                KTar tapeArchive(url.path());
                if (tapeArchive.open(QIODevice::ReadOnly))
                    url.setProtocol("tar");
                else
                    url.setProtocol("krarc");
            } else if (type == "-tar" || type == "-tgz" || type == "-tbz")
                url.setProtocol("tar");
            else
                url.setProtocol("krarc");
            openUrl(url);
        } else {
            QString protocol = KrServices::registerdProtocol(mime);
            if (protocol == "iso") {
                url.setProtocol(protocol);
                openUrl(url);
            }
        }
    }
}

void ListPanelFunc::runCommand(QString cmd)
{
    krOut<<cmd<<endl;
    QString workdir = panel->virtualPath().isLocalFile() ?
            panel->virtualPath().path() : QDir::homePath();
    if(!KRun::runCommand(cmd, krMainWindow, workdir))
        KMessageBox::error(0, i18n("Couldn't start %1", cmd));
}

void ListPanelFunc::runService(const KService &service, KUrl::List urls)
{
    krOut<<service.name()<<endl;
    QStringList args = KRun::processDesktopExec(service, urls);
    if (args.count())
        runCommand(KShell::joinArgs(args));
    else
        KMessageBox::error(0, i18n("%1 can't open %2", service.name(), urls.toStringList().join(", ")));
}

void ListPanelFunc::displayOpenWithDialog(KUrl::List urls)
{
    KOpenWithDialog dialog(urls, panel);
    dialog.hideRunInTerminal();
    if (dialog.exec()) {
        KService::Ptr service = dialog.service();
        if(!service)
            service = KService::Ptr(new KService(dialog.text(), dialog.text(), QString()));
        runService(*service, urls);
    }
}

void ListPanelFunc::openWith()
{
    KUrl::List urls = panel->view->getSelectedUrls(true);
    if(urls.count())
        displayOpenWithDialog(urls);
}

// this is done when you double click on a file
void ListPanelFunc::execute(KFileItem item)
{
    if(item.isNull())
        return;

    krOut<<item.name()<<endl;

    KUrl url = item.url();
    QString mime = item.mimetype();
    QString protocol = url.isLocalFile() ? KrServices::registerdProtocol(mime) : "";

    if (protocol == "tar" || protocol == "krarc") {
        bool encrypted;
        QString type = KRarcHandler::getType(encrypted, url.path(), mime, false);
        if (!KRarcHandler::arcHandled(type))       // if the specified archive is disabled delete the protocol
            protocol = "";
    }

    if (item.isDir()) {
        panel->view->setNameToMakeCurrent(QString());
        openUrl(url);
    } else if (!protocol.isEmpty()) {
        url.setProtocol(protocol);
        openUrl(url);
    } else {
        if (KRun::isExecutableFile(url, mime))
            runCommand(url.path());
        else {
            KService::Ptr service = KMimeTypeTrader::self()->preferredService(mime);
            if(service)
                runService(*service, KUrl::List(url));
            else
                displayOpenWithDialog(KUrl::List(url));
        }
    }
}

void ListPanelFunc::pack()
{
    KUrl::List urls = panel->view->getSelectedUrls(true);
    if (urls.isEmpty())
        return ; // nothing to pack

    QStringList fileNames; //TODO remove this
    foreach(const KUrl &url, urls)
        fileNames << url.fileName();

    // choose the default name
    QString defaultName = panel->virtualPath().fileName();
    if (defaultName.isEmpty())
        defaultName = "pack";
    if (fileNames.count() == 1)
        defaultName = fileNames.first();
    // ask the user for archive name and packer
    new PackGUI(defaultName, vfs::pathOrUrl(panel->otherPanel()->virtualPath(), KUrl::RemoveTrailingSlash), fileNames.count(), fileNames.first());
    if (PackGUI::type.isEmpty()) {
        return ; // the user canceled
    }

    // check for partial URLs
    if (!PackGUI::destination.contains(":/") && !PackGUI::destination.startsWith('/')) {
        PackGUI::destination = panel->virtualPath().prettyUrl() + '/' + PackGUI::destination;
    }

    QString destDir = PackGUI::destination;
    if (!destDir.endsWith('/'))
        destDir += '/';

    bool packToOtherPanel = (destDir == panel->otherPanel()->virtualPath().prettyUrl(KUrl::AddTrailingSlash));

    // on remote URL-s first pack into a temp file then copy to its right place
    KUrl destURL = KUrl(destDir + PackGUI::filename + '.' + PackGUI::type);
    KTemporaryFile *tempDestFile = 0;
    QString arcFile;
    if (destURL.isLocalFile())
        arcFile = destURL.path();
    else if (destURL.protocol() == "virt") {
        KMessageBox::error(krMainWindow, i18n("Cannot pack files onto a virtual destination!"));
        return;
    } else {
        tempDestFile = new KTemporaryFile();
        tempDestFile->setSuffix(QString(".") + PackGUI::type);
        tempDestFile->open();
        arcFile = tempDestFile->fileName();
        tempDestFile->close(); // necessary to create the filename
        QFile::remove
        (arcFile);
    }

    if (QFileInfo(arcFile).exists()) {
        QString msg = i18n("<qt><p>The archive <b>%1.%2</b> already exists. Do you want to overwrite it?</p><p>All data in the previous archive will be lost!</p></qt>", PackGUI::filename, PackGUI::type);
        if (PackGUI::type == "zip") {
            msg = i18n("<qt><p>The archive <b>%1.%2</b> already exists. Do you want to overwrite it?</p><p>Zip will replace identically named entries in the zip archive or add entries for new names.</p></qt>", PackGUI::filename, PackGUI::type);
        }
        if (KMessageBox::warningContinueCancel(krMainWindow, msg, QString(), KGuiItem(i18n("&Overwrite")))
                == KMessageBox::Cancel)
            return ; // stop operation
    }

    if (PackGUI::queue) {
        KIOJobWrapper *job = KIOJobWrapper::pack(files()->vfs_getOrigin(), destURL, fileNames,
                             PackGUI::type, PackGUI::extraProps, true);
        job->setAutoErrorHandlingEnabled(true);

        if (packToOtherPanel)
            job->connectTo(SIGNAL(result(KJob*)), panel->otherPanel()->func, SLOT(refresh()));

        QueueManager::currentQueue()->enqueue(job);
    } else {
        PackJob * job = PackJob::createPacker(files()->vfs_getOrigin(), destURL, fileNames, PackGUI::type, PackGUI::extraProps);
        job->setUiDelegate(new KIO::JobUiDelegate());
        KIO::getJobTracker()->registerJob(job);
        job->ui()->setAutoErrorHandlingEnabled(true);

        if (packToOtherPanel)
            connect(job, SIGNAL(result(KJob*)), panel->otherPanel()->func, SLOT(refresh()));
    }
}

void ListPanelFunc::testArchive()
{
    KUrl::List urls = panel->view->getSelectedUrls(true);
    if (urls.isEmpty())
        return;

    QStringList fileNames; //TODO remove this
    foreach(const KUrl &url, urls)
        fileNames << url.fileName();

    TestArchiveJob * job = TestArchiveJob::testArchives(files()->vfs_getOrigin(), fileNames);
    job->setUiDelegate(new KIO::JobUiDelegate());
    KIO::getJobTracker()->registerJob(job);
    job->ui()->setAutoErrorHandlingEnabled(true);
}

void ListPanelFunc::unpack()
{
    KUrl::List urls = panel->view->getSelectedUrls(true);
    if (urls.isEmpty())
        return ; // nothing to unpack

    QStringList fileNames; //TODO remove this
    foreach(const KUrl &url, urls)
        fileNames << url.fileName();

    QString s;
    if (fileNames.count() == 1)
        s = i18n("Unpack %1 to:", fileNames[0]);
    else
        s = i18np("Unpack %1 file to:", "Unpack %1 files to:", fileNames.count());

    // ask the user for the copy dest
    bool queue = false;
    KUrl dest = KChooseDir::getDir(s, panel->otherPanel()->virtualPath(), panel->virtualPath(), queue);
    if (dest.isEmpty()) return ;   // the user canceled

    bool packToOtherPanel = (dest.equals(panel->otherPanel()->virtualPath(), KUrl::CompareWithoutTrailingSlash));

    if (queue) {
        KIOJobWrapper *job = KIOJobWrapper::unpack(files()->vfs_getOrigin(), dest, fileNames, true);
        job->setAutoErrorHandlingEnabled(true);

        if (packToOtherPanel)
            job->connectTo(SIGNAL(result(KJob*)), panel->otherPanel()->func, SLOT(refresh()));

        QueueManager::currentQueue()->enqueue(job);
    } else {
        UnpackJob * job = UnpackJob::createUnpacker(files()->vfs_getOrigin(), dest, fileNames);
        job->setUiDelegate(new KIO::JobUiDelegate());
        KIO::getJobTracker()->registerJob(job);
        job->ui()->setAutoErrorHandlingEnabled(true);

        if (packToOtherPanel)
            connect(job, SIGNAL(result(KJob*)), panel->otherPanel()->func, SLOT(refresh()));
    }
}

// a small ugly function, used to prevent duplication of EVERY line of
// code (maybe except 3) from createChecksum and matchChecksum
static void checksum_wrapper(ListPanel *panel, QStringList& args, bool &folders)
{
    KFileItemList items = panel->view->getSelectedItems(true);
    if (items.isEmpty())
        return ; // nothing to do

    // determine if we need recursive mode (md5deep)
    folders = false;
    foreach(const KFileItem &item, items) {
        if (item.isDir())
            folders = true;
        args << item.name();
    }
}

void ListPanelFunc::createChecksum()
{
    QStringList args;
    bool folders;
    checksum_wrapper(panel, args, folders);
    CreateChecksumDlg dlg(args, folders, panel->realPath());
}

void ListPanelFunc::matchChecksum()
{
    QStringList args;
    bool folders;
    checksum_wrapper(panel, args, folders);
    QList<vfile*> checksumFiles = files()->vfs_search(
                                      KRQuery(MatchChecksumDlg::checksumTypesFilter)
                                  );
    MatchChecksumDlg dlg(args, folders, panel->realPath(),
                         (checksumFiles.size() == 1 ? checksumFiles[0]->vfile_getUrl().prettyUrl() : QString()));
}

void ListPanelFunc::calcSpace()
{
    KUrl::List urls = panel->view->getSelectedUrls(false);
    if (urls.isEmpty()) {
        urls = panel->view->getItems().urlList();
        if (urls.isEmpty())
            return ; // nothing to do
    }

    KrCalcSpaceDialog dlg(krMainWindow, panel, urls, false);
    dlg.exec();
    panel->slotUpdateTotals();
}

void ListPanelFunc::calcSpace(KFileItem item)
{
    if(item.isNull())
        return;
    //
    // NOTE: this is buggy incase somewhere down in the folder we're calculating,
    // there's a folder we can't enter (permissions). in that case, the returned
    // size will not be correct.
    //
    KrCalcSpaceDialog dlg(krMainWindow, panel, KUrl::List(item.url()), true);
    dlg.exec();
    panel->slotUpdateTotals();
}

void ListPanelFunc::FTPDisconnect()
{
    // you can disconnect only if connected !
    if (files() ->vfs_getType() == vfs::VFS_FTP) {
        panel->_actions->actFTPDisconnect->setEnabled(false);
        panel->view->setNameToMakeCurrent(QString());
        openUrl(panel->realPath());   // open the last local URL
    }
}

void ListPanelFunc::newFTPconnection()
{
    KUrl url = KRSpWidgets::newFTP();
    // if the user canceled - quit
    if (url.isEmpty())
        return ;

    panel->_actions->actFTPDisconnect->setEnabled(true);
    openUrl(url);
}

void ListPanelFunc::properties()
{
    KFileItemList items = panel->view->getSelectedItems(true);
    if (items.isEmpty())
        return ;

    // Show the properties dialog
    KPropertiesDialog *dlg = new KPropertiesDialog(items, krMainWindow);
    connect(dlg, SIGNAL(applied()), SLOTS, SLOT(refresh()));
    dlg->show();
}

void ListPanelFunc::refreshActions()
{
    panel->updateButtons();

    if(ACTIVE_PANEL != panel)
        return;

    vfs::VFS_TYPE vfsType = files() ->vfs_getType();

    QString protocol = files()->vfs_getOrigin().protocol();
    krRemoteEncoding->setEnabled(protocol == "ftp" || protocol == "sftp" || protocol == "fish" || protocol == "krarc");
    //krMultiRename->setEnabled( vfsType == vfs::VFS_NORMAL );  // batch rename
    //krProperties ->setEnabled( vfsType == vfs::VFS_NORMAL || vfsType == vfs::VFS_FTP ); // file properties

    /*
      krUnpack->setEnabled(true);                            // unpack archive
      krTest->setEnabled(true);                              // test archive
      krSelect->setEnabled(true);                            // select a group by filter
      krSelectAll->setEnabled(true);                         // select all files
      krUnselect->setEnabled(true);                          // unselect by filter
      krUnselectAll->setEnabled( true);                      // remove all selections
      krInvert->setEnabled(true);                            // invert the selection
      krFTPConnect->setEnabled(true);                        // connect to an ftp
      krFTPNew->setEnabled(true);                            // create a new connection
      krAllFiles->setEnabled(true);                          // show all files in list
      krCustomFiles->setEnabled(true);                       // show a custom set of files
      krRoot->setEnabled(true);                              // go all the way up
          krExecFiles->setEnabled(true);                         // show only executables
    */

    panel->_actions->setViewActions[panel->panelType]->setChecked(true);
    panel->_actions->actFTPDisconnect->setEnabled(vfsType == vfs::VFS_FTP);       // disconnect an FTP session
    panel->_actions->actCreateChecksum->setEnabled(vfsType == vfs::VFS_NORMAL);
    panel->_actions->actDirUp->setEnabled(!files()->isRoot());
    panel->_actions->actRoot->setEnabled(!files()->isRoot());
    panel->_actions->actHome->setEnabled(!atHome());
    panel->_actions->actHistoryBackward->setEnabled(history->canGoBack());
    panel->_actions->actHistoryForward->setEnabled(history->canGoForward());
    panel->view->op()->emitRefreshActions();
}

vfs* ListPanelFunc::files()
{
    if (!vfsP)
        vfsP = KrVfsHandler::getVfs(KUrl("/"), panel, 0);
    return vfsP;
}

void ListPanelFunc::copyToClipboard(bool move)
{
    if (files()->vfs_getOrigin().equals(KUrl("virt:/"), KUrl::CompareWithoutTrailingSlash)) {
        if (move)
            KMessageBox::error(krMainWindow, i18n("Cannot cut a virtual URL collection to the clipboard!"));
        else
            KMessageBox::error(krMainWindow, i18n("Cannot copy a virtual URL collection onto the clipboard!"));
        return;
    }

    KUrl::List urls = panel->view->getSelectedUrls(true);
    if (urls.count()) {
        QStringList fileNames; //TODO remove this
        foreach(const KUrl &url, urls)
            fileNames << url.fileName();

        QMimeData *mimeData = new QMimeData;
        mimeData->setData("application/x-kde-cutselection", move ? "1" : "0");
        urls.populateMimeData(mimeData);

        QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

        if (move && files()->vfs_getType() == vfs::VFS_VIRT)
            (static_cast<virt_vfs*>(files()))->vfs_removeFiles(&fileNames);
    }
}

void ListPanelFunc::pasteFromClipboard()
{
    QClipboard * cb = QApplication::clipboard();

    bool move = false;
    const QMimeData *data = cb->mimeData();
    if (data->hasFormat("application/x-kde-cutselection")) {
        QByteArray a = data->data("application/x-kde-cutselection");
        if (!a.isEmpty())
            move = (a.at(0) == '1'); // true if 1
    }

    KUrl::List urls = KUrl::List::fromMimeData(data);
    if (urls.isEmpty())
        return ;

    KUrl destUrl = panel->virtualPath();

    files()->vfs_addFiles(&urls, move ? KIO::CopyJob::Move : KIO::CopyJob::Copy, otherFunc()->files(),
                          "", PM_DEFAULT);
}

ListPanelFunc* ListPanelFunc::otherFunc()
{
    return panel->otherPanel()->func;
}

void ListPanelFunc::trashJobStarted(KIO::Job *job)
{
    connect(job, SIGNAL(result(KJob*)), SLOTS, SLOT(changeTrashIcon()));
}

void ListPanelFunc::historyGotoPos(int pos)
{
    if(history->gotoPos(pos))
        refresh();
}

void ListPanelFunc::historyBackward()
{
    if(history->goBack())
        refresh();
}

void ListPanelFunc::historyForward()
{
    if(history->goForward())
        refresh();
}

void ListPanelFunc::dirUp()
{
    openUrl(files() ->vfs_getOrigin().upUrl(), files() ->vfs_getOrigin().fileName());
}

void ListPanelFunc::home()
{
    openUrl(QDir::homePath());
}

void ListPanelFunc::root()
{
    openUrl(KUrl(ROOT_DIR));
}

void ListPanelFunc::syncOtherPanel()
{
    otherFunc()->openUrl(panel->virtualPath());
}

bool ListPanelFunc::atHome()
{
    return KUrl(QDir::homePath()).equals(panel->virtualPath(), KUrl::CompareWithoutTrailingSlash);
}

#include "panelfunc.moc"
