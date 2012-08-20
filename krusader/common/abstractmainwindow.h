/*****************************************************************************
 * Copyright (C) 2010 Jan Lepper <dehtris@yahoo.de>                          *
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

#ifndef __ABSTRACTMAINWINDOW_H__
#define __ABSTRACTMAINWINDOW_H__

class QWidget;
class AbstractView;
class QAction;
class KActionCollection;


// abstract interface to the main window
class AbstractMainWindow
{
public:
    virtual ~AbstractMainWindow() {}
    virtual QWidget *widget() = 0;
    virtual AbstractView *activeView() = 0;
    virtual KActionCollection *actions() = 0;
};

#endif // __ABSTRACTMAINWINDOW_H__
