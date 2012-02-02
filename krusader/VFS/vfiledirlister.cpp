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

#include "vfiledirlister.h"

#include "vfilecontainer.h"
#include "vfile.h"

#include "../krglobal.h"

VfileDirLister::VfileDirLister() : _files(0)
{
}

bool VfileDirLister::isRoot()
{
    if(_files)
        krOut<<_files->isRoot()<<endl;
    return _files ? _files->isRoot() : false;
}

int VfileDirLister::numItems()
{
    return _files ? _files->numVfiles() : 0;
}

KFileItemList VfileDirLister::items()
{
    if(!_files)
        return KFileItemList();

    KFileItemList list;

    foreach(vfile *vf, _files->vfiles())
        list << vf->toFileItem();

    return list;
}

KFileItem VfileDirLister::findByName(QString name)
{
    if (_files) {
        if (vfile *vf = _files->search(name))
            return vf->toFileItem();
        else
            return KFileItem();
    } else
        return KFileItem();
}

void VfileDirLister::setFiles(VfileContainer *files)
{
    if(_files && files == _files)
        return;

    emit clear();

    if(_files)
        disconnect(files, 0, this, 0);

    _files = files;
    if(files) {
        connect(_files, SIGNAL(startUpdate()), SIGNAL(completed()));
        connect(_files, SIGNAL(cleared()), SIGNAL(clear()));
        connect(_files, SIGNAL(addedVfile(vfile*)), SLOT(addedVfile(vfile*)));
        connect(_files, SIGNAL(deletedVfile(const QString&)), SIGNAL(itemDeleted(const QString&)));
        connect(_files, SIGNAL(updatedVfile(vfile*)), SLOT(updatedVfile(vfile*)));
    }

}

void VfileDirLister::addedVfile(vfile *vf)
{
    if(vf) {
        KFileItemList items;
        items << vf->toFileItem();
        emit newItems(items);
    }
}

void VfileDirLister::updatedVfile(vfile *vf)
{
    if(vf)
        emit refreshItem(vf->toFileItem());
}
