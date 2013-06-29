/*****************************************************************************
 * Copyright (C) 2002 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2002 Rafi Yanai <yanai@users.sourceforge.net>               *
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

#ifndef KRPREVIEWPOPUP_H
#define KRPREVIEWPOPUP_H

#include <QtGui/QMenu>
#include <QtGui/QPixmap>
#include <kfileitem.h>
#include <kurl.h>

class KrPreviewPopup : public QMenu
{
    Q_OBJECT

public:
    KrPreviewPopup();

    void setItems(KFileItemList items);
public slots:
    void addPreview(const KFileItem& file, const QPixmap& preview);
    void view(QAction *);

protected:
    virtual void showEvent(QShowEvent *event);

    QAction * prevNotAvailAction;
    KFileItemList files;
    bool jobStarted;

private:
    class ProxyStyle;
    static const int MAX_SIZE =400;
    static const short MARGIN =5;
};

#endif
