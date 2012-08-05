/***************************************************************************
                          krcalcspacedialog.cpp  -  description
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

                                                  S o u r c e    F i l e

***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "krcalcspacedialog.h"

#include <QtCore/QTimer>
#include <QPointer>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

#include <klocale.h>

#include "../VFS/krpermhandler.h"
#include "abstractview.h"
#include "abstractlistpanel.h"
KrCalcSpaceDialog::KrCalcSpaceDialog(QWidget* parent, const KFileItemList& lstItems) :
        KDialog(parent), m_lstItems(lstItems), m_dirSizeJob(0), m_finished(false), m_updateTimer(0),
        m_totalSize(0), m_totalFiles(0), m_totalDirs(0), m_resultIsAccurate(true)
{
    setButtons(KDialog::Ok | KDialog::Cancel);
    setDefaultButton(KDialog::Ok);
    setWindowTitle(i18n("Calculate Occupied Space"));
    setWindowModality(Qt::WindowModal);
    // the dialog: The Ok button is hidden until it is needed
    showButton(KDialog::Ok, false);
    QWidget * mainWidget = new QWidget(this);
    setMainWidget(mainWidget);
    QVBoxLayout *topLayout = new QVBoxLayout(mainWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(spacingHint());

    m_label = new QLabel("", mainWidget);
    showResult(); // fill m_label with something useful
    topLayout->addWidget(m_label);
    topLayout->addStretch(10);

    connect(this, SIGNAL(cancelClicked()), this, SLOT(reject()));
}

KrCalcSpaceDialog::~KrCalcSpaceDialog()
{
    if (m_dirSizeJob)
        m_dirSizeJob->kill();
}

void KrCalcSpaceDialog::getDataFromJob()
{
    m_totalSize         = m_dirSizeJob->totalSize();
    m_totalFiles        = m_dirSizeJob->totalFiles();
    m_totalDirs         = m_dirSizeJob->totalSubdirs();
    m_resultIsAccurate  = true;
}

void KrCalcSpaceDialog::updateData()
{
    if (!m_dirSizeJob)
        return;

    if (m_dirSizeJob->error()) {
        m_resultIsAccurate = false;
        calculationFinished(m_dirSizeJob);
    } else {
        getDataFromJob();
        showResult();
    }
}

void KrCalcSpaceDialog::calculationFinished(KJob* job)
{
    Q_ASSERT(job == m_dirSizeJob);
    m_finished = true;
    m_updateTimer->stop();
    showButton(KDialog::Cancel, false);
    showButton(KDialog::Ok, true);
    getDataFromJob();
    showResult(); // show final result
}

void KrCalcSpaceDialog::showResult()
{
    QString msg;
    QString fileName = (m_lstItems.count() == 1) ? i18n("Name: %1\n", m_lstItems.first().fileName()) : QString("");
    msg = fileName + i18n("Total occupied space: %1", KIO::convertSize(m_totalSize));
    if (m_totalSize >= 1024)
        msg += i18np(" (%1 byte)", " (%1 bytes)", KRpermHandler::parseSize(m_totalSize));
    msg += '\n';
    msg += i18np("in %1 directory", "in %1 directories", m_totalDirs);
    msg += ' ';
    msg += i18np("and %1 file", "and %1 files", m_totalFiles);
    if (!m_finished)
        msg += i18n("\nContinuing calculation...");
    else if (!m_resultIsAccurate)
        msg += i18n("\nSome subfolders were inaccessible. The result may be inaccurate.");
    m_label->setText(msg);
}

int KrCalcSpaceDialog::exec()
{
    m_dirSizeJob = KIO::directorySize(m_lstItems);
    m_updateTimer = new QTimer(this);
    m_updateTimer->start(100);
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(updateData()));
    connect(m_dirSizeJob, SIGNAL(result(KJob*)), this, SLOT(calculationFinished(KJob*)));
    return KDialog::exec(); // show the dialog
}


#include "krcalcspacedialog.moc"
