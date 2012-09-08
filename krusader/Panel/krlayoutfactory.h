/***************************************************************************
                   krlayoutfactory.h
                 -------------------
copyright            : (C) 2010 by Jan Lepper
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

#ifndef KRLAYOUTFACTORY_H
#define KRLAYOUTFACTORY_H

#include <QString>
#include <QHash>
#include <QDomElement>

class QWidget;
class QLayout;
class QBoxLayout;

class KrLayoutFactory
{
public:
    KrLayoutFactory(QWidget *panel, QHash<QString, QWidget*> &widgets) : panel(panel), widgets(widgets) {}
    // creates the layout and adds the widgets to it
    QLayout *createLayout(QString layoutName = QString());

    static QStringList layoutNames();
    static QString layoutDescription(QString layoutName);

private:
    QBoxLayout *createLayout(QDomElement e, QWidget *parent);
    QWidget *createFrame(QDomElement e, QWidget *parent);

    static bool parseFiles();
    static bool parseFile(QString path, QDomDocument &doc);
    static bool parseRessource(QString path, QDomDocument &doc);
    static bool parseContent(QByteArray content, QString fileName, QDomDocument &doc);
    static void getLayoutNames(QDomDocument doc, QStringList &names);
    static QDomElement findLayout(QDomDocument doc, QString layoutName);

    QWidget *panel;
    QHash<QString, QWidget*> &widgets;

    static bool _parsed;
    static QDomDocument _mainDoc;
    static QList<QDomDocument> _extraDocs;
};

#endif
