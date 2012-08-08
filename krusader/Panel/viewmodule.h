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

#ifndef __VIEWMODULE_H__
#define __VIEWMODULE_H__

#include "module.h"
#include "viewfactory.h"


class ViewModule : public Module, public ViewFactory
{
    Q_OBJECT
    Q_INTERFACES(ViewFactory)

public:
    // Module implementation
    virtual QString name() {
        return "view";
    }
    virtual void init();
    virtual ActionsBase *createActions(QObject *parent, KrMainWindow *mainWindow);

    // ViewFactory implementation
    virtual View *createView(int id, QWidget *parent, KConfig *cfg, QWidget *mainWindow,
                             KrQuickSearch *quickSearch, QuickFilter *quickFilter);
    virtual QList<ViewType*> registeredViews();
    virtual int defaultViewId();

private slots:
    void configChanged();
};

#endif // __VIEWMODULE_H__
