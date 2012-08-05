/***************************************************************************
                          krcalcspacedialog.h  -  description
                             -------------------
    begin                : Fri Jan 2 2004
    copyright            : (C) 2004 by Shie Erlich & Rafi Yanai
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

#ifndef KRCALCSPACEDIALOG_H
#define KRCALCSPACEDIALOG_H

#include <QPointer>

#include <kdialog.h>
#include <kio/directorysizejob.h>
#include "calcspacethread.h"

class AbstractListPanel;
class AbstractView;

class QLabel;
class QTimer;

/**
 * Dialog calculating showing the number of files and directories and its total
 * size in a dialog. If wanted, the dialog appears after 3 seconds of
 * calculation, to avoid a short appearance if the result was found quickly.
 * Computes the result in a different thread.
 */
class KrCalcSpaceDialog : public KDialog
{
    Q_OBJECT

    const KFileItemList m_lstItems;
    QPointer<KIO::DirectorySizeJob> m_dirSizeJob;
    bool m_finished;
    QTimer* m_updateTimer;

    KIO::filesize_t m_totalSize;
    unsigned long m_totalFiles;
    unsigned long m_totalDirs;
    bool m_resultIsAccurate;  // false in we were unable to access some subfolders

    QLabel* m_label;

    void getDataFromJob();
    void showResult(); // show the current result in the dialog

protected slots:
    void updateData();
    void calculationFinished(KJob* job);  // the job has finished

public:
    KrCalcSpaceDialog(QWidget* parent, const KFileItemList& lstItems);
    ~KrCalcSpaceDialog();

public slots:
    int exec(); // start calculation
};

#endif
