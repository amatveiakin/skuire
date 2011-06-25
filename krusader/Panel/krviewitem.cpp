/*****************************************************************************
 * Copyright (C) 2000-2002 Shie Erlich <erlich@users.sourceforge.net>        *
 * Copyright (C) 2000-2002 Rafi Yanai <yanai@users.sourceforge.net>          *
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

#include "krviewitem.h"
#include "../VFS/krpermhandler.h"
#include <klocale.h>
#include <kmimetype.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <QPixmap>

#define PROPS static_cast<const KrViewProperties*>(_viewProperties)

static int compareNumbers(QString& aS1, int& aPos1, QString& aS2, int& aPos2);

KrViewItem::KrViewItem(vfile *vf, const KrViewProperties* properties):
        _vf(vf), dummyVfile(false), _viewProperties(properties), _hasExtension(false), _hidden(false), _extension("")
{
    if (vf) {
        // check if the file has an extension
        const QString& vfName = vf->vfile_getName();
        int loc = vfName.lastIndexOf('.');
        if (loc > 0) { // avoid mishandling of .bashrc and friend
            // check if it has one of the predefined 'atomic extensions'
            for (QStringList::const_iterator i = PROPS->atomicExtensions.begin(); i != PROPS->atomicExtensions.end(); ++i) {
                if (vfName.endsWith(*i)) {
                    loc = vfName.length() - (*i).length();
                    break;
                }
            }
            _name = vfName.left(loc);
            _extension = vfName.mid(loc + 1);
            _hasExtension = true;
        }

        if (vfName.startsWith('.'))
            _hidden = true;
    }
}

const QString& KrViewItem::name(bool withExtension) const
{
    if (!withExtension && _hasExtension) return _name;
    else return _vf->vfile_getName();
}

QString KrViewItem::dateTime() const
{
    // convert the time_t to struct tm
    time_t time = _vf->vfile_getTime_t();
    struct tm* t = localtime((time_t *) & time);

    QDateTime tmp(QDate(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday), QTime(t->tm_hour, t->tm_min));
    return KGlobal::locale()->formatDateTime(tmp);
}

QPixmap KrViewItem::icon()
{
#if 0
    QPixmap *p;

    // This is bad - very bad. the function must return a valid reference,
    // This is an interface flow - shie please fix it with a function that return QPixmap*
    // this way we can return 0 - and do our error checking...

    // shie answers: why? what's the difference? if we return an empty pixmap, others can use it as it
    // is, without worrying or needing to do error checking. empty pixmap displays nothing
#endif
    if (dummyVfile || !_viewProperties->displayIcons)
        return QPixmap();
    else {
        KrView::Item item(_vf->toFileItem());
        return KrView::getIcon(&item, true);
    }
}
