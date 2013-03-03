/*****************************************************************************
 * Copyright (C) 2004 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2004 Rafi Yanai <yanai@users.sourceforge.net>               *
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

#ifndef DIRHISTORYQUEUE_H
#define DIRHISTORYQUEUE_H

#include <QObject>
#include <QStringList>
#include <kurl.h>
#include <kconfiggroup.h>

class AbstractListPanel;

class DirHistoryQueue : public QObject
{
    Q_OBJECT
public:
    DirHistoryQueue(AbstractListPanel *panel);
    ~DirHistoryQueue();

    void clear();
    int state() {
        return _state;
    }
    int currentPos() {
        return _currentPos;
    }
    int count() {
        return _urlQueue.count();
    }
    KUrl currentUrl();
    void setCurrentUrl(KUrl url);
    const KUrl& get(int pos) {
        return _urlQueue[pos];
    }
    void add(KUrl url, KUrl currentItem);
    void pushBackRoot(); // add root dir to beginning of history
    bool gotoPos(int pos);
    bool goBack();
    bool goForward();
    bool canGoBack() {
        return _currentPos < count() - 1;
    }
    bool canGoForward() {
        return _currentPos > 0;
    }
    KUrl currentItem(); // current item of the view

    void save(KConfigGroup cfg);
    bool restore(KConfigGroup cfg);

public slots:
    void saveCurrentItem();

private:
    AbstractListPanel* _panel;
    int _state; // increments when we move inside the history, or a new item is added
    int _currentPos;
    KUrl::List _urlQueue;
    KUrl::List _currentItems;
};

#endif
