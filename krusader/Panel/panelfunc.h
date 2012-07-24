/***************************************************************************
                                panelfunc.h
                             -------------------
    begin                : Thu May 4 2000
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

                                                     H e a d e r    F i l e

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef PANELFUNC_H
#define PANELFUNC_H

#include "../VFS/vfs.h"

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QClipboard>
#include <kservice.h>


class ListPanel;
class DirHistoryQueue;
class VfileDirLister;
namespace KIO { class DirectorySizeJob; }

class ListPanelFunc : public QObject
{
    friend class ListPanel;
    Q_OBJECT
public slots:
    void execute(KFileItem item);
    void goInside(KFileItem item);
    void urlEntered(const QString &url);
    void urlEntered(const KUrl &url);
    void openUrl(const KUrl& path, KUrl urlToMakeCurrent = KUrl(),
                 bool manuallyEntered = false);
//     void popErronousUrl();
    void immediateOpenUrl(const KUrl &url, bool disableLock = false);
    void rename(KFileItem item, QString newname);
    void calcSpace(KFileItem item);
    void trashJobStarted(KIO::Job *job);

    // actions
    void historyBackward();
    void historyForward();
    void dirUp();
    void refresh();
    void home();
    void root();
    void cdToOtherPanel();
    void FTPDisconnect();
    void newFTPconnection();
    void terminal();
    void view();
    void viewDlg();
    void edit();
    void editNew(); // create a new textfile and edit it
    void openWith();
    void copyFiles(bool enqueue = false);
    void moveFiles(bool enqueue = false);
    void copyFilesByQueue() {
        copyFiles(true);
    }
    void moveFilesByQueue() {
        moveFiles(true);
    }
    void mkdir();
    void deleteFiles(bool reallyDelete = false);
    void rename();
    void krlink(bool sym = true);
    void createChecksum();
    void matchChecksum();
    void pack();
    void unpack();
    void testArchive();
    void calcSpace(); // calculate the occupied space and show it in a dialog
    void calcAllDirsSpace();
    void properties();
    void cut() {
        copyToClipboard(true);
    }
    void copyToClipboard(bool move = false);
    void pasteFromClipboard();
    void syncOtherPanel();

public:
    ListPanelFunc(ListPanel *parent);
    ~ListPanelFunc();

    inline vfile* getVFile(const QString& name) {
        return files()->vfs_search(name);
    }

    void redirectLink();
    void runService(const KService &service, KUrl::List urls);
    void displayOpenWithDialog(KUrl::List urls);

    ListPanelFunc* otherFunc();
    bool atHome();
    bool canGoBack();
    bool canGoForward();

protected slots:
    void doRefresh();
    void slotFileCreated(KJob *job); // a file has been created by editNewFile()
    void slotKdsResult(KJob* job);
    void historyGotoPos(int pos);
    void clipboardChanged(QClipboard::Mode mode);

protected:
    KUrl cleanPath(const KUrl &url);
    bool isSyncing(const KUrl &url);
    void openUrlInternal(const KUrl& url, KUrl urlToMakeCurrent,
                         bool immediately, bool disableLock, bool manuallyEntered);
    void runCommand(QString cmd);
    void cancelCalcSpace();

    vfs* files();  // return a pointer to the vfs

    ListPanel*           panel;     // our ListPanel
    DirHistoryQueue*     history;
    vfs*                 vfsP;      // pointer to vfs.
    VfileDirLister      *dirLister;
    QTimer               delayTimer;
    KUrl                 syncURL;
    KUrl                 fileToCreate; // file that's to be created by editNewFile()
    bool                 urlManuallyEntered;
    QHash<KIO::DirectorySizeJob*, QString> kdsFileNameMap;

    static QPointer<ListPanelFunc> copyToClipboardOrigin;

private:
    KUrl getVirtualBaseURL();
};

#endif
