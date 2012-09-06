/*****************************************************************************
 * Copyright (C) 2003 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2003 Rafi Yanai <yanai@users.sourceforge.net>               *
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

#include "krpopupmenu.h"

#include <QPixmap>
#include <QDesktopWidget>
#include <QSignalMapper>

#include <klocale.h>
#include <kprocess.h>
#include <krun.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <ktoolinvocation.h>
#include <kactioncollection.h>

#include "listpanel.h"
#include "panelfunc.h"
#include "listpanelactions.h"
#include "../krservices.h"
#include "../defaults.h"
#include "../MountMan/kmountman.h"
#include "../krusader.h"
#include "../krslots.h"
#include "../krusaderview.h"
#include "../panelmanager.h"
#include "../krtrashhandler.h"
#include "abstractview.h"

#ifdef __LIBKONQ__
#include <konq_popupmenu.h>
#include <konq_menuactions.h>
#include <konq_popupmenuinformation.h>
#endif


void KrPopupMenu::execMenu(KrPopupMenu *menu, QPoint pos)
{
    menu->exec(pos);
}

void KrPopupMenu::run(ListPanel *panel, bool onlyOpenWith)
{
    KrPopupMenu menu(panel, panel, onlyOpenWith);
    if(menu.item.isNull())
        return;

    QRect itemRect = panel->view()->itemRectGlobal(menu.item.url());
    QPoint pos = itemRect.bottomLeft();

    QSize menuSize = menu.sizeHint();
    QSize screenSize = QApplication::desktop()->size();

    if ((screenSize.height() - pos.y()) < menuSize.height()) {
        // the menu doesn't fit below the item
        pos = itemRect.topLeft();
        pos.setY(pos.y() - menuSize.height());
    }

    execMenu(&menu, pos);
}

void KrPopupMenu::run(QPoint pos, ListPanel *panel, bool onlyOpenWith)
{
    KrPopupMenu menu(panel, panel, onlyOpenWith);

    QSize menuSize = menu.sizeHint();
    QSize screenSize = QApplication::desktop()->size();

    if ((screenSize.height() - pos.y()) < menuSize.height()) {
        // the menu doesn't fit below the item
        pos.setY(pos.y() - menuSize.height());
    }

    execMenu(&menu, pos);
}

KrPopupMenu::KrPopupMenu(ListPanel *panel, QWidget *parent, bool onlyOpenWith) :
        KMenu(parent),
        panel(panel),
        mainWindow(panel->manager()->mainWindow()),
        empty(false),
        multipleSelections(false),
        mapper(new QSignalMapper(this))
{
#ifdef __LIBKONQ__
    konqMenu = 0;
    konqMenuActions = 0;
#endif

    connect(mapper, SIGNAL(mapped(int)), SLOT(performAction(int)));

    bool isLocalDir = panel->url().isLocalFile();

    items = panel->view()->getSelectedItems(true);
    if (items.isEmpty()) {
        addCreateNewMenu();
        addSeparator();
        addEmptyMenuEntries();
        return;
    }

    if (items.count() > 1)
        multipleSelections = true;

    bool inTrash = false, trashOnly = false;

    for(int i = 0; i < items.count(); i++) {
        if (items[i].url().protocol() == "trash") {
            inTrash = true;
            if(0 == i)
                trashOnly = true;
            break;
        }
    }

    item = items.first();

    if(onlyOpenWith) {
        addOpenWithEntries(this);
        return;
    }

    // ------------ the OPEN option - open preferred service
    QAction * openAct = addAction(OPEN_ID, i18n("Open/Run"));
    if (!multipleSelections) {   // meaningful only if one file is selected
        openAct->setIcon(KIcon(item.iconName()));
        bool isExecutable = KRun::isExecutableFile(item.url(), item.mimetype());
        openAct->setText(isExecutable ? i18n("Run") : i18n("Open"));
        // open in a new tab (if folder)
        if (item.isDir()) {
            QAction * openTab = addAction(OPEN_TAB_ID, i18n("Open in New Tab"));
            openTab->setIcon(KIcon("tab-new"));
            openTab->setText(i18n("Open in New Tab"));
        }
        addSeparator();
    }

    // ------------- Preview - normal vfs only ?
    if (isLocalDir) {
        // create the preview popup
        preview.setItems(items);
        QAction *pAct = addMenu(&preview);
        pAct->setText(i18n("Preview"));
    }

    // -------------- Open with
    KMenu *openWith = new KMenu(this);
    addOpenWithEntries(openWith);
    QAction *owAct = addMenu(openWith);
    owAct->setText(i18n("Open With"));
    addSeparator();

    // --------------- user actions
    QAction *uAct = new UserActionPopupMenu(item.url());
    addAction(uAct);
    uAct->setText(i18n("User Actions"));

#ifdef __LIBKONQ__
    // -------------- konqueror menu
    // This section adds all Konqueror/Dolphin menu items.
    // It's now updated to KDE4 and working, I've just commented it out.
    // Section below this one adds only servicemenus.

    // Append only servicemenus

    //TODO: deprecated since KDE4.3: remove these three lines
    KonqPopupMenuInformation info;
    info.setItems(items);
    info.setParentWidget(this);

    konqMenuActions = new KonqMenuActions();
    //TODO: deprecated since KDE4.3: remove this line, use two commented lines
    konqMenuActions->setPopupMenuInfo(info);
    //konqMenuActions->setParentWidget( this );
    //konqMenuActions->setItemListProperties( items );
    konqMenuActions->addActionsTo(this);

    addSeparator();
#endif

    // ------------- 'create new' submenu
    addCreateNewMenu();
    addSeparator();

    // ---------- COPY
    addAction(COPY_ID, i18n("Copy..."));
    // ------- MOVE
    addAction(MOVE_ID, i18n("Move..."));
    // ------- RENAME - only one file
    if (!multipleSelections && !inTrash)
        addAction(RENAME_ID, i18n("Rename"));

    // -------- MOVE TO TRASH
    KConfigGroup saver(krConfig, "General");
    bool trash = saver.readEntry("Move To Trash", _MoveToTrash);
    if (trash && !inTrash)
        addAction(TRASH_ID, i18n("Move to Trash"));
    // -------- DELETE
    addAction(DELETE_ID, i18n("Delete"));
    // -------- SHRED - only one file
    /*      if (isLocalDir &&
                !item.isDir() && !multipleSelections )
                addAction(SHRED_ID, i18n( "Shred" )); */

    // ---------- link handling
    // create new shortcut or redirect links - only on local directories:
    if (isLocalDir) {
        addSeparator();
        addAction(NEW_SYMLINK_ID, i18n("New Symlink..."), &linkPopup);
        addAction(NEW_LINK_ID, i18n("New Hardlink..."), &linkPopup);
        if (item.isLink())
            addAction(REDIRECT_LINK_ID, i18n("Redirect Link..."), &linkPopup);
        QAction *linkAct = addMenu(&linkPopup);
        linkAct->setText(i18n("Link Handling"));
    }
    addSeparator();

    // ---------- calculate space
    if (isLocalDir && (item.isDir() || multipleSelections))
        addMainWindowAction("calculate");

    // ---------- mount/umount/eject
    if (isLocalDir && item.isDir() && !multipleSelections) {
        if (krMtMan.getStatus(item.url().path(KUrl::RemoveTrailingSlash)) == KMountMan::MOUNTED)
            addAction(UNMOUNT_ID, i18n("Unmount"));
        else if (krMtMan.getStatus(item.url().path(KUrl::RemoveTrailingSlash)) == KMountMan::NOT_MOUNTED)
            addAction(MOUNT_ID, i18n("Mount"));
        if (krMtMan.ejectable(item.url().path(KUrl::RemoveTrailingSlash)))
            addAction(EJECT_ID, i18n("Eject"));
    }

    // --------- send by mail
    if (Krusader::supportedTools().contains("MAIL") && !item.isDir())
        addAction(SEND_BY_EMAIL_ID, i18n("Send by Email"));

    // --------- empty trash
    if (trashOnly) {
        addAction(RESTORE_TRASHED_FILE_ID, i18n("Restore"));
        addAction(EMPTY_TRASH_ID, i18n("Empty Trash"));
    }

    // --------- synchronize
    if (panel->view()->numSelected())
        addAction(SYNC_SELECTED_ID, i18n("Synchronize Selected Files..."));

    // --------- copy/paste
    addSeparator();
    addAction(COPY_CLIP_ID, i18n("Copy to Clipboard"));
    addAction(MOVE_CLIP_ID, i18n("Cut to Clipboard"));
    addAction(PASTE_CLIP_ID, i18n("Paste from Clipboard"));
    addSeparator();

    // --------- properties
    addMainWindowAction("properties");
}

KrPopupMenu::~KrPopupMenu()
{
    items.clear();
#ifdef __LIBKONQ__
    if (konqMenu) delete konqMenu;
    if (konqMenuActions) delete konqMenuActions;
#endif
}

QAction *KrPopupMenu::addAction(int id, QString text, KMenu *menu)
{
    QAction *action = menu->addAction(text);
    connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(action, id);
    return action;
}

void KrPopupMenu::addMainWindowAction(QString name)
{
    if (QAction *act = mainWindow->action(name))
        addAction(act);
}

void KrPopupMenu::addEmptyMenuEntries()
{
    addAction(PASTE_CLIP_ID, i18n("Paste from Clipboard"));
}

void KrPopupMenu::addCreateNewMenu()
{
    addAction(MKDIR_ID, i18n("Folder..."), &createNewPopup)->setIcon(KIcon("folder"));
    addAction(NEW_TEXT_FILE_ID, i18n("Text File..."), &createNewPopup)->setIcon(KIcon("text-plain"));

    QAction *newAct = addMenu(&createNewPopup);
    newAct->setText(i18n("Create New"));

}

void KrPopupMenu::addOpenWithEntries(KMenu *menu)
{
    // -------------- Open with: try to find-out which apps can open the file
    // this too, is meaningful only if one file is selected or if all the files
    // have the same mimetype !
    QString mime = item.mimetype();
    // check if all the list have the same mimetype
    for (int i = 1; i < items.count(); ++i) {
        if (items[i].mimetype() != mime) {
            mime.clear();
            break;
        }
    }
    if (!mime.isEmpty()) {
        offers = KMimeTypeTrader::self()->query(mime);
        for (int i = 0; i < offers.count(); ++i) {
            KSharedPtr<KService> service = offers[ i ];
            if (service->isValid() && service->isApplication())
                addAction(SERVICE_LIST_ID + i, service->name(), menu)->setIcon(KIcon(service->icon()));
        }
        menu->addSeparator();
        if (item.isDir())
            addAction(OPEN_TERM_ID, i18n("Terminal"), menu)->setIcon(KIcon("konsole"));
        addAction(CHOOSE_ID, i18n("Other..."), menu);
    }
}

void KrPopupMenu::performAction(int id)
{
    KUrl::List lst;

    switch (id) {
    case - 1 : // the user clicked outside of the menu
        return ;
    case OPEN_TAB_ID :
        // assuming only 1 file is selected (otherwise we won't get here)
        panel->manager()->newTab(item.url(), panel);
        break;
    case OPEN_ID :
        foreach(KFileItem item, items)
            panel->func->execute(item);
        break;
    case COPY_ID :
        panel->func->copyFiles();
        break;
    case MOVE_ID :
        panel->func->moveFiles();
        break;
    case RENAME_ID :
        panel->func->rename();
        break;
    case TRASH_ID :
        panel->func->deleteFiles(false);
        break;
    case DELETE_ID :
        panel->func->deleteFiles(true);
        break;
    case EJECT_ID :
        krMtMan.eject(item.url().path(KUrl::RemoveTrailingSlash));
        break;
        /*         case SHRED_ID :
                    if ( KMessageBox::warningContinueCancel( krApp,
                         i18n("<qt>Do you really want to shred <b>%1</b>? Once shred, the file is gone forever.</qt>", item->name()),
                         QString(), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "Shred" ) == KMessageBox::Continue )
                       KShred::shred(item.url().pathOrUrl(KUrl::RemoveTrailingSlash ));
                  break;*/
    case OPEN_KONQ_ID :
        KToolInvocation::startServiceByDesktopName("konqueror", item.url().pathOrUrl());
        break;
    case CHOOSE_ID : // open-with dialog
        lst.append(item.url());
        panel->func->displayOpenWithDialog(lst);
        break;
    case MOUNT_ID :
        krMtMan.mount(item.url().path(KUrl::RemoveTrailingSlash));
        break;
    case NEW_LINK_ID :
        panel->func->krlink(false);
        break;
    case NEW_SYMLINK_ID :
        panel->func->krlink(true);
        break;
    case REDIRECT_LINK_ID :
        panel->func->redirectLink();
        break;
    case EMPTY_TRASH_ID :
        KrTrashHandler::emptyTrash();
        break;
    case RESTORE_TRASHED_FILE_ID :
        KrTrashHandler::restoreTrashedFiles(items.urlList());
        break;
    case UNMOUNT_ID :
        krMtMan.unmount(item.url().path(KUrl::RemoveTrailingSlash));
        break;
    case COPY_CLIP_ID :
        panel->func->copyToClipboard();
        break;
    case MOVE_CLIP_ID :
        panel->func->copyToClipboard(true);
        break;
    case PASTE_CLIP_ID :
        panel->func->pasteFromClipboard();
        break;
    case SEND_BY_EMAIL_ID :
        SLOTS->sendFileByEmail(items.urlList());
        break;
    case MKDIR_ID :
        panel->func->mkdir();
        break;
    case NEW_TEXT_FILE_ID:
        panel->func->editNew();
        break;
    case SYNC_SELECTED_ID : {
        QStringList selectedNames; //FIXME remove this; update syncronizer to take urls as argument
        foreach(KFileItem item, items)
            selectedNames << item.name();
        if (panel->otherPanel()->view()->numSelected()) {
            KFileItemList otherItems = panel->otherPanel()->view()->getSelectedItems(true);
            foreach(KFileItem otherItem, otherItems) {
                if (!selectedNames.contains(otherItem.name()))
                    selectedNames.append(otherItem.name());
            }
        }
        SLOTS->slotSynchronizeDirs(selectedNames);
    }
    break;
    case OPEN_TERM_ID : {
        QStringList args;
        if (!item.isDir())
            args << item.name();
        SLOTS->runTerminal(item.url().path(), args);
    }
    break;
    }

    // check if something from the open-with-offered-services was selected
    if (id >= SERVICE_LIST_ID) {
        panel->func->runService(*(offers[ id - SERVICE_LIST_ID ]),
                                items.urlList());
    }
}

#include "krpopupmenu.moc"
