/*****************************************************************************
 * Copyright (C) 2004 Jonas Bähr <jonas.baehr@web.de>                        *
 * Copyright (C) 2004 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2011 Jan Lepper <jan_lepper@gmx.de>                         *
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

#include "viewexpplaceholders.h"

#include "krview.h"
#include "../UserAction/expander.h"
#include "abstractlistpanel.h"

#include <klocale.h>
#include <QObject>


//FIXME: inherit exp_simpleplaceholder
class ViewExpPlaceholder : public exp_placeholder
{
public:
    virtual TagString expFunc(const AbstractListPanel* panel, const TagStringList& parameter, const bool& useUrl, Expander& exp) const;
    virtual TagString expFuncImpl(KrView *view, const QStringList& parameter, Expander &exp) const = 0;
};

TagString ViewExpPlaceholder::expFunc(const AbstractListPanel* panel, const TagStringList& parameter, const bool& useUrl, Expander& exp) const
{
    Q_UNUSED(useUrl)
    Q_UNUSED(parameter)
    Q_UNUSED(exp)

    QStringList lst;
    for (TagStringList::const_iterator it = parameter.begin(), end = parameter.end();it != end;++it) {
        if ((*it).isSimple())
            lst.push_back((*it).string());
        else {
            setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_SYNTAX,
                                i18n("%Each% is not allowed in parameter to %1", description())));
            return QString();
        }
    }

    if (!panel) {
        panelMissingError(_expression, exp);
        return QString();
    }

    if (KrView *view = qobject_cast<KrView*>(panel->view()->self()))
        return expFuncImpl(view, lst, exp);
    else {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_WORLD,
                            i18n("%1 View doesn't implement the necessary interface", expression())));
        return QString();
    }
}


/**
  * expands %_Filter% ('_' is replaced by 'a', 'o', 'r' or 'l' to indicate the active, other, right or left panel) with the correspondend filter (ie: "*.cpp")
  */
class ExpFilter : public ViewExpPlaceholder
{
public:
    ExpFilter();
    virtual TagString expFuncImpl(KrView *view, const QStringList& parameter, Expander &exp) const;
};

ExpFilter::ExpFilter()
{
    _expression = "Filter";
    _description = i18n("Filter Mask (*.h, *.cpp, etc.)");
    _needPanel = true;
}

TagString ExpFilter::expFuncImpl(KrView *view, const QStringList& parameter, Expander &exp) const
{
    Q_UNUSED(parameter)
    Q_UNUSED(exp)

    return view->filterMask().nameFilter();
}


/**
  * This sets the sorting on a specific colunm
  */
class ExpColSort : public ViewExpPlaceholder
{
public:
    ExpColSort();
    virtual TagString expFuncImpl(KrView *view, const QStringList& parameter, Expander &exp) const;
};

ExpColSort::ExpColSort()
{
    _expression = "ColSort";
    _description = i18n("Set Sorting for This Panel...");
    _needPanel = true;

    QStringList columnDescriptions;
    for(int col = KrViewProperties::Name; col < KrViewProperties::MAX_COLUMNS; col++)
        columnDescriptions << KrView::columnId(col);

    addParameter(exp_parameter(i18n("Choose a column:"), "__choose:" + columnDescriptions.join(";"), true));
    addParameter(exp_parameter(i18n("Choose a sort sequence:"), "__choose:Toggle;Asc;Desc", false));
}

TagString ExpColSort::expFuncImpl(KrView *view, const QStringList& parameter, Expander &exp) const
{
    if (parameter.count() == 0 || parameter[0].isEmpty()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: no column specified for %_ColSort(column)%")));
        return QString();
    }

    KrViewProperties::ColumnType oldColumn = view->properties()->sortColumn;
    KrViewProperties::ColumnType column = KrViewProperties::NoColumn;

    for(int col = KrViewProperties::Name; col < KrViewProperties::MAX_COLUMNS; col++) {
        if (parameter[0].toLower() == KrView::columnId(col)) {
            column = static_cast<KrViewProperties::ColumnType>(col);
            break;
        }
    }
    if (column == KrViewProperties::NoColumn) {
        setError(exp, Error(Error::exp_S_WARNING, Error::exp_C_ARGUMENT, i18n("Expander: unknown column specified for %_ColSort(%1)%", parameter[0])));
        return QString();
    }

    bool descending = view->properties()->sortOptions & KrViewProperties::Descending;

    if (parameter.count() <= 1 || (parameter[1].toLower() != "asc" && parameter[1].toLower() != "desc")) { // no sortdir parameter
        if(column == oldColumn) // reverse direction if column is unchanged
            descending = !descending;
        else // otherwise set to ascending
            descending = false;
    } else { // sortdir specified
        if (parameter[1].toLower() == "asc")
            descending = false;
        else // == desc
            descending = true;
    }

    view->setSortMode(column, descending);

    return QString();  // this doesn't return anything, that's normal!
}


void ViewExpPlaceholders::init()
{
    Expander::registerPlaceholder(new ExpFilter);
    Expander::registerPlaceholder(new ExpColSort);
}
