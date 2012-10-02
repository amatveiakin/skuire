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

#include "rename.h"

#include "vfs_p.h"
#include "virtualrenamejob.h"

#include <kio/copyjob.h>

namespace VFS
{


KJob *rename(KFileItem file, QString newName)
{
    if (isVirtUrl(file.url())) {
        QString virtualDir = virtualDirFromUrl(file.url());
        Q_ASSERT(!virtualDir.isEmpty());
        if (!virtualDir.isEmpty()) {
            KJob *job = new VirtualRenameJob(virtualDir, newName);
            job->start();
            return job;
        } else
            return 0;
    } else {
        KUrl dest = file.url();
        dest.setFileName(newName);
        return KIO::moveAs(file.url(), dest);
    }
}


} // namespace VFS
