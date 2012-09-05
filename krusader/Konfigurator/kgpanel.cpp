/***************************************************************************
                        kgpanel.cpp  -  description
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

#include "kgpanel.h"
#include "../defaults.h"
#include "../Dialogs/krdialogs.h"
#include <QtGui/QTabWidget>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <klocale.h>
#include <QtGui/QValidator>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#include "viewtype.h"
#include "viewfactory.h"
#include "module.h"
#include "viewconfigui.h"
#include "../krusaderapp.h"
#include "../Panel/krlayoutfactory.h"

enum {
    PAGE_MISC = 0,
    PAGE_VIEW,
    PAGE_PANELTOOLBAR,
    PAGE_MOUSE,
    PAGE_LAYOUT
};


KgPanel::KgPanel(bool first, QWidget* parent) :
        KonfiguratorPage(first, parent)
{
    tabWidget = new QTabWidget(this);
    setWidget(tabWidget);
    setWidgetResizable(true);

    setupMiscTab();
    setupPanelTab();
    setupPanelToolbarTab();
    setupMouseModeTab();
    setupLayoutTab();
}

ViewConfigUI *KgPanel::viewConfigUI()
{
    ViewConfigUI *viewCfg = qobject_cast<ViewConfigUI*>(KrusaderApp::self()->module("View"));
    Q_ASSERT(viewCfg);
    return viewCfg;
}

// ---------------------------------------------------------------------------------------
//  ---------------------------- Misc TAB ------------------------------------------------
// ---------------------------------------------------------------------------------------
void KgPanel::setupMiscTab()
{
    QScrollArea *scrollArea = new QScrollArea(tabWidget);
    QWidget *tab = new QWidget(scrollArea);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setWidget(tab);
    scrollArea->setWidgetResizable(true);
    tabWidget->addTab(scrollArea, i18n("General"));

    QVBoxLayout *miscLayout = new QVBoxLayout(tab);
    miscLayout->setSpacing(6);
    miscLayout->setContentsMargins(11, 11, 11, 11);

// ---------------------------------------------------------------------------------------
// ------------------------------- Address Bar -------------------------------------------
// ---------------------------------------------------------------------------------------
    QGroupBox *miscGrp = createFrame(i18n("Address bar"), tab);
    QGridLayout *miscGrid = createGridLayout(miscGrp);

    KONFIGURATOR_CHECKBOX_PARAM general_settings[] = { // cfg_class, cfg_name, default, text, restart, tooltip
        {"Look&Feel", "FlatOriginBar", _FlatOriginBar, i18n("Flat address bar"),   true,  0 },
        {"Look&Feel", "ShortenHomePath", _ShortenHomePath, i18n("Shorten home path"),   true,  i18n("Display home path as \"~\"") },
    };
    KonfiguratorCheckBoxGroup *cbs = createCheckBoxGroup(2, 0, general_settings, 2 /*count*/, miscGrp, PAGE_MISC);
    cbs->find("FlatOriginBar")->addDep(cbs->find("ShortenHomePath"));
    miscGrid->addWidget(cbs, 0, 0);

    miscLayout->addWidget(miscGrp);


// ---------------------------------------------------------------------------------------
// ------------------------------- Operation ---------------------------------------------
// ---------------------------------------------------------------------------------------
    miscGrp = createFrame(i18n("Operation"), tab);
    miscGrid = createGridLayout(miscGrp);

    KONFIGURATOR_CHECKBOX_PARAM operation_settings[] = { // cfg_class, cfg_name, default, text, restart, tooltip
        {"Look&Feel", "Mark Dirs",            _MarkDirs,          i18n("Autoselect directories"),   false,  i18n("When matching the select criteria, not only files will be selected, but also directories.") },
        {"Look&Feel", "Rename Selects Extension", true,          i18n("Rename selects extension"),   false,  i18n("When renaming a file, the whole text is selected. If you want Total-Commander like renaming of just the name, without extension, uncheck this option.") },
        {"Look&Feel", "UnselectBeforeOperation", _UnselectBeforeOperation, i18n("Unselect files before copy/move"), false, i18n("Unselect files, which are to be copied/moved, before the operation starts.") },
        {"Look&Feel", "FilterDialogRemembersSettings", _FilterDialogRemembersSettings, i18n("Filter dialog remembers settings"), false, i18n("The filter dialog is opened with the last filter settings that where applied to the panel.") },
    };
    cbs = createCheckBoxGroup(2, 0, operation_settings, 4 /*count*/, miscGrp, PAGE_MISC);
    miscGrid->addWidget(cbs, 0, 0);

    miscLayout->addWidget(miscGrp);

// ---------------------------------------------------------------------------------------
// ------------------------------ Tabs ---------------------------------------------------
// ---------------------------------------------------------------------------------------
    miscGrp = createFrame(i18n("Tabs"), tab);
    miscGrid = createGridLayout(miscGrp);

    KONFIGURATOR_CHECKBOX_PARAM tabbar_settings[] = { //   cfg_class  cfg_name                default             text                              restart tooltip
        {"Look&Feel", "Fullpath Tab Names",   _FullPathTabNames,  i18n("Use full path tab names"), true ,  i18n("Display the full path in the folder tabs. By default only the last part of the path is displayed.") },
        {"Look&Feel", "Show Tab Buttons",   true,  i18n("Show new/close tab buttons"), true ,  i18n("Show the new/close tab buttons") },
    };
    cbs = createCheckBoxGroup(2, 0, tabbar_settings, 2 /*count*/, miscGrp, PAGE_MISC);
    miscGrid->addWidget(cbs, 0, 0);

// -----------------  Tab Bar position ----------------------------------
    QHBoxLayout *hbox = new QHBoxLayout();

    hbox->addWidget(new QLabel(i18n("Tab Bar position:"), miscGrp));

    KONFIGURATOR_NAME_VALUE_PAIR positions[] = {{ i18n("Top"),                "top" },
        { i18n("Bottom"),                    "bottom" }
    };

    KonfiguratorComboBox *cmb = createComboBox("Look&Feel", "Tab Bar Position",
                                "bottom", positions, 2, miscGrp, true, false, PAGE_MISC);

    hbox->addWidget(cmb);
    hbox->addWidget(createSpacer(miscGrp));

    miscGrid->addLayout(hbox, 1, 0);

    miscLayout->addWidget(miscGrp);

// ---------------------------------------------------------------------------------------
// -----------------------------  Quicksearch  -------------------------------------------
// ---------------------------------------------------------------------------------------
    miscGrp = createFrame(i18n("Quicksearch/Quickfilter"), tab);
    miscGrid = createGridLayout(miscGrp);

    KONFIGURATOR_CHECKBOX_PARAM quicksearch[] = { //   cfg_class  cfg_name                default             text                              restart tooltip
        {"Look&Feel", "New Style Quicksearch",  _NewStyleQuicksearch, i18n("New style Quicksearch"), false,  i18n("Opens a quick search dialog box.") },
        {"Look&Feel", "Case Sensitive Quicksearch",  _CaseSensitiveQuicksearch, i18n("Case sensitive Quicksearch/QuickFilter"), false,  i18n("All files beginning with capital letters appear before files beginning with non-capital letters (UNIX default).") },
        {"Look&Feel", "Up/Down Cancels Quicksearch",  false, i18n("Up/Down cancels Quicksearch"), false,  i18n("Pressing the Up/Down buttons cancels Quicksearch.") },
    };

    quicksearchCheckboxes = createCheckBoxGroup(2, 0, quicksearch, 3 /*count*/, miscGrp, PAGE_MISC);
    miscGrid->addWidget(quicksearchCheckboxes, 0, 0);
    connect(quicksearchCheckboxes->find("New Style Quicksearch"), SIGNAL(stateChanged(int)), this, SLOT(slotDisable()));
    slotDisable();

    // -------------- Quicksearch position -----------------------

    hbox = new QHBoxLayout();

    hbox->addWidget(new QLabel(i18n("Position:"), miscGrp));

    cmb = createComboBox("Look&Feel", "Quicksearch Position",
                            "bottom", positions, 2, miscGrp, true, false, PAGE_MISC);
    hbox->addWidget(cmb);

    hbox->addWidget(createSpacer(miscGrp));

    miscGrid->addLayout(hbox, 1, 0);


    miscLayout->addWidget(miscGrp);

// --------------------------------------------------------------------------------------------
// ------------------------------- Status/Totalsbar settings ----------------------------------
// --------------------------------------------------------------------------------------------
    miscGrp = createFrame(i18n("Status/Totalsbar"), tab);
    miscGrid = createGridLayout(miscGrp);

    KONFIGURATOR_CHECKBOX_PARAM barSettings[] =
    {
        {"Look&Feel", "Show Size In Bytes", true, i18n("Show size in bytes too"), true,  i18n("Show size in bytes too") },
        {"Look&Feel", "ShowSpaceInformation", true, i18n("Show space information"), true,  i18n("Show free/total space on the device") },
    };
    KonfiguratorCheckBoxGroup *barSett = createCheckBoxGroup(2, 0, barSettings,
                                          2 /*count*/, miscGrp, PAGE_MISC);
    miscGrid->addWidget(barSett, 1, 0, 1, 2);

    miscLayout->addWidget(miscGrp);
}

// --------------------------------------------------------------------------------------------
// ------------------------------------ Layout Tab --------------------------------------------
// --------------------------------------------------------------------------------------------
void KgPanel::setupLayoutTab()
{
    QScrollArea *scrollArea = new QScrollArea(tabWidget);
    QWidget *tab = new QWidget(scrollArea);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setWidget(tab);
    scrollArea->setWidgetResizable(true);
    tabWidget->addTab(scrollArea, i18n("Layout"));

    QGridLayout *grid = createGridLayout(tab);

    QStringList layoutNames = KrLayoutFactory::layoutNames();
    int numLayouts = layoutNames.count();

    grid->addWidget(createSpacer(tab), 0, 2);

    QLabel *l = new QLabel(i18n("Layout:"), tab);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(l, 0, 0);
    KONFIGURATOR_NAME_VALUE_PAIR *layouts = new KONFIGURATOR_NAME_VALUE_PAIR[numLayouts];
    for (int i = 0; i != numLayouts; i++) {
        layouts[ i ].text = KrLayoutFactory::layoutDescription(layoutNames[i]);
        layouts[ i ].value = layoutNames[i];
    }
    KonfiguratorComboBox *cmb = createComboBox("PanelLayout", "Layout", "default",
                         layouts, numLayouts, tab, true, false, PAGE_LAYOUT);
    grid->addWidget(cmb, 0, 1);
    delete [] layouts;

    l = new QLabel(i18n("Frame Color:"), tab);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(l, 1, 0);
    KONFIGURATOR_NAME_VALUE_PAIR frameColor[] = {
        { i18nc("Frame color", "Defined by Layout"), "default" },
        { i18nc("Frame color", "None"), "none" },
        { i18nc("Frame color", "Statusbar"), "Statusbar" }
    };
    cmb = createComboBox("PanelLayout", "FrameColor",
                            "default", frameColor, 3, tab, true, false, PAGE_LAYOUT);
    grid->addWidget(cmb, 1, 1);


    l = new QLabel(i18n("Frame Shape:"), tab);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(l, 2, 0);
    KONFIGURATOR_NAME_VALUE_PAIR frameShape[] = {
        { i18nc("Frame shape", "Defined by Layout"), "default" },
        { i18nc("Frame shape", "None"), "NoFrame" },
        { i18nc("Frame shape", "Box"), "Box" },
        { i18nc("Frame shape", "Panel"), "Panel" },
    };
    cmb = createComboBox("PanelLayout", "FrameShape",
                            "default", frameShape, 4, tab, true, false, PAGE_LAYOUT);
    grid->addWidget(cmb, 2, 1);


    l = new QLabel(i18n("Frame Shadow:"), tab);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(l, 3, 0);
    KONFIGURATOR_NAME_VALUE_PAIR frameShadow[] = {
        { i18nc("Frame shadow", "Defined by Layout"), "default" },
        { i18nc("Frame shadow", "None"), "Plain" },
        { i18nc("Frame shadow", "Raised"), "Raised" },
        { i18nc("Frame shadow", "Sunken"), "Sunken" },
    };
    cmb = createComboBox("PanelLayout", "FrameShadow",
                            "default", frameShadow, 4, tab, true, false, PAGE_LAYOUT);
    grid->addWidget(cmb, 3, 1);
}


// ----------------------------------------------------------------------------------
//  ---------------------------- VIEW TAB -------------------------------------------
// ----------------------------------------------------------------------------------
void KgPanel::setupPanelTab()
{
    QScrollArea *scrollArea = new QScrollArea(tabWidget);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);

    QWidget *tab_panel = viewConfigUI()->createViewCfgTab(scrollArea, this, PAGE_VIEW);
    scrollArea->setWidget(tab_panel);

    tabWidget->addTab(scrollArea, i18n("View"));
}

// -----------------------------------------------------------------------------------
//  -------------------------- Panel Toolbar TAB ----------------------------------
// -----------------------------------------------------------------------------------
void KgPanel::setupPanelToolbarTab()
{
    QScrollArea *scrollArea = new QScrollArea(tabWidget);
    QWidget *toolbarTab = new QWidget(scrollArea);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setWidget(toolbarTab);
    scrollArea->setWidgetResizable(true);
    tabWidget->addTab(scrollArea, i18n("Buttons"));

    QBoxLayout * panelToolbarVLayout = new QVBoxLayout(toolbarTab);
    panelToolbarVLayout->setSpacing(6);
    panelToolbarVLayout->setContentsMargins(11, 11, 11, 11);

    KONFIGURATOR_CHECKBOX_PARAM panelToolbarActiveCheckbox[] =
        //   cfg_class    cfg_name                default        text                          restart tooltip
    {
        {"ListPanelButtons", "Icons", false, i18n("Toolbar buttons have icons"), true,   "" },
        {"Look&Feel",  "Media Button Visible",  true,    i18n("Show Media Button"), true ,  i18n("The media button will be visible.") },
        {"Look&Feel",  "Back Button Visible",  false,      i18n("Show Back Button"),     true ,  "Goes back in history." },
        {"Look&Feel",  "Forward Button Visible",  false,   i18n("Show Forward Button"),     true ,  "Goes forward in history." },
        {"Look&Feel",  "History Button Visible",  true,    i18n("Show History Button"), true ,  i18n("The history button will be visible.") },
        {"Look&Feel",  "Bookmarks Button Visible",  true,    i18n("Show Bookmarks Button"), true ,  i18n("The bookmarks button will be visible.") },
        {"Look&Feel", "Panel Toolbar visible", _PanelToolBar, i18n("Show Panel Toolbar"), true,   i18n("The panel toolbar will be visible.") },
    };

    panelToolbarActive = createCheckBoxGroup(1, 0, panelToolbarActiveCheckbox, 7/*count*/, toolbarTab, PAGE_PANELTOOLBAR);
    connect(panelToolbarActive->find("Panel Toolbar visible"), SIGNAL(stateChanged(int)), this, SLOT(slotEnablePanelToolbar()));

    QGroupBox * panelToolbarGrp = createFrame(i18n("Visible Panel Toolbar buttons"), toolbarTab);
    QGridLayout * panelToolbarGrid = createGridLayout(panelToolbarGrp);

    KONFIGURATOR_CHECKBOX_PARAM panelToolbarCheckboxes[] = {
        //   cfg_class    cfg_name                default             text                       restart tooltip
        {"Look&Feel",  "Open Button Visible",  _Open,      i18n("Open button"),     true ,  i18n("Opens the directory browser.") },
        {"Look&Feel",  "Equal Button Visible", _cdOther,   i18n("Equal button (=)"), true ,  i18n("Changes the panel directory to the other panel directory.") },
        {"Look&Feel",  "Up Button Visible",    _cdUp,      i18n("Up button (..)"),  true ,  i18n("Changes the panel directory to the parent directory.") },
        {"Look&Feel",  "Home Button Visible",  _cdHome,    i18n("Home button (~)"), true ,  i18n("Changes the panel directory to the home directory.") },
        {"Look&Feel",  "Root Button Visible",  _cdRoot,    i18n("Root button (/)"), true ,  i18n("Changes the panel directory to the root directory.") },
        {"Look&Feel",  "SyncBrowse Button Visible",  _syncBrowseButton,    i18n("Toggle-button for sync-browsing"), true ,  i18n("Each directory change in the panel is also performed in the other panel.") },
    };


    pnlcbs = createCheckBoxGroup(1, 0, panelToolbarCheckboxes,
                                 sizeof(panelToolbarCheckboxes) / sizeof(*panelToolbarCheckboxes),
                                 panelToolbarGrp, PAGE_PANELTOOLBAR);

    panelToolbarVLayout->addWidget(panelToolbarActive, 0, 0);
    panelToolbarGrid->addWidget(pnlcbs, 0, 0);
    panelToolbarVLayout->addWidget(panelToolbarGrp,    1, 0);

    // Enable panel toolbar checkboxes
    slotEnablePanelToolbar();
}

// ---------------------------------------------------------------------------
//  -------------------------- Mouse TAB ----------------------------------
// ---------------------------------------------------------------------------
void KgPanel::setupMouseModeTab()
{
    QScrollArea *scrollArea = new QScrollArea(tabWidget);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);

    QWidget *tab_mouse = viewConfigUI()->createSelectionModeCfgTab(scrollArea, this, PAGE_MOUSE);
    scrollArea->setWidget(tab_mouse);

    tabWidget->addTab(scrollArea, i18n("Selection Mode"));
}

void KgPanel::slotDisable()
{
    bool isNewStyleQuickSearch = quicksearchCheckboxes->find("New Style Quicksearch")->isChecked();
    quicksearchCheckboxes->find("Case Sensitive Quicksearch")->setEnabled(isNewStyleQuickSearch);
}

void KgPanel::slotEnablePanelToolbar()
{
    bool enableTB = panelToolbarActive->find("Panel Toolbar visible")->isChecked();
    pnlcbs->find("Root Button Visible")->setEnabled(enableTB);
    pnlcbs->find("Home Button Visible")->setEnabled(enableTB);
    pnlcbs->find("Up Button Visible")->setEnabled(enableTB);
    pnlcbs->find("Equal Button Visible")->setEnabled(enableTB);
    pnlcbs->find("Open Button Visible")->setEnabled(enableTB);
    pnlcbs->find("SyncBrowse Button Visible")->setEnabled(enableTB);
}

int KgPanel::activeSubPage()
{
    return tabWidget->currentIndex();
}
