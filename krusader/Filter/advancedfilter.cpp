/***************************************************************************
                      advancedfilter.cpp  -  description
                             -------------------
    copyright            : (C) 2003 + by Shie Erlich & Rafi Yanai & Csaba Karai
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

#include "advancedfilter.h"

#include <time.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QPixmap>
#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QButtonGroup>
#include <QtGui/QSpinBox>

#include <KLocale>
#include <KDebug>
#include <KMessageBox>
#include <KIconLoader>

#include "../krglobal.h"
#include "../Dialogs/krdialogs.h"

#define USERSFILE  QString("/etc/passwd")
#define GROUPSFILE QString("/etc/group")

AdvancedFilter::AdvancedFilter(FilterTabs *tabs, QWidget *parent) : QWidget(parent), fltTabs(tabs)
{
    QGridLayout *filterLayout = new QGridLayout(this);
    filterLayout->setSpacing(6);
    filterLayout->setContentsMargins(11, 11, 11, 11);

    // Options for size

    QGroupBox *sizeGroup = new QGroupBox(this);
    QSizePolicy sizeGroupPolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    sizeGroupPolicy.setHeightForWidth(sizeGroup->sizePolicy().hasHeightForWidth());
    sizeGroup->setSizePolicy(sizeGroupPolicy);
    sizeGroup->setTitle(i18n("Size"));
    QGridLayout *sizeLayout = new QGridLayout(sizeGroup);
    sizeLayout->setAlignment(Qt::AlignTop);
    sizeLayout->setSpacing(6);
    sizeLayout->setContentsMargins(11, 11, 11, 11);

    minSizeEnabled = new QCheckBox(sizeGroup);
    minSizeEnabled->setText(i18n("At Least"));
    minSizeEnabled->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    sizeLayout->addWidget(minSizeEnabled, 0, 0);

    minSizeAmount = new QSpinBox(sizeGroup);
    minSizeAmount->setRange(0, 999999999);
    minSizeAmount->setEnabled(false);
    sizeLayout->addWidget(minSizeAmount, 0, 1);

    minSizeType = new KComboBox(false, sizeGroup);
    // i18n: @item:inlistbox next to a text field containing the amount
    minSizeType->addItem(i18n("Byte"));
    // i18n: @item:inlistbox next to a text field containing the amount
    minSizeType->addItem(i18n("KiB"));
    // i18n: @item:inlistbox next to a text field containing the amount
    minSizeType->addItem(i18n("MiB"));
    // i18n: @item:inlistbox next to a text field containing the amount
    minSizeType->addItem(i18n("GiB"));
    minSizeType->setEnabled(false);
    sizeLayout->addWidget(minSizeType, 0, 2);

    maxSizeEnabled = new QCheckBox(sizeGroup);
    maxSizeEnabled->setText(i18n("At Most"));
    maxSizeEnabled->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    sizeLayout->addWidget(maxSizeEnabled, 0, 3);

    maxSizeAmount = new QSpinBox(sizeGroup);
    maxSizeAmount->setRange(0, 999999999);
    maxSizeAmount->setEnabled(false);
    sizeLayout->addWidget(maxSizeAmount, 0, 4);

    maxSizeType = new KComboBox(false, sizeGroup);
    // i18n: @item:inlistbox next to a text field containing the amount
    maxSizeType->addItem(i18n("Byte"));
    // i18n: @item:inlistbox next to a text field containing the amount
    maxSizeType->addItem(i18n("KiB"));
    // i18n: @item:inlistbox next to a text field containing the amount
    maxSizeType->addItem(i18n("MiB"));
    // i18n: @item:inlistbox next to a text field containing the amount
    maxSizeType->addItem(i18n("GiB"));
    maxSizeType->setEnabled(false);
    sizeLayout->addWidget(maxSizeType, 0, 5);

    filterLayout->addWidget(sizeGroup, 0, 0, 1, 2);

    // Options for date

    QPixmap iconDate = krLoader->loadIcon("date", KIconLoader::Toolbar, 16);

    QGroupBox *dateGroup = new QGroupBox(this);
    QButtonGroup *btnGroup = new QButtonGroup(dateGroup);
    dateGroup->setTitle(i18n("Date"));
    btnGroup->setExclusive(true);
    QGridLayout *dateLayout = new QGridLayout(dateGroup);
    dateLayout->setAlignment(Qt::AlignTop);
    dateLayout->setSpacing(6);
    dateLayout->setContentsMargins(11, 11, 11, 11);

    anyDateEnabled = new QRadioButton(dateGroup);
    anyDateEnabled->setText(i18n("Any date"));
    btnGroup->addButton(anyDateEnabled);
    anyDateEnabled->setChecked(true);

    modifiedBetweenEnabled = new QRadioButton(dateGroup);
    modifiedBetweenEnabled->setText(i18n("&Modified between"));
    btnGroup->addButton(modifiedBetweenEnabled);

    modifiedBetweenData1 = new KLineEdit(dateGroup);
    modifiedBetweenData1->setEnabled(false);
    modifiedBetweenData1->setText("");

    modifiedBetweenBtn1 = new QToolButton(dateGroup);
    modifiedBetweenBtn1->setEnabled(false);
    modifiedBetweenBtn1->setText("");
    modifiedBetweenBtn1->setIcon(QIcon(iconDate));

    QLabel *andLabel = new QLabel(dateGroup);
    andLabel->setText(i18n("an&d"));

    modifiedBetweenData2 = new KLineEdit(dateGroup);
    modifiedBetweenData2->setEnabled(false);
    modifiedBetweenData2->setText("");
    andLabel->setBuddy(modifiedBetweenData2);

    modifiedBetweenBtn2 = new QToolButton(dateGroup);
    modifiedBetweenBtn2->setEnabled(false);
    modifiedBetweenBtn2->setText("");
    modifiedBetweenBtn2->setIcon(QIcon(iconDate));

    notModifiedAfterEnabled = new QRadioButton(dateGroup);
    notModifiedAfterEnabled->setText(i18n("&Not modified after"));
    btnGroup->addButton(notModifiedAfterEnabled);

    notModifiedAfterData = new KLineEdit(dateGroup);
    notModifiedAfterData->setEnabled(false);
    notModifiedAfterData->setText("");

    notModifiedAfterBtn = new QToolButton(dateGroup);
    notModifiedAfterBtn->setEnabled(false);
    notModifiedAfterBtn->setText("");
    notModifiedAfterBtn->setIcon(QIcon(iconDate));

    modifiedInTheLastEnabled = new QRadioButton(dateGroup);
    modifiedInTheLastEnabled->setText(i18n("Mod&ified in the last"));
    btnGroup->addButton(modifiedInTheLastEnabled);

    modifiedInTheLastData = new QSpinBox(dateGroup);
    modifiedInTheLastData->setRange(0, 99999);
    modifiedInTheLastData->setEnabled(false);

    modifiedInTheLastType = new KComboBox(dateGroup);
    modifiedInTheLastType->addItem(i18n("days"));
    modifiedInTheLastType->addItem(i18n("weeks"));
    modifiedInTheLastType->addItem(i18n("months"));
    modifiedInTheLastType->addItem(i18n("years"));
    modifiedInTheLastType->setEnabled(false);

    notModifiedInTheLastData = new QSpinBox(dateGroup);
    notModifiedInTheLastData->setRange(0, 99999);
    notModifiedInTheLastData->setEnabled(false);

    QLabel *notModifiedInTheLastLbl = new QLabel(dateGroup);
    notModifiedInTheLastLbl->setText(i18n("No&t modified in the last"));
    notModifiedInTheLastLbl->setBuddy(notModifiedInTheLastData);

    notModifiedInTheLastType = new KComboBox(dateGroup);
    notModifiedInTheLastType->addItem(i18n("days"));
    notModifiedInTheLastType->addItem(i18n("weeks"));
    notModifiedInTheLastType->addItem(i18n("months"));
    notModifiedInTheLastType->addItem(i18n("years"));
    notModifiedInTheLastType->setEnabled(false);

    // Date options layout

    dateLayout->addWidget(anyDateEnabled, 0, 0);

    dateLayout->addWidget(modifiedBetweenEnabled, 1, 0);
    dateLayout->addWidget(modifiedBetweenData1, 1, 1);
    dateLayout->addWidget(modifiedBetweenBtn1, 1, 2);
    dateLayout->addWidget(andLabel, 1, 3);
    dateLayout->addWidget(modifiedBetweenData2, 1, 4);
    dateLayout->addWidget(modifiedBetweenBtn2, 1, 5);

    dateLayout->addWidget(notModifiedAfterEnabled, 2, 0);
    dateLayout->addWidget(notModifiedAfterData, 2, 1);
    dateLayout->addWidget(notModifiedAfterBtn, 2, 2);

    dateLayout->addWidget(modifiedInTheLastEnabled, 3, 0);
    QHBoxLayout *modifiedInTheLastLayout = new QHBoxLayout();
    modifiedInTheLastLayout->addWidget(modifiedInTheLastData);
    modifiedInTheLastLayout->addWidget(modifiedInTheLastType);
    dateLayout->addLayout(modifiedInTheLastLayout, 3, 1);

    dateLayout->addWidget(notModifiedInTheLastLbl, 4, 0);
    modifiedInTheLastLayout = new QHBoxLayout();
    modifiedInTheLastLayout->addWidget(notModifiedInTheLastData);
    modifiedInTheLastLayout->addWidget(notModifiedInTheLastType);
    dateLayout->addLayout(modifiedInTheLastLayout, 4, 1);

    filterLayout->addWidget(dateGroup, 1, 0, 1, 2);

    // Options for ownership

    QGroupBox *ownershipGroup = new QGroupBox(this);
    ownershipGroup->setTitle(i18n("Ownership"));

    QGridLayout *ownershipLayout = new QGridLayout(ownershipGroup);
    ownershipLayout->setAlignment(Qt::AlignTop);
    ownershipLayout->setSpacing(6);
    ownershipLayout->setContentsMargins(11, 11, 11, 11);

    belongsToUserEnabled = new QCheckBox(ownershipGroup);
    belongsToUserEnabled->setText(i18n("Belongs to &user"));
    ownershipLayout->addWidget(belongsToUserEnabled, 0, 0);

    belongsToUserData = new KComboBox(ownershipGroup);
    belongsToUserData->setEnabled(false);
    belongsToUserData->setEditable(false);
    ownershipLayout->addWidget(belongsToUserData, 0, 1);

    belongsToGroupEnabled = new QCheckBox(ownershipGroup);
    belongsToGroupEnabled->setText(i18n("Belongs to gr&oup"));
    ownershipLayout->addWidget(belongsToGroupEnabled, 1, 0);

    belongsToGroupData = new KComboBox(ownershipGroup);
    belongsToGroupData->setEnabled(false);
    belongsToGroupData->setEditable(false);
    ownershipLayout->addWidget(belongsToGroupData, 1, 1);

    ownershipLayout->setColumnStretch(0, 0);
    ownershipLayout->setColumnStretch(1, 1);

    filterLayout->addWidget(ownershipGroup, 2, 0);

    // Options for permissions

    permissionsGroup = new QGroupBox(this);
    permissionsGroup->setTitle(i18n("P&ermissions"));
    permissionsGroup->setCheckable(true);
    permissionsGroup->setChecked(false);

    QGridLayout *permissionsGridLayout = new QGridLayout();
    permissionsGridLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    permissionsGridLayout->addWidget(new QLabel(i18n("Owner")),  1, 1);
    permissionsGridLayout->addWidget(new QLabel(i18n("Group")),  2, 1);
    permissionsGridLayout->addWidget(new QLabel(i18n("Others")), 3, 1);

    QLabel *readPermLabel = new QLabel(i18n("r"));
    readPermLabel->setToolTip(i18n("Permission to read a file or list a directory's contents"));
    QLabel *writePermLabel = new QLabel(i18n("w"));
    writePermLabel->setToolTip(i18n("Permission to write to a file or directory"));
    QLabel *executePermLabel = new QLabel(i18n("x"));
    executePermLabel->setToolTip(i18n("Permission to execute a file or recurse a directory tree"));

    permissionsGridLayout->addWidget(readPermLabel,    0, 2);
    permissionsGridLayout->addWidget(writePermLabel,   0, 3);
    permissionsGridLayout->addWidget(executePermLabel, 0, 4);

    createPermissionsCheckbox(permissionsGroup, permissionsGridLayout, ownerR, 1, 2);
    createPermissionsCheckbox(permissionsGroup, permissionsGridLayout, ownerW, 1, 3);
    createPermissionsCheckbox(permissionsGroup, permissionsGridLayout, ownerX, 1, 4);

    createPermissionsCheckbox(permissionsGroup, permissionsGridLayout, groupR, 2, 2);
    createPermissionsCheckbox(permissionsGroup, permissionsGridLayout, groupW, 2, 3);
    createPermissionsCheckbox(permissionsGroup, permissionsGridLayout, groupX, 2, 4);

    createPermissionsCheckbox(permissionsGroup, permissionsGridLayout, otherR, 3, 2);
    createPermissionsCheckbox(permissionsGroup, permissionsGridLayout, otherW, 3, 3);
    createPermissionsCheckbox(permissionsGroup, permissionsGridLayout, otherX, 3, 4);

    QVBoxLayout *permissionsVLayout = new QVBoxLayout(permissionsGroup);
    permissionsVLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    permissionsVLayout->addLayout(permissionsGridLayout);
    permissionsVLayout->addSpacing(fontMetrics().height() / 2);
    permissionsVLayout->addWidget(new QLabel(i18n("<i>Partially checked permissions act as wildcard.</i>")));

    filterLayout->addWidget(permissionsGroup, 2, 1);

    // Set topmost layout stretch

    filterLayout->setRowStretch(0, 3);  // Name
    filterLayout->setRowStretch(1, 5);  // Date
    filterLayout->setRowStretch(2, 4);  // Ownership & Permissions

    filterLayout->setColumnStretch(0, 3);  // Ownership
    filterLayout->setColumnStretch(1, 2);  // Permissions

    // Connection table

    connect(minSizeEnabled, SIGNAL(toggled(bool)), minSizeAmount, SLOT(setEnabled(bool)));
    connect(minSizeEnabled, SIGNAL(toggled(bool)), minSizeType, SLOT(setEnabled(bool)));
    connect(maxSizeEnabled, SIGNAL(toggled(bool)), maxSizeAmount, SLOT(setEnabled(bool)));
    connect(maxSizeEnabled, SIGNAL(toggled(bool)), maxSizeType, SLOT(setEnabled(bool)));
    connect(modifiedBetweenEnabled, SIGNAL(toggled(bool)), modifiedBetweenData1, SLOT(setEnabled(bool)));
    connect(modifiedBetweenEnabled, SIGNAL(toggled(bool)), modifiedBetweenBtn1, SLOT(setEnabled(bool)));
    connect(modifiedBetweenEnabled, SIGNAL(toggled(bool)), modifiedBetweenData2, SLOT(setEnabled(bool)));
    connect(modifiedBetweenEnabled, SIGNAL(toggled(bool)), modifiedBetweenBtn2, SLOT(setEnabled(bool)));
    connect(notModifiedAfterEnabled, SIGNAL(toggled(bool)), notModifiedAfterData, SLOT(setEnabled(bool)));
    connect(notModifiedAfterEnabled, SIGNAL(toggled(bool)), notModifiedAfterBtn, SLOT(setEnabled(bool)));
    connect(modifiedInTheLastEnabled, SIGNAL(toggled(bool)), modifiedInTheLastData, SLOT(setEnabled(bool)));
    connect(modifiedInTheLastEnabled, SIGNAL(toggled(bool)), modifiedInTheLastType, SLOT(setEnabled(bool)));
    connect(modifiedInTheLastEnabled, SIGNAL(toggled(bool)), notModifiedInTheLastData, SLOT(setEnabled(bool)));
    connect(modifiedInTheLastEnabled, SIGNAL(toggled(bool)), notModifiedInTheLastType, SLOT(setEnabled(bool)));
    connect(belongsToUserEnabled, SIGNAL(toggled(bool)), belongsToUserData, SLOT(setEnabled(bool)));
    connect(belongsToGroupEnabled, SIGNAL(toggled(bool)), belongsToGroupData, SLOT(setEnabled(bool)));
    connect(modifiedBetweenBtn1, SIGNAL(clicked()), this, SLOT(modifiedBetweenSetDate1()));
    connect(modifiedBetweenBtn2, SIGNAL(clicked()), this, SLOT(modifiedBetweenSetDate2()));
    connect(notModifiedAfterBtn, SIGNAL(clicked()), this, SLOT(notModifiedAfterSetDate()));

    // fill the users and groups list

    fillList(belongsToUserData, USERSFILE);
    fillList(belongsToGroupData, GROUPSFILE);

    // tab order
    setTabOrder(minSizeEnabled, minSizeAmount);
    setTabOrder(minSizeAmount, maxSizeEnabled);
    setTabOrder(maxSizeEnabled, maxSizeAmount);
    setTabOrder(maxSizeAmount, modifiedBetweenEnabled);
    setTabOrder(modifiedBetweenEnabled, modifiedBetweenData1);
    setTabOrder(modifiedBetweenData1, modifiedBetweenData2);
    setTabOrder(modifiedBetweenData2, notModifiedAfterEnabled);
    setTabOrder(notModifiedAfterEnabled, notModifiedAfterData);
    setTabOrder(notModifiedAfterData, modifiedInTheLastEnabled);
    setTabOrder(modifiedInTheLastEnabled, modifiedInTheLastData);
    setTabOrder(modifiedInTheLastData, notModifiedInTheLastData);
    setTabOrder(notModifiedInTheLastData, belongsToUserEnabled);
    setTabOrder(belongsToUserEnabled, belongsToUserData);
    setTabOrder(belongsToUserData, belongsToGroupEnabled);
    setTabOrder(belongsToGroupEnabled, belongsToGroupData);
    setTabOrder(belongsToGroupData, permissionsGroup);
    setTabOrder(permissionsGroup, ownerR);
    setTabOrder(ownerR, ownerW);
    setTabOrder(ownerW, ownerX);
    setTabOrder(ownerX, groupR);
    setTabOrder(groupR, groupW);
    setTabOrder(groupW, groupX);
    setTabOrder(groupX, otherR);
    setTabOrder(otherR, otherW);
    setTabOrder(otherW, otherX);
    setTabOrder(otherX, minSizeType);
    setTabOrder(minSizeType, maxSizeType);
    setTabOrder(maxSizeType, modifiedInTheLastType);
    setTabOrder(modifiedInTheLastType, notModifiedInTheLastType);
}

void AdvancedFilter::modifiedBetweenSetDate1()
{
    changeDate(modifiedBetweenData1);
}

void AdvancedFilter::modifiedBetweenSetDate2()
{
    changeDate(modifiedBetweenData2);
}

void AdvancedFilter::notModifiedAfterSetDate()
{
    changeDate(notModifiedAfterData);
}

void AdvancedFilter::createPermissionsCheckbox(QWidget* parent, QGridLayout* layout, QCheckBox*& checkbox, int row, int col)
{
    checkbox = new QCheckBox(parent);
    checkbox->setTristate(true);
    checkbox->setCheckState(Qt::PartiallyChecked);
    layout->addWidget(checkbox, row, col);
}

void AdvancedFilter::changeDate(KLineEdit *p)
{
    // check if the current date is valid
    QDate d = KGlobal::locale()->readDate(p->text());
    if (!d.isValid()) d = QDate::currentDate();

    KRGetDate *gd = new KRGetDate(d, this);
    d = gd->getDate();
    // if a user pressed ESC or closed the dialog, we'll return an invalid date
    if (d.isValid())
        p->setText(KGlobal::locale()->formatDate(d, KLocale::ShortDate));
    delete gd;
}

void AdvancedFilter::fillList(KComboBox *list, QString filename)
{
    QFile data(filename);
    if (!data.open(QIODevice::ReadOnly)) {
        krOut << "Search: Unable to read " << filename << " !!!" << endl;
        return;
    }
    // and read it into the temporary array
    QTextStream t(&data);
    while (!t.atEnd()) {
        QString s = t.readLine();
        QString name = s.left(s.indexOf(':'));
        if (!name.startsWith('#'))
            list->addItem(name);
    }
}

void AdvancedFilter::invalidDateMessage(KLineEdit *p)
{
    // FIXME p->text() is empty sometimes (to reproduce, set date to "13.09.005")
    KMessageBox::detailedError(this, i18n("Invalid date entered."),
                               i18n("The date %1 is not valid according to your locale. Please re-enter a valid date (use the date button for easy access).", p->text()));
    p->setFocus();
}

bool AdvancedFilter::getSettings(FilterSettings &s)
{
    s.minSizeEnabled =  minSizeEnabled->isChecked();
    s.minSize.amount = minSizeAmount->value();
    s.minSize.unit = static_cast<FilterSettings::SizeUnit>(minSizeType->currentIndex());

    s.maxSizeEnabled = maxSizeEnabled->isChecked();
    s.maxSize.amount = maxSizeAmount->value();
    s.maxSize.unit = static_cast<FilterSettings::SizeUnit>(maxSizeType->currentIndex());

    if (s.minSizeEnabled && s.maxSizeEnabled && (s.maxSize.size() < s.minSize.size())) {
        KMessageBox::detailedError(this, i18n("Specified sizes are inconsistent."),
                            i18n("Please re-enter the values, so that the left side size "
                                 "will be smaller than (or equal to) the right side size."));
        minSizeAmount->setFocus();
        return false;
    }

    s.modifiedBetweenEnabled = modifiedBetweenEnabled->isChecked();
    s.modifiedBetween1 = KGlobal::locale()->readDate(modifiedBetweenData1->text());
    s.modifiedBetween2 = KGlobal::locale()->readDate(modifiedBetweenData2->text());

    if (s.modifiedBetweenEnabled) {
        // check if date is valid
        if (!s.modifiedBetween1.isValid()) {
            invalidDateMessage(modifiedBetweenData1);
            return false;
        } else if (!s.modifiedBetween2.isValid()) {
            invalidDateMessage(modifiedBetweenData2);
            return false;
        } else if (s.modifiedBetween1 > s.modifiedBetween2) {
            KMessageBox::detailedError(this, i18n("Dates are inconsistent."),
                                i18n("The date on the left is later than the date on the right. "
                                     "Please re-enter the dates, so that the left side date "
                                     "will be earlier than the right side date."));
            modifiedBetweenData1->setFocus();
            return false;
        }
    }

    s.notModifiedAfterEnabled = notModifiedAfterEnabled->isChecked();
    s.notModifiedAfter = KGlobal::locale()->readDate(notModifiedAfterData->text());

    if(s.notModifiedAfterEnabled && !s.notModifiedAfter.isValid()) {
        invalidDateMessage(notModifiedAfterData);
        return false;
    }

    s.modifiedInTheLastEnabled = modifiedInTheLastEnabled->isChecked();
    s.modifiedInTheLast.amount = modifiedInTheLastData->value();
    s.modifiedInTheLast.unit =
        static_cast<FilterSettings::TimeUnit>(modifiedInTheLastType->currentIndex());
    s.notModifiedInTheLast.amount = notModifiedInTheLastData->value();
    s.notModifiedInTheLast.unit =
        static_cast<FilterSettings::TimeUnit>(notModifiedInTheLastType->currentIndex());

    if (s.modifiedInTheLastEnabled  &&
            s.modifiedInTheLast.amount && s.notModifiedInTheLast.amount) {
        if (s.modifiedInTheLast.days() < s.notModifiedInTheLast.days()) {
            KMessageBox::detailedError(this, i18n("Dates are inconsistent."),
                                i18n("The date on top is later than the date on the bottom. "
                                     "Please re-enter the dates, so that the top date "
                                     "will be earlier than the bottom date."));
            modifiedInTheLastData->setFocus();
            return false;
        }
    }

    s.ownerEnabled = belongsToUserEnabled->isChecked();
    s.owner = belongsToUserData->currentText();

    s.groupEnabled = belongsToGroupEnabled->isChecked();
    s.group = belongsToGroupData->currentText();

    s.permissionsEnabled = permissionsGroup->isChecked();
    s.permissions = getPermissionsString(ownerR, "r") + getPermissionsString(ownerW, "w") + getPermissionsString(ownerX, "x") +
                    getPermissionsString(groupR, "r") + getPermissionsString(groupW, "w") + getPermissionsString(groupX, "x") +
                    getPermissionsString(otherR, "r") + getPermissionsString(otherW, "w") + getPermissionsString(otherX, "x");

    return true;
}

void AdvancedFilter::applySettings(const FilterSettings &s)
{
    minSizeEnabled->setChecked(s.minSizeEnabled);
    minSizeAmount->setValue(s.minSize.amount);
    minSizeType->setCurrentIndex(s.minSize.unit);

    maxSizeEnabled->setChecked(s.maxSizeEnabled);
    maxSizeAmount->setValue(s.maxSize.amount);
    maxSizeType->setCurrentIndex(s.maxSize.unit);

    if (s.modifiedBetweenEnabled)
        modifiedBetweenEnabled->setChecked(true);
    else if (s.notModifiedAfterEnabled)
        notModifiedAfterEnabled->setChecked(true);
    else if (s.modifiedInTheLastEnabled)
        modifiedInTheLastEnabled->setChecked(true);
    else
        anyDateEnabled->setChecked(true);

    modifiedBetweenData1->setText(
        KGlobal::locale()->formatDate(s.modifiedBetween1, KLocale::ShortDate));
    modifiedBetweenData2->setText(
        KGlobal::locale()->formatDate(s.modifiedBetween2, KLocale::ShortDate));

    notModifiedAfterData->setText(
        KGlobal::locale()->formatDate(s.notModifiedAfter, KLocale::ShortDate));

    modifiedInTheLastData->setValue(s.modifiedInTheLast.amount);
    modifiedInTheLastType->setCurrentIndex(s.modifiedInTheLast.unit);
    notModifiedInTheLastData->setValue(s.notModifiedInTheLast.amount);
    notModifiedInTheLastType->setCurrentIndex(s.notModifiedInTheLast.unit);

    belongsToUserEnabled->setChecked(s.ownerEnabled);
    setComboBoxValue(belongsToUserData, s.owner);

    belongsToGroupEnabled->setChecked(s.groupEnabled);
    setComboBoxValue(belongsToGroupData, s.group);

    permissionsGroup->setChecked(s.permissionsEnabled);
    QString perm = s.permissions;
    if (perm.length() != 9)
        perm = "?????????";
    setCheckBoxValue(ownerR, QString(perm[0]));
    setCheckBoxValue(ownerW, QString(perm[1]));
    setCheckBoxValue(ownerX, QString(perm[2]));
    setCheckBoxValue(groupR, QString(perm[3]));
    setCheckBoxValue(groupW, QString(perm[4]));
    setCheckBoxValue(groupX, QString(perm[5]));
    setCheckBoxValue(otherR, QString(perm[6]));
    setCheckBoxValue(otherW, QString(perm[7]));
    setCheckBoxValue(otherX, QString(perm[8]));
}

#include "advancedfilter.moc"
