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

#include "virtualrenamejob.h"

#include "vfs_p.h"
#include "../../VFS/virt_vfs.h"

#include <kdebug.h>

//FIXME overwriting is done without confirmation

namespace VFS
{


VirtualRenameJob::VirtualRenameJob(QString src, QString dest) :
    src(src), dest(dest)
{
    startTimer.setSingleShot(true);
    startTimer.setInterval(0);
    connect(&startTimer, SIGNAL(timeout()), SLOT(slotStart()));
}

void VirtualRenameJob::start()
{
    startTimer.start();
}

void VirtualRenameJob::slotStart()
{
    virt_vfs virtfs(0, false);
    if (virtfs.vfs_refresh(KUrl(QString("virt:/"))))
        virtfs.vfs_rename(src, dest);
    else
        abort();

    emitResult();
}

} // namespace VFS
