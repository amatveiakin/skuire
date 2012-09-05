/***************************************************************************
                    kurllistrequester.cpp  -  description
                             -------------------
    copyright            : (C) 2005 by Csaba Karai
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

#include "kurllistrequester.h"
#include "../VFS/vfs.h"
#include <QtGui/QPixmap>
#include <QtGui/QCursor>
#include <QtGui/QLayout>
#include <QGridLayout>
#include <QKeyEvent>
#include <kfiledialog.h>
#include <kmenu.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>

#define DELETE_ITEM_ID    100

KURLListRequester::KURLListRequester(Mode requestMode, QWidget *parent)
    : QWidget(parent), mode(requestMode)
{
    // Creating the widget

    QGridLayout *urlListRequesterGrid = new QGridLayout(this);
    urlListRequesterGrid->setSpacing(0);
    urlListRequesterGrid->setContentsMargins(0, 0, 0, 0);

    urlLineEdit = new KLineEdit(this);
    urlListRequesterGrid->addWidget(urlLineEdit, 0, 0);

    urlListBox = new KrListWidget(this);
    urlListBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
    urlListRequesterGrid->addWidget(urlListBox, 1, 0, 1, 3);

    urlAddBtn = new QToolButton(this);
    urlAddBtn->setText("");
    urlAddBtn->setIcon(KIcon("arrow-down"));
    urlListRequesterGrid->addWidget(urlAddBtn, 0, 1);

    urlBrowseBtn = new QToolButton(this);
    urlBrowseBtn->setText("");
    urlBrowseBtn->setIcon(KIcon("folder"));
    urlListRequesterGrid->addWidget(urlBrowseBtn, 0, 2);

    // add shell completion

    completion.setMode(KUrlCompletion::FileCompletion);
    urlLineEdit->setCompletionObject(&completion);

    // connection table

    connect(urlAddBtn, SIGNAL(clicked()), this, SLOT(slotAdd()));
    connect(urlBrowseBtn, SIGNAL(clicked()), this, SLOT(slotBrowse()));
    connect(urlLineEdit, SIGNAL(returnPressed(const QString&)), this, SLOT(slotAdd()));
    connect(urlListBox, SIGNAL(itemRightClicked(QListWidgetItem *, const QPoint &)), this,
            SLOT(slotRightClicked(QListWidgetItem *, const QPoint &)));
    connect(urlLineEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(changed()));
}

void KURLListRequester::slotAdd()
{
    QString text = urlLineEdit->text().simplified();
    if (text.length()) {
        QString error;
        emit checkValidity(text, error);

        if (!error.isNull())
            KMessageBox::error(this, error);
        else {
            urlListBox->addItem(text);
            urlLineEdit->clear();
            emit changed();
        }
    }
}

void KURLListRequester::slotBrowse()
{
    KUrl url;
    switch (mode) {
        case RequestFiles:
            url = KFileDialog::getOpenUrl(KUrl(), QString(), this);
            break;
        case RequestDirs:
            url = KFileDialog::getExistingDirectoryUrl(KUrl(), this);
            break;
    }
    if (!url.isEmpty())
        urlLineEdit->setText(url.pathOrUrl());
    urlLineEdit->setFocus();
}

void KURLListRequester::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Delete) {
        if (urlListBox->hasFocus()) {
            deleteSelectedItems();
            return;
        }
    }

    QWidget::keyPressEvent(e);
}

void KURLListRequester::deleteSelectedItems()
{
    QList<QListWidgetItem *> delList = urlListBox->selectedItems();
    for (int i = 0; i != delList.count(); i++)
        delete delList[ i ];
    emit changed();
}

void KURLListRequester::slotRightClicked(QListWidgetItem *item, const QPoint &pos)
{
    if (item == 0)
        return;

    KMenu popupMenu(this);
    QAction * menuAction = popupMenu.addAction(i18n("Delete"));

    if (menuAction == popupMenu.exec(pos)) {
        if (item->isSelected())
            deleteSelectedItems();
        else {
            delete item;
            emit changed();
        }
    }
}

KUrl::List KURLListRequester::urlList()
{
    KUrl::List urls;

    QString text = urlLineEdit->text().simplified();
    if (!text.isEmpty()) {
        QString error;
        emit checkValidity(text, error);
        if (error.isNull())
            urls.append(KUrl(text));
    }

    for (int i = 0; i != urlListBox->count(); i++) {
        QListWidgetItem *item = urlListBox->item(i);

        QString text = item->text().simplified();

        QString error;
        emit checkValidity(text, error);
        if (error.isNull())
            urls.append(KUrl(text));
    }

    return urls;
}

void KURLListRequester::setUrlList(KUrl::List urlList)
{
    urlLineEdit->clear();
    urlListBox->clear();

    KUrl::List::iterator it;

    for (it = urlList.begin(); it != urlList.end(); ++it)
        urlListBox->addItem(it->pathOrUrl());

    emit changed();
}

#include "kurllistrequester.moc"
