/***************************************************************************
                                kfnkeys.cpp
                             -------------------
    copyright            : (C) 2000 by Shie Erlich & Rafi Yanai
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

#include "kfnkeys.h"

#include "../defaults.h"
#include "../kractions.h"
#include "../Panel/listpanelactions.h"
#include "abstractmainwindow.h"

#include <kdebug.h>
#include <klocale.h>
#include <kglobalsettings.h>

#include <QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QFontMetrics>


struct KFnKeys::Button : public QPushButton
{
    Button(QWidget *parent, QString label, QString actionName) :
            QPushButton(parent),
            label(label), actionName(actionName) {}

    QString label;
    QString actionName;
};


KFnKeys::KFnKeys(QWidget *parent, AbstractMainWindow *mainWindow) :
        QWidget(parent), mainWindow(mainWindow)
{
    setFont(KGlobalSettings::generalFont());
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    addButton(i18n("Term"), "F2_Terminal");
    addButton(i18n("View"), "F3_View");
    addButton(i18n("Edit"), "F4_Edit");
    addButton(i18n("Copy"), "F5_Copy");
    addButton(i18n("Move"), "F6_Move");
    addButton(i18n("Mkdir"), "F7_Mkdir");
    addButton(i18n("Delete"), "F8_Delete");
    addButton(i18n("Rename"), "F9_Rename");
    addButton(i18n("Quit"), "F10_Quit");

    updateShortcuts();
}

void KFnKeys::addButton(QString label, QString actionName)
{
    Button *button = new Button(this, label, actionName);
    if (QAction *action = mainWindow->action(button->actionName))
        connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
    else
        kDebug()<<"no such action:"<<actionName;
    buttons << button;
    layout->addWidget(button);
}

void KFnKeys::updateShortcuts()
{
    foreach(Button *button, buttons) {
        if (QAction *action = mainWindow->action(button->actionName))
            button->setText(action->shortcut().toString() + ' ' + button->label);
        else
            kDebug()<<"no such action:"<<button->actionName;
    }
}

#include "kfnkeys.moc"
