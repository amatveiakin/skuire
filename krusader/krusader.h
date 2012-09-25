/***************************************************************************
                                krusader.h
                           -------------------
    begin                : Thu May 4 2000
    copyright            : (C) 2000 by Shie Erlich & Rafi Yanai
    e-mail               : krusader@users.sourceforge.net
    web site             : http://krusader.sourceforge.net
 ---------------------------------------------------------------------------
 The main application ! what's more to say ?
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

#ifndef KRUSADER_H
#define KRUSADER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "abstracttwinpanelfm.h"
#include "abstractpanelmanager.h"
#include "abstractlistpanel.h"

// KDE includes
#include <kapplication.h>
#include <kparts/mainwindow.h>
#include <kstandardaction.h>
#include <kaction.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <QtCore/QStringList>
#include <QMoveEvent>
#include <QShowEvent>
#include <QResizeEvent>
#include <QHideEvent>
#include <kdebug.h>

#include <QTimer>

#include "VFS/kiojobwrapper.h"

#ifdef __KJSEMBED__
class KrJS;
#endif

class KrusaderStatus;
class KRPleaseWaitHandler;
class KSystemTrayIcon;
class UserMenu;
class UserAction;
class PopularUrls;
class QueueManager;
class TabActions;
class ActionsBase;


class Krusader : public KParts::MainWindow, public AbstractTwinPanelFM,
                 public CurrentViewCallback, public CurrentPanelCallback
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.krusader.Instance")

public:
    using KXMLGUIClient::action;

    Krusader();
    virtual ~Krusader();

    // AbstractMainWindow implementation
    virtual QWidget *widget() {
        return this;
    }
    virtual AbstractView *activeView();
    virtual KActionCollection *actions() {
        return actionCollection();
    }
    virtual QAction *action(QString name);
    // AbstractTwinPanelFM implementation
    virtual AbstractPanelManager *activeManager();
    virtual AbstractPanelManager *leftManager();
    virtual AbstractPanelManager *rightManager();

    // CurrentViewCallback implementation
    void onViewCreated(AbstractView *view);
    void onCurrentViewChanged(AbstractView *view);

    // CurrentPanelCallback implementation
    void onPanelCreated(AbstractListPanel *panel);
    void onCurrentPanelChanged(AbstractListPanel *panel);

    virtual PopularUrls *popularUrls() {
        return _popularUrls;
    }
    virtual KrActions *krActions() {
        return _krActions;
    }
    virtual TabActions *tabActions() {
        return _tabActions;
    }
    virtual void plugActionList(const char *name, QList <QAction*> &list) {
        KParts::MainWindow::plugActionList(name, list);
    }

    void configChanged(bool isGUIRestartNeeded);
    /**
     * This returns a defferent icon if krusader runs with root-privileges
     * @return a character string with the specitif icon-name
     */
    static const char* privIcon();
    static QStringList supportedTools(); // find supported tools


public slots:
    void moveToTop();
    void statusBarUpdate(QString mess);
    // in use by Krusader only
    void saveSettings();
    void savePosition();
    void updateUserActions();
    void updateGUI(bool enforce = false);
    void slotClose();
    void setDirectExit() {
        directExit = true;
    }

protected slots:
    void doOpenUrl();
    void configureShortcuts();
    void slotShortcutsConfigured();

protected:
    bool queryExit();
    bool queryClose();
    void setupActions();
    bool versionControl();  // handle version differences in krusaderrc
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void moveEvent(QMoveEvent *);
    void resizeEvent(QResizeEvent *);

public Q_SLOTS:
    Q_SCRIPTABLE bool isRunning();
    Q_SCRIPTABLE bool isLeftActive();
    Q_SCRIPTABLE bool openUrl(QString url);

public:
    static Krusader *App;       // a kApp style pointer
    static QString   AppName;   // the name of the application
    PopularUrls *_popularUrls; // holds a sorted list of the most popular urls visited
    QueueManager *queueManager;
    // return a path to a temp dir or file we can use.
    QString getTempDir();
    QString getTempFile();

    // the internal progress bar variales + functions
    KRPleaseWaitHandler* plzWait;
    void startWaiting(QString msg = "Please Wait", int count = 0 , bool cancel = false);
    void stopWait();
    bool wasWaitingCancelled() const;

    KrusaderStatus *status;
    static UserMenu *userMenu;

#ifdef __KJSEMBED__
    static KrJS *js;
#endif

signals:
    void changeMessage(QString);
    // emitted when we are about to quit
    void shutdown();

private:
    AbstractListPanel *activePanel();
    static void supportedTool(QStringList &tools, QString toolType,
                              QStringList names, QString confName);

    KrActions *_krActions;
    QList<ActionsBase*> _allActions;
    TabActions *_tabActions;
    KSystemTrayIcon *sysTray;
    QPoint       oldPos;
    QSize        oldSize;
    bool         isStarting;
    bool         isExiting;
    bool         directExit;
    KrJobStarter jobStarter;
    QTimer      _openUrlTimer;
    QString     _urlToOpen;
};


// main modules
#define krApp        Krusader::App

#ifdef __KJSEMBED__
#define krJS   Krusader::App->js
#endif

#endif
