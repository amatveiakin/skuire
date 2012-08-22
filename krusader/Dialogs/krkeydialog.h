/*****************************************************************************
 * Copyright (C) 2006 Jonas Bähr <jonas.baehr@web.de>                        *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This package is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this package; if not, write to the Free Software               *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA *
 *****************************************************************************/

#ifndef KRKEYDIALOG_H
#define KRKEYDIALOG_H

#include <kshortcutsdialog.h>

/**
 * @short KDE's KKeyDialog extended by the ability to export/import shortcuts
 */
class KrKeyDialog : public KShortcutsDialog
{
    Q_OBJECT
public:
    KrKeyDialog(QWidget* parent, KActionCollection *actionCollection);
    ~KrKeyDialog();

private slots:
    void slotImportShortcuts();
    void slotExportShortcuts();

private:
    void importLegacyShortcuts(const QString& file);

    KShortcutsEditor* _chooser;
    KActionCollection *_actionCollection;
};

#endif
