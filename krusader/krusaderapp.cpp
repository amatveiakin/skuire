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

#include "krusaderapp.h"

#include "Konfigurator/konfigurator.h"
#include "module.h"
#include "krusader.h"

#include <kstartupinfo.h>

Module *ViewModule_create();
Module *ListPanelModule_create();


KrusaderApp::KrusaderApp(): KApplication()
{
    _modules << ViewModule_create();
    _modules << ListPanelModule_create();

    KStartupInfo *startupInfo = new KStartupInfo(0, this);
    connect(startupInfo, SIGNAL(gotNewStartup (const KStartupInfoId&, const KStartupInfoData&)),
            SLOT(slotGotNewStartup(const KStartupInfoId&, const KStartupInfoData&)));
    connect(startupInfo, SIGNAL(gotRemoveStartup(const KStartupInfoId&, const KStartupInfoData&)),
            SLOT(slotGotRemoveStartup(const KStartupInfoId&, const KStartupInfoData&)));
}

KrusaderApp::~KrusaderApp()
{
    foreach(Module *m, _modules)
        delete m;
    _modules.clear();
}

KrusaderApp* KrusaderApp::self()
{
    return qobject_cast<KrusaderApp*>(qApp);
}

void KrusaderApp::initModules()
{
    foreach(Module *m, _modules)
        m->init();
}

Module *KrusaderApp::findModule(QString name)
{
    foreach(Module *m, _modules) {
        if (m->name() == name)
            return m;
    }
    return 0;
}

Module *KrusaderApp::module(QString name)
{
    Module *m = findModule(name);
    if (!m) {
        qFatal("Can't find module: %s", name.toLocal8Bit().data());
        abort();
    }
    else
        return m;
}

void KrusaderApp::focusInEvent(QFocusEvent* /*event*/)
{
    emit windowActive();
}

void KrusaderApp::focusOutEvent(QFocusEvent* /*event*/)
{
    emit windowInactive();
}

void KrusaderApp::runKonfigurator(bool firstTime)
{
    Konfigurator *konfigurator = new Konfigurator(firstTime);
    connect(konfigurator, SIGNAL(configChanged(bool)), SLOT(slotConfigChanged(bool)));

    konfigurator->exec();

    delete konfigurator;
}

void KrusaderApp::slotConfigChanged(bool isGUIRestartNeeded)
{
    krApp->configChanged(isGUIRestartNeeded);

    emit configChanged();
}

void KrusaderApp::slotGotNewStartup(const KStartupInfoId &id, const KStartupInfoData &data)
{
    Q_UNUSED(id)
    Q_UNUSED(data)
    setOverrideCursor(Qt::BusyCursor);
}

void KrusaderApp::slotGotRemoveStartup(const KStartupInfoId &id, const KStartupInfoData &data)
{
    Q_UNUSED(id)
    Q_UNUSED(data)
    restoreOverrideCursor();
}
