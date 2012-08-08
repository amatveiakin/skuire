/***************************************************************************
                   viewtype.h
                 -------------------
copyright            : (C) 2000-2008 by Csaba Karai
                       (C) 2012 by Jan Lepper
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

                                         H e a d e r    F i l e

***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef __VIEWTYPE_H__
#define __VIEWTYPE_H__

#include <QString>
#include <QKeySequence>


class ViewType
{
public:

    ViewType(int id, QString name, QString desc, QString icon, QKeySequence shortcut);
    virtual ~ViewType() {}

    inline int                     id()                    {
        return m_id;
    }
    inline QString                 name()                  {
        return m_name;
    }
    inline QString                 description()           {
        return m_description;
    }
    inline QString                 icon()                  {
        return m_icon;
    }
    inline QKeySequence            shortcut()              {
        return m_shortcut;
    }

protected:
    int                            m_id;
    QString                        m_name;
    QString                        m_description;
    QString                        m_icon;
    QKeySequence                   m_shortcut;
};

#endif // __VIEWTYPE_H__
