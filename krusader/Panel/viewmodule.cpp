/*****************************************************************************
 * Copyright (C) 2012 Jan Lepper <jan_lepper@gmx.de>                         *
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

#include "viewmodule.h"

#include "viewexpplaceholders.h"
#include "krview.h"
#include "krviewfactory.h"
#include "viewactions.h"
#include "krselectionmode.h"
#include "viewconfigtab.h"
#include "selectionmodeconfigtab.h"

#include "../krusaderapp.h"


Module *ViewModule_create()
{
    return new ViewModule();
}


void ViewModule::init()
{
    ViewExpPlaceholders::init();
    KrView::initGlobal();
    connect(KrusaderApp::self(), SIGNAL(configChanged()), this, SLOT(configChanged()));
}

ActionsBase *ViewModule::createActions(QObject *parent, AbstractMainWindow *mainWindow)
{
    return new ViewActions(parent, mainWindow);
}

AbstractView *ViewModule::createView(int id, QWidget *parent, KConfig *cfg, QWidget *mainWindow,
                             KrQuickSearch *quickSearch, QuickFilter *quickFilter)
{
    AbstractView *view = KrViewFactory::createView(id, parent, cfg);
    view->init(mainWindow, quickSearch, quickFilter);
    return view;
}

QList<ViewType*> ViewModule::registeredViews()
{
    QList<ViewType*> list;
    foreach(KrViewInstance *inst, KrViewFactory::registeredViews())
        list << inst;
    return list;
}

int ViewModule::defaultViewId()
{
    return KrViewFactory::defaultViewId();
}

QWidget *ViewModule::createViewCfgTab(QWidget* parent, KonfiguratorPage *page, int tabId)
{
    return new ViewConfigTab(parent, page, tabId);
}

QWidget *ViewModule::createSelectionModeCfgTab(QWidget* parent, KonfiguratorPage *page, int tabId)
{
    return new SelectionModeConfigTab(parent, page, tabId);
}

void ViewModule::configChanged()
{
    KrSelectionMode::resetSelectionHandler();
    KrView::refreshAllViews();
}
