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

#ifndef __VIEWFACTORY_H__
#define __VIEWFACTORY_H__


class View;
class QWidget;
class KConfig;
class KrQuickSearch;
class QuickFilter;

class ViewFactory
{
public:
    virtual ~ViewFactory() {}
    virtual View *createView(int id, QWidget *parent, KConfig *cfg, QWidget *mainWindow,
                             KrQuickSearch *quickSearch, QuickFilter *quickFilter) = 0;
};

Q_DECLARE_INTERFACE (ViewFactory, "org.krusader.ViewFactory/0.1")

#endif // __VIEWFACTORY_H__
