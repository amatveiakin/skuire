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

#ifndef __VFILEDIRLISTER_H__
#define __VFILEDIRLISTER_H__

#include "abstractdirlister.h"

class VfileContainer;
class vfile;

class VfileDirLister : public AbstractDirLister
{
    Q_OBJECT
public:
    VfileDirLister();

    virtual bool isRoot();
    virtual int numItems();
    virtual KFileItemList items();

    void setFiles(VfileContainer*);

private slots:
    void addedVfile(vfile*);
    void updatedVfile(vfile*);

private:
    VfileContainer *_files;
};


#endif // __VFILEDIRLISTER_H__