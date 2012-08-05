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

#ifndef KRUSADERAPP_H
#define KRUSADERAPP_H

#include <kapplication.h>
class QFocusEvent;

// declare a dummy kapplication, just to get Qt's focusin focusout events
class KrusaderApp: public KApplication
{
    Q_OBJECT

public:
    KrusaderApp();
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void runKonfigurator(bool firstTime);

    static KrusaderApp* self();

signals:
    void windowActive();
    void windowInactive();
    void configChanged();

public slots:
    void runKonfigurator() {
        runKonfigurator(false);
    }

protected slots:
    void slotConfigChanged(bool isGUIRestartNeeded);
};

#endif // KRUSADERAPP_H
