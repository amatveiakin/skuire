/***************************************************************************
                         filterbase.h  -  description
                             -------------------
    copyright            : (C) 2005 + by Csaba Karai
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

#ifndef FILTERBASE_H
#define FILTERBASE_H

#include "filtersettings.h"
#include "../VFS/krquery.h"

#include <QtCore/QString>
#include <QCheckBox>
#include <QComboBox>

class FilterTabs;

class FilterBase
{
public:
    virtual ~FilterBase()   {}

    virtual void            queryAccepted() = 0;
    virtual QString         name() = 0;
    virtual FilterTabs *    filterTabs() = 0;
    virtual bool            getSettings(FilterSettings&) = 0;
    virtual void            applySettings(const FilterSettings&) = 0;

protected:
    static QString getPermissionsString(const QCheckBox* checkbox, const QString& perm) {
        switch (checkbox->checkState()) {
            case Qt::Unchecked:
                return QString("-");
            case Qt::PartiallyChecked:
                return QString("?");
            case Qt::Checked:
                return perm;
        }
        return "?";
    }

    static void setCheckBoxValue(QCheckBox *checkbox, QString value) {
        if (value == "-")
            checkbox->setCheckState(Qt::Unchecked);
        else if (value == "?")
            checkbox->setCheckState(Qt::PartiallyChecked);
        else
            checkbox->setCheckState(Qt::Checked);
    }

    static void setComboBoxValue(QComboBox *combobox, QString value) {
        int idx = combobox->findText(value);
        combobox->setCurrentIndex(idx < 0 ? 0 : idx);
    }
};

#endif /* FILTERBASE_H */
