/***************************************************************************
                                konfigurator.h
                             -------------------
    begin                : Thu May 4 2000
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

                                                     H e a d e r    F i l e

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef KONFIGURATOR_H
#define KONFIGURATOR_H

#include "konfiguratorpage.h"
#include <qwidget.h>
//Added by qt3to4:
#include <Q3PtrList>
#include <kdialog.h>
#include <kpagedialog.h>
#include <qtimer.h>

class QLineEdit;
class QString;

class Konfigurator : public KPageDialog
{
   Q_OBJECT

public:
  Konfigurator( bool f=false, int startPage = 0 ); // true if Konfigurator is run for the first time
  ~Konfigurator() {};

  bool isGUIRestartNeeded() { return restartGUI; }

protected:
  void newPage(KonfiguratorPage *, const QString &, const QString &, const KIcon & );// adds widget and connects to slot
  void createLayout( int startPage );

protected slots:
  void slotUser1();
  void slotApply();   // actually used for defaults
  void slotCancel();
  void slotApplyEnable();
  bool slotPageSwitch();
  void slotRestorePage();

private:
  Q3PtrList<KPageWidgetItem>  kgPages;
  bool                        firstTime;
  KPageWidgetItem            *lastPage;
  bool                        internalCall;
  QTimer                      restoreTimer;
  bool                        restartGUI;
};

#endif

