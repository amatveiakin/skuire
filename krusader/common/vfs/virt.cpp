/***************************************************************************
                virt.cpp - code based on panelfunc.cpp
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

#include "vfs_p.h"

namespace VFS
{


KUrl getVirtualBaseURL(KUrl srcBaseUrl, KUrl::List srcUrls, KUrl dest)
{
    if (!srcBaseUrl.isValid())
        return KUrl();
    if (srcUrls.isEmpty())
        return KUrl();
    if (!isVirtUrl(srcBaseUrl) || isVirtUrl(dest))
        return KUrl();

    KUrl::List urls = srcUrls;

    KUrl base = urls[0].upUrl();

    if (isVirtUrl(base))  // is it a virtual subfolder?
        return KUrl();          // --> cannot keep the directory structure

    for (int i = 1; i < urls.count(); i++) {
        KUrl url = urls[i];
        if (base.isParentOf(url))
            continue;
        else if (base.protocol() != url.protocol())
            return KUrl();
        else { // base isn't the same, nor a parent of url
               // so url needs to be a parent of base
            if (url.isParentOf(base))
                base = url;
            else
                return KUrl();
        }
    }

    return base;
}

} // namespace VFS
