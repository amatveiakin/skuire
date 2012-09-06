/***************************************************************************
                selectionmodeconfigtab.cpp  -  description
                             -------------------
    copyright            : (C) 2003 by Csaba Karai
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

                                                     S o u r c e    F i l e

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "selectionmodeconfigtab.h"

#include "krselectionmode.h"

#include "../defaults.h"
#include "../Konfigurator/konfiguratorpage.h"
#include "../GUI/krtreewidget.h"

#include <klocale.h>
#include <QGridLayout>
#include <QGroupBox>


SelectionModeConfigTab::SelectionModeConfigTab(QWidget* parent, KonfiguratorPage *page, int tabId) :
        QWidget(parent),
        mousePreview(0),
        mouseRadio(0),
        mouseCheckboxes(0)
{
    QGridLayout *mouseLayout = new QGridLayout(this);
    mouseLayout->setSpacing(6);
    mouseLayout->setContentsMargins(11, 11, 11, 11);

    // -------------- General -----------------
    QGroupBox *mouseGeneralGroup = page->createFrame(i18n("General"), this);
    QGridLayout *mouseGeneralGrid = page->createGridLayout(mouseGeneralGroup);
    mouseGeneralGrid->setSpacing(0);
    mouseGeneralGrid->setContentsMargins(5, 5, 5, 5);

    KONFIGURATOR_NAME_VALUE_TIP mouseSelection[] = {
        //     name           value          tooltip
        { i18n("Krusader Mode"), "0", i18n("Both keys allow selecting files. To select more than one file, hold the Ctrl key and click the left mouse button. Right-click menu is invoked using a short click on the right mouse button.") },
        { i18n("Konqueror Mode"), "1", i18n("Pressing the left mouse button selects files - you can click and select multiple files. Right-click menu is invoked using a short click on the right mouse button.") },
        { i18n("Total-Commander Mode"), "2", i18n("The left mouse button does not select, but sets the current file without affecting the current selection. The right mouse button selects multiple files and the right-click menu is invoked by pressing and holding the right mouse button.") },
        { i18n("Ergonomic Mode"), "4", i18n("The left mouse button does not select, but sets the current file without affecting the current selection. The right mouse button invokes the context-menu. You can select with Ctrl key and the left button.") },
        { i18n("Custom Selection Mode"), "3", i18n("Design your own selection mode.") }
    };
    mouseRadio = page->createRadioButtonGroup("Look&Feel", "Mouse Selection", "0", 1, 5, mouseSelection, 5, mouseGeneralGroup, true, tabId);
    mouseRadio->layout()->setContentsMargins(0, 0, 0, 0);
    mouseGeneralGrid->addWidget(mouseRadio, 0, 0);

    for (int i = 0; i != mouseRadio->count(); i++)
        connect(mouseRadio->find(i), SIGNAL(clicked()), SLOT(slotSelectionModeChanged()));

    mouseLayout->addWidget(mouseGeneralGroup, 0, 0);

    // -------------- Details -----------------
    QGroupBox *mouseDetailGroup = page->createFrame(i18n("Details"), this);
    QGridLayout *mouseDetailGrid = page->createGridLayout(mouseDetailGroup);
    mouseDetailGrid->setSpacing(0);
    mouseDetailGrid->setContentsMargins(5, 5, 5, 5);

    KONFIGURATOR_NAME_VALUE_TIP singleOrDoubleClick[] = {
        //          name            value            tooltip
        { i18n("Double-click selects (classic)"), "0", i18n("A single click on a file will select and focus, a double click opens the file or steps into the directory.") },
        { i18n("Obey KDE's global selection policy"), "1", i18n("<p>Use KDE's global setting:</p><p><i>KDE System Settings -> Input Devices -> Mouse</i></p>") }
    };
    KonfiguratorRadioButtons *clickRadio = page->createRadioButtonGroup("Look&Feel", "Single Click Selects", "0", 1, 0, singleOrDoubleClick, 2, mouseDetailGroup, true, tabId);
    clickRadio->layout()->setContentsMargins(0, 0, 0, 0);
    mouseDetailGrid->addWidget(clickRadio, 0, 0);

    KONFIGURATOR_CHECKBOX_PARAM mouseCheckboxesParam[] = {
        // {cfg_class,   cfg_name,   default
        //  text,  restart,
        //  tooltip }
        {"Custom Selection Mode",  "QT Selection",  _QtSelection,
            i18n("Based on KDE's selection mode"), true,
            i18n("If checked, use a mode based on KDE's style.") },
        {"Custom Selection Mode",  "Left Selects",  _LeftSelects,
         i18n("Left mouse button selects"), true,
         i18n("If checked, left clicking an item will select it.") },
        {"Custom Selection Mode",  "Left Preserves", _LeftPreserves,
         i18n("Left mouse button preserves selection"), true,
         i18n("If checked, left clicking an item will select it, but will not unselect other, already selected items.") },
        {"Custom Selection Mode",  "ShiftCtrl Left Selects",  _ShiftCtrlLeft,
         i18n("Shift/Ctrl-Left mouse button selects"), true,
         i18n("If checked, Shift/Ctrl left clicking will select items.\nNote: this is meaningless if 'Left Button Selects' is checked.") },
        {"Custom Selection Mode",  "Right Selects",  _RightSelects,
         i18n("Right mouse button selects"), true,
         i18n("If checked, right clicking an item will select it.") },
        {"Custom Selection Mode",  "Right Preserves",  _RightPreserves,
         i18n("Right mouse button preserves selection"), true,
         i18n("If checked, right clicking an item will select it, but will not unselect other, already selected items.") },
        {"Custom Selection Mode",  "ShiftCtrl Right Selects",  _ShiftCtrlRight,
         i18n("Shift/Ctrl-Right mouse button selects"), true,
         i18n("If checked, Shift/Ctrl right clicking will select items.\nNote: this is meaningless if 'Right Button Selects' is checked.") },
        {"Custom Selection Mode",  "Space Moves Down",  _SpaceMovesDown,
         i18n("Spacebar moves down"), true,
         i18n("If checked, pressing the spacebar will select the current item and move down.\nOtherwise, current item is selected, but remains the current item.") },
        {"Custom Selection Mode",  "Space Calc Space",  _SpaceCalcSpace,
         i18n("Spacebar calculates disk space"), true,
         i18n("If checked, pressing the spacebar while the current item is a directory, will (except from selecting the directory)\ncalculate space occupied of the directory (recursively).") },
        {"Custom Selection Mode",  "Insert Moves Down",  _InsertMovesDown,
         i18n("Insert moves down"), true,
         i18n("If checked, pressing Insert will select the current item, and move down to the next item.\nOtherwise, current item is not changed.") },
        {"Custom Selection Mode",  "Immediate Context Menu",  _ImmediateContextMenu,
         i18n("Right clicking pops context menu immediately"), true,
         i18n("If checked, right clicking will result in an immediate showing of the context menu.\nOtherwise, user needs to click and hold the right mouse button for 500ms.") },
    };


    mouseCheckboxes = page->createCheckBoxGroup(1, 0, mouseCheckboxesParam, 11 /*count*/, mouseDetailGroup, tabId);
    mouseDetailGrid->addWidget(mouseCheckboxes, 1, 0);

    for (int i = 0; i < mouseCheckboxes->count(); i++)
        connect(mouseCheckboxes->find(i), SIGNAL(clicked()), SLOT(slotMouseCheckBoxChanged()));

    mouseLayout->addWidget(mouseDetailGroup, 0, 1, 2, 1);

    // Disable the details-button if not in custom-mode
    slotSelectionModeChanged();

    // -------------- Preview -----------------
    QGroupBox *mousePreviewGroup = page->createFrame(i18n("Preview"), this);
    QGridLayout *mousePreviewGrid = page->createGridLayout(mousePreviewGroup);
    // TODO preview
    mousePreview = new KrTreeWidget(mousePreviewGroup);
    mousePreviewGrid->addWidget(mousePreview, 0 , 0);
    mousePreviewGroup->setEnabled(false); // TODO re-enable once the preview is implemented
    // ------------------------------------------
    mouseLayout->addWidget(mousePreviewGroup, 1, 0);
}

void SelectionModeConfigTab::slotSelectionModeChanged()
{
    KrSelectionMode *selectionMode =
        KrSelectionMode::getSelectionHandlerForMode(mouseRadio->selectedValue());
    if (selectionMode == NULL)   //User mode
        return;
    selectionMode->init();
    mouseCheckboxes->find("QT Selection")->setChecked(selectionMode->useQTSelection());
    mouseCheckboxes->find("Left Selects")->setChecked(selectionMode->leftButtonSelects());
    mouseCheckboxes->find("Left Preserves")->setChecked(selectionMode->leftButtonPreservesSelection());
    mouseCheckboxes->find("ShiftCtrl Left Selects")->setChecked(selectionMode->shiftCtrlLeftButtonSelects());
    mouseCheckboxes->find("Right Selects")->setChecked(selectionMode->rightButtonSelects());
    mouseCheckboxes->find("Right Preserves")->setChecked(selectionMode->rightButtonPreservesSelection());
    mouseCheckboxes->find("ShiftCtrl Right Selects")->setChecked(selectionMode->shiftCtrlRightButtonSelects());
    mouseCheckboxes->find("Space Moves Down")->setChecked(selectionMode->spaceMovesDown());
    mouseCheckboxes->find("Space Calc Space")->setChecked(selectionMode->spaceCalculatesDiskSpace());
    mouseCheckboxes->find("Insert Moves Down")->setChecked(selectionMode->insertMovesDown());
    mouseCheckboxes->find("Immediate Context Menu")->setChecked(selectionMode->showContextMenu() == -1);
}

void SelectionModeConfigTab::slotMouseCheckBoxChanged()
{
    mouseRadio->selectButton("3");    //custom selection mode
}
