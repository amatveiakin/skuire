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

#include "panelmanager.h"

#include "defaults.h"
#include "tabactions.h"
#include "krusaderview.h"
#include "krusaderapp.h"
#include "abstracttwinpanelfm.h"
#include "abstractlistpanelfactory.h"
#include "module.h"

#include <qstackedwidget.h>
#include <QtGui/QToolButton>
#include <QtGui/QImage>
#include <QGridLayout>
#include <QTimer>
#include <QDir>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kiconloader.h>

//HACK
#include "Panel/listpanel.h"

#define HIDE_ON_SINGLE_TAB  false


PanelManager::PanelManager(QWidget *parent, AbstractTwinPanelFM* mainWindow, bool left,
                CurrentPanelCallback *currentPanelCb, CurrentViewCallback *currentViewCb) :
        QWidget(parent),
        _otherManager(0),
        _mainWindow(mainWindow),
        _currentPanel(0),
        _currentPanelCb(currentPanelCb),
        _currentViewCb(currentViewCb),
        _actions(mainWindow->tabActions()),
        _layout(0),
        _left(left)
{
}

void PanelManager::init()
{
    Module *listPanelModule = KrusaderApp::self()->module("ListPanel");
    panelFactory = qobject_cast<AbstractListPanelFactory*>(listPanelModule);
    Q_ASSERT(panelFactory);

    _layout = new QGridLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    _layout->setSpacing(0);
    _stack = new QStackedWidget(this);

    // new tab button
    _newTab = new QToolButton(this);
    _newTab->setAutoRaise(true);
    _newTab->setText(i18n("Open a new tab in home"));
    _newTab->setToolTip(i18n("Open a new tab in home"));
    _newTab->setIcon(SmallIcon("tab-new"));
    _newTab->adjustSize();
    connect(_newTab, SIGNAL(clicked()), this, SLOT(slotNewTab()));

    // tab-bar
    _tabbar = new PanelTabBar(this, _actions);
    connect(_tabbar, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentTabChanged(int)));
    connect(_tabbar, SIGNAL(tabCloseRequested(int)), this, SLOT(slotCloseTab(int)));
    connect(_tabbar, SIGNAL(closeCurrentTab()), this, SLOT(slotCloseTab()));
    connect(_tabbar, SIGNAL(newTab(const KUrl&)), this, SLOT(slotNewTab(const KUrl&)));
    connect(_tabbar, SIGNAL(draggingTab(QMouseEvent*)), this, SLOT(slotDraggingTab(QMouseEvent*)));
    connect(_tabbar, SIGNAL(draggingTabFinished(QMouseEvent*)), this, SLOT(slotDraggingTabFinished(QMouseEvent*)));

    QHBoxLayout *tabbarLayout = new QHBoxLayout;
    tabbarLayout->setSpacing(0);
    tabbarLayout->setContentsMargins(0, 0, 0, 0);

    tabbarLayout->addWidget(_tabbar);
    tabbarLayout->addWidget(_newTab);

    _layout->addWidget(_stack, 0, 0);
    _layout->addLayout(tabbarLayout, 1, 0);

    updateTabbarPos();

    setLayout(_layout);

    createPanel(true);

    tabsCountChanged();
}

bool PanelManager::isActive()
{
    return _mainWindow->activeManager() == this;
}

void PanelManager::onActiveStateChanged()
{
    _currentPanel->onActiveStateChanged();
}

void PanelManager::tabsCountChanged()
{
    bool showTabbar = true;
    if (_tabbar->count() <= 1 && HIDE_ON_SINGLE_TAB)
        showTabbar = false;

    KConfigGroup cfg(krConfig, "Look&Feel");
    bool showButtons = showTabbar && cfg.readEntry("Show Tab Buttons", true);

    _newTab->setVisible(showButtons);
    _tabbar->setVisible(showTabbar);

    // disable close button if only 1 tab is left
    _tabbar->setTabsClosable(showButtons && _tabbar->count() > 1);

    _actions->refreshActions();
}

void PanelManager::activate()
{
    Q_ASSERT(sender() == (currentPanel()));
    emit setActiveManager(this);
    Q_ASSERT(isActive());
    _actions->refreshActions();
}

void PanelManager::slotCurrentTabChanged(int index)
{
    AbstractListPanel *p = _tabbar->getPanel(index);

    if (!p || p == _currentPanel)
        return;

    AbstractListPanel *prev = _currentPanel;
    _currentPanel = p;

    _stack->setCurrentWidget(_currentPanel);

    if(prev)
        prev->onActiveStateChanged();
    _currentPanel->onActiveStateChanged();

    _currentViewCb->onCurrentViewChanged(_currentPanel->view());
    _currentPanelCb->onCurrentPanelChanged(_currentPanel);

    emit pathChanged(p);

    if(otherManager() && otherManager()->currentPanel())
        otherManager()->currentPanel()->onOtherPanelChanged();
}

void PanelManager::connectPanel(AbstractListPanel *p)
{
    connect(p, SIGNAL(activate()), this, SLOT(activate()));
    connect(p, SIGNAL(pathChanged(AbstractListPanel*)), this, SIGNAL(pathChanged(AbstractListPanel*)));
}

void PanelManager::disconnectPanel(AbstractListPanel *p)
{
    disconnect(p, SIGNAL(activate()), this, 0);
    disconnect(p, SIGNAL(pathChanged(AbstractListPanel*)), this, 0);
}

AbstractListPanel* PanelManager::createPanel(bool setCurrent, KConfigGroup cfg, AbstractListPanel *nextTo)
{
    // create the panel and add it into the widgetstack
    AbstractListPanel * p = panelFactory->createPanel(_stack, this, _currentViewCb, cfg);
    _currentPanelCb->onPanelCreated(p);

    addPanel(p, setCurrent, nextTo);

    return p;
}

void PanelManager::addPanel(AbstractListPanel *panel, bool setCurrent, AbstractListPanel *nextTo)
{
    _stack->addWidget(panel);
    connectPanel(panel);

    // now, create the corrosponding tab
    int index = _tabbar->addPanel(panel, setCurrent, nextTo);
    tabsCountChanged();

    if (setCurrent)
        slotCurrentTabChanged(index);
}

void PanelManager::saveSettings(KConfigGroup config, bool localOnly, bool saveHistory)
{
    config.writeEntry("ActiveTab", activeTab());

    KConfigGroup grpTabs(&config, "Tabs");
    foreach(QString grpTab, grpTabs.groupList())
        grpTabs.deleteGroup(grpTab);

    for(int i = 0; i < _tabbar->count(); i++) {
        AbstractListPanel *panel = _tabbar->getPanel(i);
        KConfigGroup grpTab(&grpTabs, "Tab" + QString::number(i));
        panel->saveSettings(grpTab, localOnly, saveHistory);
    }
}

void PanelManager::loadSettings(KConfigGroup config)
{
    KConfigGroup grpTabs(&config, "Tabs");
    int numTabsOld = _tabbar->count();
    int numTabsNew = grpTabs.groupList().count();

    for(int i = 0;  i < numTabsNew; i++) {
        KConfigGroup grpTab(&grpTabs, "Tab" + QString::number(i));
        AbstractListPanel *panel;
        if(i < numTabsOld)
            panel = _tabbar->getPanel(i);
        else
            panel = createPanel(false, grpTab);
        panel->restoreSettings(grpTab);
    }

    for(int i = numTabsOld - 1; i >= numTabsNew && i > 0; i--)
        slotCloseTab(i);

    setActiveTab(config.readEntry("ActiveTab", 0));

    // this is needed so that all tab labels get updated
    layoutTabs();
}

void PanelManager::layoutTabs()
{
    // delayed url refreshes may be pending -
    // delay the layout too so it happens after them
    QTimer::singleShot(0, _tabbar, SLOT(layoutTabs()));
}

void PanelManager::moveTabToOtherSide()
{
    if(tabCount() < 2)
        return;

    AbstractListPanel *p;
    _tabbar->removeCurrentPanel(p); // this should result in a panel switch
    // now the new panel should be active
    _stack->removeWidget(p);
    disconnectPanel(p);

    p->reparent(_otherManager->_stack, _otherManager);
    _otherManager->addPanel(p, true);
}

void PanelManager::slotNewTab(const KUrl& url, bool setCurrent, AbstractListPanel *nextTo)
{
    AbstractListPanel *p = createPanel(setCurrent, KConfigGroup(), nextTo);
    p->start(url);
}

void PanelManager::slotNewTab()
{
    slotNewTab(QDir::home().absolutePath());
}

void PanelManager::slotCloseTab()
{
    slotCloseTab(_tabbar->currentIndex());
}

void PanelManager::slotCloseTab(int index)
{
    if (_tabbar->count() <= 1)    /* if this is the last tab don't close it */
        return ;

    AbstractListPanel *oldp;

    _tabbar->removePanel(index, oldp); //this automatically changes the current panel

    _stack->removeWidget(oldp);
    deletePanel(oldp);
    tabsCountChanged();
}

void PanelManager::updateTabbarPos()
{
    KConfigGroup group(krConfig, "Look&Feel");
    if(group.readEntry("Tab Bar Position", "bottom") == "top") {
        _layout->addWidget(_stack, 2, 0);
        _tabbar->setShape(QTabBar::RoundedNorth);
    } else {
        _layout->addWidget(_stack, 0, 0);
        _tabbar->setShape(QTabBar::RoundedSouth);
    }
}

int PanelManager::activeTab()
{
    return _tabbar->currentIndex();
}

void PanelManager::setActiveTab(int index)
{
    _tabbar->setCurrentIndex(index);
}

void PanelManager::slotNextTab()
{
    int currTab = _tabbar->currentIndex();
    int nextInd = (currTab == _tabbar->count() - 1 ? 0 : currTab + 1);
    _tabbar->setCurrentIndex(nextInd);
}


void PanelManager::slotPreviousTab()
{
    int currTab = _tabbar->currentIndex();
    int nextInd = (currTab == 0 ? _tabbar->count() - 1 : currTab - 1);
    _tabbar->setCurrentIndex(nextInd);
}

void PanelManager::deletePanel(AbstractListPanel * p)
{
    disconnect(p);

    //HACK

    if (((ListPanel*)p)->canDelete())
        delete p;
    else
        ((ListPanel*)p)->requestDelete();
}

void PanelManager::slotCloseInactiveTabs()
{
    int i = 0;
    while (i < _tabbar->count()) {
        if (i == activeTab())
            i++;
        else
            slotCloseTab(i);
    }
}

void PanelManager::slotCloseDuplicatedTabs()
{
    int i = 0;
    while (i < _tabbar->count() - 1) {
        AbstractListPanel *panel1 = _tabbar->getPanel(i);
        if (panel1 != 0) {
            for (int j = i + 1; j < _tabbar->count(); j++) {
                AbstractListPanel *panel2 = _tabbar->getPanel(j);
                if (panel2 != 0 && panel1->url().equals(panel2->url(), KUrl::CompareWithoutTrailingSlash)) {
                    if (j == activeTab()) {
                        slotCloseTab(i);
                        i--;
                        break;
                    } else {
                        slotCloseTab(j);
                        j--;
                    }
                }
            }
        }
        i++;
    }
}

int PanelManager::findTab(KUrl url)
{
    url.cleanPath();
    for(int i = 0; i < _tabbar->count(); i++) {
        if(_tabbar->getPanel(i)) {
            KUrl panelUrl = _tabbar->getPanel(i)->virtualPath();
            panelUrl.cleanPath();
            if(panelUrl.equals(url, KUrl::CompareWithoutTrailingSlash))
                return i;
        }
    }
    return -1;
}

void PanelManager::slotLockTab()
{
    currentPanel()->setLocked(!currentPanel()->isLocked());
    _actions->refreshActions();
}

void PanelManager::newTabs(const QStringList& urls) {
    for(int i = 0; i < urls.count(); i++)
        slotNewTab(KUrl(urls[i]));
}

AbstractListPanel *PanelManager::currentPanel()
{
    return _currentPanel;
}

#include "panelmanager.moc"
