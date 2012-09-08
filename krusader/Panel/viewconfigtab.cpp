/***************************************************************************
                    viewconfigtab.cpp  -  description
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

#include "viewconfigtab.h"

#include "krview.h"

#include "viewtype.h"
#include "viewfactory.h"
#include "../defaults.h"
#include "../Konfigurator/konfiguratorpage.h"

#include <klocale.h>

#include <QGridLayout>
#include <QGroupBox>


ViewConfigTab::ViewConfigTab(QWidget* parent, KonfiguratorPage *page, int tabId) :
//         ConfigTab(parent, page, tabId)
        QWidget(parent)
{
//     QScrollArea *scrollArea = new QScrollArea(tabWidget);
//     QWidget *this = new QWidget(scrollArea);
//     scrollArea->setFrameStyle(QFrame::NoFrame);
//     scrollArea->setWidget(this);
//     scrollArea->setWidgetResizable(true);
//     tabWidget->addTab(scrollArea, i18n("View"));

    QGridLayout *panelLayout = new QGridLayout(this);
    panelLayout->setSpacing(6);
    panelLayout->setContentsMargins(11, 11, 11, 11);
    QGroupBox *panelGrp = page->createFrame(i18n("General"), this);
    panelLayout->addWidget(panelGrp, 0, 0);
    QGridLayout *panelGrid = page->createGridLayout(panelGrp);

    // ----------------------------------------------------------------------------------
    //  ---------------------------- General settings -----------------------------------
    // ----------------------------------------------------------------------------------

    // -------------------- Panel Font ----------------------------------
    QHBoxLayout *hbox = new QHBoxLayout();

    hbox->addWidget(new QLabel(i18n("View font:"), panelGrp));

    KonfiguratorFontChooser * chsr = page->createFontChooser("Look&Feel", "Filelist Font", _FilelistFont, panelGrp, true, tabId);
    hbox->addWidget(chsr);

    hbox->addWidget(page->createSpacer(panelGrp));

    panelGrid->addLayout(hbox, 1, 0);


    // -------------------- Misc options ----------------------------------
    KONFIGURATOR_CHECKBOX_PARAM panelSettings[] =
        //   cfg_class  cfg_name                default text                                  restart tooltip
    {
        {"Look&Feel", "Human Readable Size",            _HumanReadableSize,      i18n("Use human-readable file size"), true ,  i18n("File sizes are displayed in B, KB, MB and GB, not just in bytes.") },
        {"Look&Feel", "Show Hidden",                    _ShowHidden,             i18n("Show hidden files"),      false,  i18n("Display files beginning with a dot.") },
        {"Look&Feel", "Numeric permissions",            _NumericPermissions,     i18n("Numeric Permissions"), true,  i18n("Show octal numbers (0755) instead of the standard permissions (rwxr-xr-x) in the permission column.") },
        {"Look&Feel", "Load User Defined Folder Icons", _UserDefinedFolderIcons, i18n("Load the user defined folder icons"), true ,  i18n("Load the user defined directory icons (can cause decrease in performance).") },
    };

    KonfiguratorCheckBoxGroup *panelSett = page->createCheckBoxGroup(2, 0, panelSettings, 4 /*count*/, panelGrp, tabId);

    panelGrid->addWidget(panelSett, 3, 0, 1, 2);

    // =========================================================
    panelGrid->addWidget(page->createLine(panelGrp), 4, 0);

    // ------------------------ Sort Method ----------------------------------

    hbox = new QHBoxLayout();

    hbox->addWidget(new QLabel(i18n("Sort method:"), panelGrp));

    KONFIGURATOR_NAME_VALUE_PAIR sortMethods[] = {{ i18n("Alphabetical"), QString::number(KrViewProperties::Alphabetical) },
        { i18n("Alphabetical and numbers"),    QString::number(KrViewProperties::AlphabeticalNumbers) },
        { i18n("Character code"),              QString::number(KrViewProperties::CharacterCode) },
        { i18n("Character code and numbers"),  QString::number(KrViewProperties::CharacterCodeNumbers) },
        { i18nc("Krusader sort", "Krusader"),  QString::number(KrViewProperties::Krusader) }
    };
    KonfiguratorComboBox *cmb = page->createComboBox("Look&Feel", "Sort method", QString::number(_DefaultSortMethod),
                            sortMethods, 5, panelGrp, true, false, tabId);
    hbox->addWidget(cmb);
    hbox->addWidget(page->createSpacer(panelGrp));

    panelGrid->addLayout(hbox, 5, 0);

    // ------------------------ Sort Options ----------------------------------
    KONFIGURATOR_CHECKBOX_PARAM sortSettings[] =
        // cfg_class, cfg_name, default, text, restart, tooltip
    {
        {"Look&Feel", "Case Sensative Sort", _CaseSensativeSort, i18n("Case sensitive sorting"), true,
            i18n("All files beginning with capital letters appear before files beginning with non-capital letters (UNIX default).") },
        {"Look&Feel", "Show Directories First", true, i18n("Show directories first"), true, 0 },
        {"Look&Feel", "Always sort dirs by name", false, i18n("Always sort dirs by name"), true,
            i18n("Directories are sorted by name, regardless of the sort column.") },
        {"Look&Feel", "Locale Aware Sort", true, i18n("Locale aware sorting"), true,
            i18n("The sorting is performed in a locale- and also platform-dependent manner. Can be slow.") },
    };

    KonfiguratorCheckBoxGroup *sortSett = page->createCheckBoxGroup(2, 0, sortSettings,
                                          4 /*count*/, panelGrp, tabId);
    sortSett->find("Show Directories First")->addDep(sortSett->find("Always sort dirs by name"));

    panelGrid->addWidget(sortSett, 6, 0, 1, 2);


    // ----------------------------------------------------------------------------------
    //  ---------------------------- View modes -----------------------------------------
    // ----------------------------------------------------------------------------------

    panelGrp = page->createFrame(i18n("View modes"), this);
    panelLayout->addWidget(panelGrp, 1, 0);
    panelGrid = page->createGridLayout(panelGrp);

    // -------------------- Default Panel Type ----------------------------------
    hbox = new QHBoxLayout();

    hbox->addWidget(new QLabel(i18n("Default view mode:"), panelGrp));

    ViewFactory *factory = ViewFactory::self();

    QList<ViewType *> views = factory->registeredViews();
    const int viewsSize = views.size();
    KONFIGURATOR_NAME_VALUE_PAIR *panelTypes = new KONFIGURATOR_NAME_VALUE_PAIR[ viewsSize ];

    QString defType = "0";

    for (int i = 0; i != viewsSize; i++) {
        ViewType * inst = views[ i ];
        panelTypes[ i ].text = inst->description();
        panelTypes[ i ].text.remove('&');
        panelTypes[ i ].value = QString("%1").arg(inst->id());
        if (inst->id() == factory->defaultViewId())
            defType = QString("%1").arg(inst->id());
    }

    cmb = page->createComboBox("Look&Feel", "Default Panel Type", defType, panelTypes, viewsSize, panelGrp, false, false, tabId);
    hbox->addWidget(cmb);
    hbox->addWidget(page->createSpacer(panelGrp));

    delete [] panelTypes;

    panelGrid->addLayout(hbox, 0, 0);

    // ----- Individual Settings Per View Type ------------------------
    QTabWidget *tabs_view = new QTabWidget(panelGrp);
    panelGrid->addWidget(tabs_view, 11, 0);

    for(int i = 0; i < views.count(); i++) {
        QWidget *tab = new QWidget(tabs_view);
        tabs_view->addTab(tab, views[i]->description());
        setupView(views[i], tab, page, tabId);
    }

}

void ViewConfigTab::setupView(ViewType *instance, QWidget *parent, KonfiguratorPage *page, int tabId)
{
    QGridLayout *grid = page->createGridLayout(parent);

// -------------------- Filelist icon size ----------------------------------
    QHBoxLayout *hbox = new QHBoxLayout();

    hbox->addWidget(new QLabel(i18n("Default icon size:"), parent));

    KONFIGURATOR_NAME_VALUE_PAIR *iconSizes = new KONFIGURATOR_NAME_VALUE_PAIR[KrView::iconSizes.count()];
    for(int i = 0; i < KrView::iconSizes.count(); i++)
        iconSizes[i].text =  iconSizes[i].value = QString::number(KrView::iconSizes[i]);
    KonfiguratorComboBox *cmb = page->createComboBox(instance->name(), "IconSize", _FilelistIconSize, iconSizes, KrView::iconSizes.count(), parent, true, true, tabId);
    delete [] iconSizes;
    cmb->lineEdit()->setValidator(new QRegExpValidator(QRegExp("[1-9]\\d{0,1}"), cmb));
    hbox->addWidget(cmb);

    hbox->addWidget(page->createSpacer(parent));

    grid->addLayout(hbox, 1, 0);

//--------------------------------------------------------------------
    KONFIGURATOR_CHECKBOX_PARAM iconSettings[] =
        //   cfg_class             cfg_name        default       text            restart        tooltip
    {
        {instance->name(), "With Icons",      _WithIcons,   i18n("Use icons in the filenames"), true,  i18n("Show the icons for filenames and directories.") },
        {instance->name(), "ShowPreviews", false, i18n("Show previews by default"), false, i18n("Show previews of files and directories.") },
    };

    KonfiguratorCheckBoxGroup *iconSett = page->createCheckBoxGroup(2, 0, iconSettings, 2 /*count*/, parent, tabId);

    grid->addWidget(iconSett, 2, 0, 1, 2);

}
