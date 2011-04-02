/*****************************************************************************
 * Copyright (C) 2011 Jan Lepper <jan_lepper@gmx.de>                         *
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

#ifndef __CALCSPACETHREAD_H__
#define __CALCSPACETHREAD_H__

#include <kurl.h>
#include <kio/global.h>
#include <QThread>
#include <QMutex>

class CalcSpaceClient;

class CalcSpaceThread : public QThread
{
public:
    CalcSpaceThread(KUrl url, KUrl::List items);

    void updateItems(CalcSpaceClient *client) const;
    void getStats(KIO::filesize_t  &totalSize,
                    unsigned long &totalFiles,
                    unsigned long &totalDirs) const;
    void run(); // start calculation
    void stop(); // stop it. Thread continues until vfs_calcSpace returns

private:
    KIO::filesize_t m_totalSize;
    KIO::filesize_t  m_currentSize;
    unsigned long m_totalFiles;
    unsigned long m_totalDirs;
    KUrl::List m_items;
    QHash <KUrl, KIO::filesize_t> m_sizes;
    KUrl m_url;
    mutable QMutex m_mutex;
    bool m_stop;
};

class CalcSpaceClient
{
    friend class CalcSpaceThread;

    virtual void updateItemSize(KUrl url, KIO::filesize_t size) = 0;
};

#endif // __CALCSPACETHREAD_H__
