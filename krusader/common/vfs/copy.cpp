/***************************************************************************
                copy.cpp - code based on panelfunc.cpp
            ------------------------------------------------
copyright            : (C) 2000 by Shie Erlich & Rafi Yanai
                       (C) 2012 by Jan Lepper <jan_lepper@gmx.de>
e-mail               : krusader@users.sourceforge.net
web site             : http://krusader.sourceforge.net
---------------------------------------------------------------------------
Description
***************************************************************************

A

 db   dD d8888b. db    db .d8888.  .d8b.  d8888b. d88888b d8888b.
 88 ,8P' 88  `8D 88    88 88'  YP d8' `8b 88  `8D 88'     88  `8D
 88,8P   88oobY' 88    88 `8bo.   88ooo88 88   88 88ooooo 88oobY'
 88`8b   88`8b   88    88   `Y8b. 88~~~88 88   88 88~~~~~ 88`8b
 88 `88. 88 `88. 88b  d88 db   8D 88   88 88  .8D 88.     88 `88.
 YP   YD 88   YD ~Y8888P' `8888Y' YP   YP Y8888D' Y88888P 88   YD

                                                 S o u r c e    F i l e

***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "copy.h"

#include "abstractjobwrapper.h"
#include "vfs_p.h"

#include "../../VFS/preservingcopyjob.h"
#include "../../VFS/kiojobwrapper.h"
#include "../../defaults.h"
#include "../../Dialogs/krdialogs.h"
#include "../../Queue/queue_mgr.h"

#include <kdebug.h>
#include <klocale.h>
#include <kio/jobuidelegate.h>
#include <kmessagebox.h>

namespace VFS
{


AbstractJobWrapper *copy(KFileItemList files, KUrl &dest, bool confirm, KUrl srcBaseUrl)
{
    PreserveMode pmode = PM_DEFAULT;
    bool queue = false;

    KUrl::List urls = files.urlList();

    if (urls.isEmpty() || !dest.isValid())
        return 0;

    QStringList fileNames; //TODO: remove this
    foreach(const KUrl &url, urls)
        fileNames << url.fileName();

    KUrl virtualBaseURL;

    KConfigGroup group(KGlobal::config(), "Advanced");

    // confirm copy
    if (confirm) {
        bool preserveAttrs = group.readEntry("PreserveAttributes", _PreserveAttributes);
        QString s;

        if (fileNames.count() == 1)
            s = i18n("Copy %1 to:", fileNames.first());
        else
            s = i18n("Copy %1 files to:", fileNames.count());

        // ask the user for the copy dest
        virtualBaseURL = getVirtualBaseURL(srcBaseUrl, urls, dest);
        kDebug()<<virtualBaseURL;
        dest = KChooseDir::getDir(s, dest, srcBaseUrl, queue, preserveAttrs, virtualBaseURL);
        if (dest.isEmpty())
            return 0;   // the user canceled
        if (preserveAttrs)
            pmode = PM_PRESERVE_ATTR;
        else
            pmode = PM_NONE;
    }

    KIOJobWrapper *job = 0;

    if (!virtualBaseURL.isEmpty()) {
        job = KIOJobWrapper::virtualCopy(files, dest, virtualBaseURL, pmode, true);
    } else if (isVirtUrl(dest)) {
        QString destDir = virtualDirFromUrl(dest);
        if (!destDir.isEmpty())
            job = KIOJobWrapper::virtualAdd(urls, destDir);
        else {
            KMessageBox::sorry(0, i18n("%1 is not a virtual directory.", dest.prettyUrl()));
            return 0;
        }
    } else {
        // you can rename only *one* file not a batch,
        // so a batch dest must alwayes be a directory
        if (fileNames.count() > 1)
            dest.adjustPath(KUrl::AddTrailingSlash);
        job = KIOJobWrapper::copy(pmode, urls, dest, true);
        job->setAutoErrorHandlingEnabled(true);
    }

    if (queue)
        QueueManager::currentQueue()->enqueue(job);
    else
        job->start();

    return job;
}


} // namespace VFS
