/***************************************************************************
                            krpreviews.cpp
                         -------------------
copyright            : (C) 2009 by Jan Lepper
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

#include "krpreviews.h"
#include "krpreviewjob.h"

#include "krcolorcache.h"
#include "krview.h"
#include "../defaults.h"


KrPreviews::KrPreviews(KrView *view) :  _view(view), _job(0)
{
    connect(&KrColorCache::getColorCache(), SIGNAL(colorsRefreshed()), SLOT(slotRefreshColors()));
}

KrPreviews::~KrPreviews()
{
    clear();
}

void KrPreviews::clear()
{
    if(_job) {
        _job->kill(KJob::EmitResult);
        _job = 0;
    }
}

void KrPreviews::update()
{
    clear();
    newItems(_view->getItems());
}

void KrPreviews::newItems(const KFileItemList& items)
{
    foreach(KFileItem item, items)
        updatePreview(item);
}

void KrPreviews::itemsUpdated(const QList<QPair<KFileItem, KFileItem> >& items)
{
    for(int i = 0; i < items.count(); i++) {
        QPair<KFileItem, KFileItem> pair = items[i];
        startJob();
        _job->updateItem(pair.first, pair.second);
    }
}

void KrPreviews::itemsDeleted(const KFileItemList& items)
{
    foreach(KFileItem item, items)
        deletePreview(item);
}

void KrPreviews::deletePreview(KFileItem item)
{
    if(_job)
        _job->removeItem(item);
}

void KrPreviews::updatePreview(KFileItem item)
{
    startJob();
    _job->scheduleItem(item);
}

void KrPreviews::startJob()
{
    if(!_job) {
        _job = new KrPreviewJob(this);
        connect(_job, SIGNAL(result(KJob*)), SLOT(slotJobResult(KJob*)));
        emit jobStarted(_job);
    }
}

void KrPreviews::slotJobResult(KJob *job)
{
    (void) job;
    _job = 0;
}

void KrPreviews::slotRefreshColors()
{
    clear();
    update();
}

void KrPreviews::addPreview(KFileItem item, const QPixmap &preview)
{
    if(!preview.isNull())
        emit gotPreview(item, preview);
}
