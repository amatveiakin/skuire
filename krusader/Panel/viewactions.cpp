/***************************************************************************
                          viewactions.cpp
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

                                               S o u r c e    F i l e

***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "viewactions.h"

#include "krview.h"
#include "../krusaderapp.h"
#include "abstractmainwindow.h"

#include <klocale.h>
#include <ktoggleaction.h>

ViewActions::ViewActions(QObject *parent, AbstractMainWindow *mainWindow) :
    ActionsBase(parent, mainWindow)
{
    // zoom
    actZoomIn = action(i18n("Zoom In"), "zoom-in", 0, SLOT(zoomIn()), "zoom_in");
    actZoomOut = action(i18n("Zoom Out"), "zoom-out", 0, SLOT(zoomOut()), "zoom_out");
    actDefaultZoom = action(i18n("Default Zoom"), "zoom-original", 0, SLOT(defaultZoom()), "default_zoom");

    // filter
    action(i18n("&All Files"), 0, Qt::SHIFT + Qt::Key_F10, SLOT(allFilter()), "all files");
    //actExecFilter = new KAction( i18n( "&Executables" ), SHIFT + Qt::Key_F11,
    //                             SLOTS, SLOT( execFilter() ), actionCollection(), "exec files" );
    action(i18n("&Custom"), 0, Qt::SHIFT + Qt::Key_F12, SLOT(customFilter()), "custom files");
    actToggleGlobalFilter = toggleAction(i18n("Enable Global Filter"), 0, 0, SLOT(toggleGlobalFilter(bool)), "toggle_global_filter");
    actSetGlobalFilter = action(i18n("Set Global Filter"), 0, 0, SLOT(setGlobalFilter()), "set_global_filter");

    // selection
    actSelect = action(i18n("Select &Group..."), "kr_select", Qt::CTRL + Qt::Key_Plus, SLOT(markGroup()), "select group");
    actSelectAll = action(i18n("&Select All"), "kr_selectall", Qt::ALT + Qt::Key_Plus, SLOT(markAll()), "select all");
    actUnselect = action(i18n("&Unselect Group..."), "kr_unselect", Qt::CTRL + Qt::Key_Minus, SLOT(unmarkGroup()), "unselect group");
    actUnselectAll = action(i18n("U&nselect All"), "kr_unselectall", Qt::ALT + Qt::Key_Minus, SLOT(unmarkAll()), "unselect all");
    actInvert = action(i18n("&Invert Selection"), "kr_invert", Qt::ALT + Qt::Key_Asterisk, SLOT(invertSelection()), "invert");
    actRestoreSelection = action(i18n("Restore Selection"), 0, 0, SLOT(restoreSelection()), "restore_selection");

    // other stuff
    action(i18n("Show View Options Menu"), 0, 0, SLOT(showOptionsMenu()), "show_view_options_menu");
    action(i18n("Set Focus to the Panel"), 0, Qt::Key_Escape, SLOT(focusPanel()), "focus_panel");
    action(i18n("Apply settings to other tabs"), 0, 0, SLOT(applySettingsToOthers()), "view_apply_settings_to_others");
    action(i18n("QuickFilter"), 0, Qt::CTRL + Qt::Key_I, SLOT(quickFilter()), "quick_filter");
    actTogglePreviews = toggleAction(i18n("Show Previews"), 0, 0, SLOT(showPreviews(bool)), "toggle previews");
    actToggleHidden = toggleAction(i18n("Show &Hidden Files"), 0, Qt::CTRL + Qt::Key_Period, SLOT(showHidden(bool)), "toggle hidden files");
    KAction *actSaveaveDefaultSettings = action(i18n("Save settings as default"), 0, 0, SLOT(saveDefaultSettings()), "view_save_default_settings");

    // tooltips
    actSelect->setToolTip(i18n("Select files using a filter"));
    actSelectAll->setToolTip(i18n("Select all files in the current directory"));
    actUnselectAll->setToolTip(i18n("Unselect all selected files"));
    actSaveaveDefaultSettings->setToolTip(i18n("Save settings as default for new instances of this view type"));

    connect(KrusaderApp::self(), SIGNAL(configChanged()), SLOT(configChanged()));
}

inline KrView *ViewActions::activeView()
{
    if (AbstractView *view = _mainWindow->activeView()) {
        KrView *v = qobject_cast<KrView*>(view->self());
        Q_ASSERT(v);
        return v;
    } else
        return 0;
}

void ViewActions::onViewCreated(AbstractView *view)
{
    connect(view->emitter(), SIGNAL(refreshActions()), SLOT(refreshActions()));
}

// zoom

void ViewActions::zoomIn()
{
    activeView()->zoomIn();
}

void ViewActions::zoomOut()
{
    activeView()->zoomOut();
}

void ViewActions::defaultZoom()
{
    activeView()->setDefaultFileIconSize();
}

// filter

void ViewActions::allFilter()
{
    activeView()->setFilter(KrViewProperties::All);
}
#if 0
void ViewActions::execFilter()
{
    activeView()->setFilter(KrViewProperties::All);
}
#endif
void ViewActions::customFilter()
{
    activeView()->setFilter(KrViewProperties::Custom);
}

void ViewActions::showOptionsMenu()
{
    activeView()->showContextMenu();
}

// selection

void ViewActions::markAll()
{
    activeView()->changeSelection(KRQuery("*"), true);
}

void ViewActions::unmarkAll()
{
    activeView()->changeSelection(KRQuery("*"), false);
}

void ViewActions::markGroup()
{
    activeView()->customSelection(true);
}

void ViewActions::unmarkGroup()
{
    activeView()->customSelection(false);
}

void ViewActions::invertSelection()
{
    activeView()->invertSelection();
}

void ViewActions::restoreSelection()
{
    activeView()->restoreSelection();
}

// other stuff

void ViewActions::saveDefaultSettings()
{
    activeView()->saveDefaultSettings();
}

void ViewActions::applySettingsToOthers()
{
    activeView()->applySettingsToOthers();
}

void ViewActions::focusPanel()
{
    activeView()->widget()->setFocus();
}

void ViewActions::quickFilter()
{
    activeView()->startQuickFilter();
}

void ViewActions::showPreviews(bool show)
{
    activeView()->showPreviews(show);
}

void ViewActions::showHidden(bool show)
{
    KrView::showHidden(show);
}

void ViewActions::toggleGlobalFilter(bool toggle)
{
    KrView::enableGlobalFilter(toggle);
}

void ViewActions::setGlobalFilter()
{
    KrView::setGlobalFilter();
    refreshActions();
}

void ViewActions::refreshActions()
{
    actDefaultZoom->setEnabled(activeView()->defaultFileIconSize() != activeView()->fileIconSize());
    int idx = KrView::iconSizes.indexOf(activeView()->fileIconSize());
    actZoomOut->setEnabled(idx > 0);
    actZoomIn->setEnabled(idx < (KrView::iconSizes.count() - 1));
    actRestoreSelection->setEnabled(activeView()->canRestoreSelection());
    actTogglePreviews->setChecked(activeView()->previewsShown());
    actToggleHidden->setChecked(KrView::isShowHidden());
    actToggleGlobalFilter->setChecked(KrView::isGlobalFilterEnabled());
    actToggleGlobalFilter->setEnabled(KrView::isGlobalFilterSet());
}

void ViewActions::configChanged()
{
    refreshActions();
}
