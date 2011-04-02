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

#include "calcspacethread.h"

#include "../VFS/vfs.h"
#include "../VFS/krvfshandler.h"

CalcSpaceThread::CalcSpaceThread(KUrl url, KUrl::List items)
        : m_totalSize(0), m_currentSize(0), m_totalFiles(0), m_totalDirs(0),
          m_items(items), m_url(url), m_stop(false) {}


void CalcSpaceThread::getStats(KIO::filesize_t  &totalSize,
                                             unsigned long &totalFiles,
                                             unsigned long &totalDirs) const
{
    QMutexLocker locker(&m_mutex);
    totalSize = m_totalSize + m_currentSize;
    totalFiles = m_totalFiles;
    totalDirs = m_totalDirs;
}

void CalcSpaceThread::updateItems(CalcSpaceClient *client) const
{
    QMutexLocker locker(&m_mutex);
    foreach(KUrl item, m_items) {
        QHash<KUrl, KIO::filesize_t>::ConstIterator it = m_sizes.find(item);
        if(it != m_sizes.end())
            client->updateItemSize(item, *it);
    }
}

void CalcSpaceThread::stop()
{
    m_stop = true;
}

void CalcSpaceThread::run()
{
    if (m_items.isEmpty())
        return;

    vfs *files = KrVfsHandler::getVfs(m_url);
    if(!files->vfs_refresh(m_url))
        return;

    for (KUrl::List::ConstIterator it = m_items.begin(); it != m_items.end(); ++it) {
        files->vfs_calcSpace((*it).fileName(), &m_currentSize, &m_totalFiles, &m_totalDirs , & m_stop);

        if (m_stop)
            break;

        m_mutex.lock();
        m_sizes[*it] = m_currentSize;
        m_totalSize += m_currentSize;
        m_currentSize = 0;
        m_mutex.unlock();
    }

    delete files;
}
