/***************************************************************************
                            krpreviewjob.cpp
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

#include "krpreviewjob.h"
#include "krpreviews.h"

#include "krview.h"
#include "../defaults.h"

#include <stdio.h>
#include <QWidget>

#define ASSERT(what) if(!what) abort();

// how much items to process by a single job
// view becomes unresponsive during load if set too high
#define MAX_CHUNK_SIZE 50

//TODO: if new items become visible, give them priority

KrPreviewJob::KrPreviewJob(KrPreviews *parent) : _job(0), _parent(parent)
{
    _timer.setSingleShot(true);
    _timer.setInterval(0);
    connect(&_timer, SIGNAL(timeout()), SLOT(slotStartJob()));
}

KrPreviewJob::~KrPreviewJob()
{
    doKill();
}

void KrPreviewJob::scheduleItem(KFileItem item)
{
    if(!_scheduled.contains(item)) {
        _scheduled.append(item);
        setTotalAmount(KJob::Files, totalAmount(KJob::Files) + 1);
    }
    if(!_job)
        _timer.start();
}

void KrPreviewJob::updateItem(KFileItem oldItem, KFileItem newItem)
{
    int index = _scheduled.indexOf(oldItem);
    if(index != -1 && oldItem != newItem)
        removeItem(oldItem);
    scheduleItem(newItem);
}

void KrPreviewJob::removeItem(KFileItem item)
{
    setTotalAmount(KJob::Files, totalAmount(KJob::Files) - _scheduled.removeAll(item));
    if(_job) {
        doKill();
        if(!_scheduled.isEmpty())
            _timer.start();
    }
    if(_scheduled.isEmpty())
        emitResult();
}

void KrPreviewJob::slotFailed(const KFileItem & item)
{
    slotGotPreview(item, QPixmap());
}

void KrPreviewJob::slotGotPreview(const KFileItem & item, const QPixmap & preview)
{
    _scheduled.removeOne(item);

    _parent->addPreview(item, preview);

    setProcessedAmount(KJob::Files, processedAmount(KJob::Files) + 1);
    emitPercent(processedAmount(KJob::Files), totalAmount(KJob::Files));
}

void KrPreviewJob::slotStartJob()
{
    ASSERT(_job == 0);
    ASSERT(!_scheduled.isEmpty());

    sort();

    int size = _parent->_view->fileIconSize();

    KFileItemList list;
    for(int i = 0; i < _scheduled.count() && i < MAX_CHUNK_SIZE; i++)
        list << _scheduled[i];

    QStringList allPlugins = KIO::PreviewJob::availablePlugins();
    _job = new KIO::PreviewJob(list, QSize(size, size), &allPlugins);
        _job->setOverlayIconAlpha(0);
        _job->setOverlayIconSize(0);
        _job->setScaleType(KIO::PreviewJob::ScaledAndCached);
    connect(_job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)), SLOT(slotGotPreview(const KFileItem&, const QPixmap&)));
    connect(_job, SIGNAL(failed(const KFileItem&)), SLOT(slotFailed(const KFileItem&)));
    connect(_job, SIGNAL(result(KJob*)), SLOT(slotJobResult(KJob*)));
}

void KrPreviewJob::slotJobResult(KJob *job)
{
    (void) job;

    if(!disconnect(_job, 0, this, 0))
        abort();

    _job = 0;

    if(_scheduled.isEmpty())
        emitResult();
    else
         _timer.start();
}

// move currently visible items to beginning of the list
void KrPreviewJob::sort()
{
    QSet<KFileItem> visibleSet =
        QSet<KFileItem>::fromList(_parent->_view->getVisibleItems());
    for(int i = 0, visible_end = 0; i < _scheduled.count(); i++) {
        KFileItem item = _scheduled[i];
        if(visibleSet.contains(item)) {
            if(i != visible_end)
                _scheduled.move(i, visible_end);
            visible_end++;
        }
    }
}

bool KrPreviewJob::doKill()
{
    _timer.stop();
    if(_job) {
        if(!disconnect(_job, 0, this, 0))
            abort();
        if(!_job->kill())
            abort();
        _job = 0;
    }
    return true;
}
