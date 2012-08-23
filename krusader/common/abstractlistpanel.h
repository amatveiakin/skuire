/***************************************************************************
                               krpanel.h
                            -------------------
   copyright            : (C) 2010 by Jan Lepper
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


#ifndef __ABSTRACTLISTPANEL_H__
#define __ABSTRACTLISTPANEL_H__

#include "abstractpanelmanager.h"

#include <kurl.h>
#include <kconfiggroup.h>
#include <QWidget>


class AbstractView;
class AbstractDirLister;


class AbstractListPanel : public QWidget
{
    Q_OBJECT

signals:
    void activate(); // request to become the active panel
    void pathChanged(AbstractListPanel *panel);

public:
    AbstractListPanel(QWidget *parent, AbstractPanelManager *manager) :
        QWidget(parent),
        view(0), _manager(manager) {}
    virtual ~AbstractListPanel() {}

    //TODO: remove this
    KUrl virtualPath() const {
        return url();
    }

    virtual KUrl url() const = 0;

    virtual AbstractDirLister *dirLister() = 0;

    virtual bool isLocked() = 0;
    virtual void setLocked(bool lock) = 0;
    virtual void saveSettings(KConfigGroup cfg, bool localOnly, bool saveHistory = false) = 0;
    virtual void restoreSettings(KConfigGroup cfg) = 0;
    virtual void start(KUrl url = KUrl(), bool immediate = false) = 0;
    virtual void openUrl(KUrl url, KUrl itemToMakeCurrent = KUrl()) = 0;
    virtual void reparent(QWidget *parent, AbstractPanelManager *manager) = 0;
    virtual void getFocusCandidates(QVector<QWidget*> &widgets) = 0;
    virtual void onActiveStateChanged() = 0; // to be called by panel manager
    virtual void onOtherPanelChanged() = 0; // to be called by panel manager

    AbstractPanelManager *manager() {
        return _manager;
    }
    AbstractListPanel *otherPanel() {
        return _manager->otherManager()->currentPanel();
    }
    bool isLeft() const {
        return _manager->isLeft();
    }

    AbstractView *view;

public slots:
    virtual void refresh() = 0;

protected:
    AbstractPanelManager *_manager;
};

class CurrentViewCallback
{
public:
    virtual ~CurrentViewCallback() {}
    virtual void onViewCreated(AbstractView *view) = 0;
    virtual void onCurrentViewChanged(AbstractView *view) = 0;
};

#endif // __ABSTRACTLISTPANEL_H__
