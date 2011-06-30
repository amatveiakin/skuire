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

#ifndef __ABSTRACTDIRLISTER_H__
#define __ABSTRACTDIRLISTER_H__

#include <QObject>
#include <QList>
#include <QPair>

#include <kfileitem.h>

// This interface is based on KDirLister

class AbstractDirLister : public QObject
{
    Q_OBJECT
public:
  virtual bool isRoot() = 0;
  virtual int numItems() = 0;
  virtual KFileItemList items() = 0;

signals:
  void started();
  void completed();

  /**
   * Signal to clear all items.
   * Make sure to connect to this signal to avoid doubled items.
   */
  void clear();

   /*
   * Signal new items.
   *
   * @param items a list of new items
   */
  void newItems( const KFileItemList& items );

  /**
   * Signal that items have been deleted
   *
   * @param items the list of deleted items
   */
  void itemsDeleted( const KFileItemList& items );

  /**
   * Signal an item to refresh (its mimetype/icon/name has changed).
   * Note: KFileItem::refresh has already been called on those items.
   * @param items the items to refresh. This is a list of pairs, where
   * the first item in the pair is the OLD item, and the second item is the
   * NEW item. This allows to track which item has changed, especially after
   * a renaming.
   */
  void refreshItems( const QList<QPair<KFileItem, KFileItem> >& items );

  //FIXME: remove the following when VfileDirLister is removed
  void itemDeleted(const QString& name);
  void refreshItem(KFileItem item);
};

#endif // __ABSTRACTDIRLISTER_H__
