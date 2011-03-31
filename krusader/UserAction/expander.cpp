/*****************************************************************************
 * Copyright (C) 2004 Jonas Bähr <jonas.baehr@web.de>                        *
 * Copyright (C) 2004 Shie Erlich <erlich@users.sourceforge.net>             *
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

#include "expander.h"

#include "../krusader.h"
#include "../krusaderview.h"
#include "../panelmanager.h"
#include "../Panel/listpanel.h"
#include "../Panel/panelfunc.h"
#include "../Panel/krview.h"
#include "../Synchronizer/synchronizergui.h"
#include "../Search/krsearchdialog.h"
#include "../GUI/profilemanager.h"
#include "../VFS/preservingcopyjob.h"
#include "../KViewer/krviewer.h"
#include "../krservices.h"

#ifdef __KJSEMBED__
#include "../KrJS/krjs.h"
#endif

#include <kdebug.h>
#include <kinputdialog.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <ktemporaryfile.h>
#include <QtCore/QStringList>
#include <QtGui/QClipboard>
#include <QtCore/QTextStream>
#include <QList>

#include <functional>
#include <algorithm>
using namespace std;

#define NEED_PANEL if (panel==0) { panelMissingError(_expression,exp); return QString(); }

#include "tstring.h"

QList<const exp_placeholder*>& Expander::_placeholder()
{
    static QList<const exp_placeholder*> ret;
    return ret;
}

void exp_placeholder::panelMissingError(const QString &s, Expander& exp)
{
    exp.setError(Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Needed panel specification missing in expander %1", s)));
}

QStringList exp_placeholder::fileList(const KrPanel* const panel, const QString& type, const QString& mask, const bool omitPath, const bool useUrl, Expander& exp, const QString& error)
{
    FileItemList items;
    if (type.isEmpty() || type == "all")
        items = panel->view->getItems(mask);
    else if (type == "files")
        items = panel->view->getItems(mask, false, true);
    else if (type == "dirs")
        items = panel->view->getItems(mask, true, false);
    else if (type == "selected")
        items = panel->view->getSelectedItems(true);
    else {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: Bad argument to %1: %2 is not valid item specifier", error, type)));
        return QStringList();
    }

    QStringList list;
    foreach(KFileItem item, items) {
        if(omitPath)
            list << item.name();
        else
            list << (useUrl ? item.url().url() : item.url().path());
    }
    return list;
}

namespace
{

class exp_simpleplaceholder : public exp_placeholder
{
public:
    EXP_FUNC;
    virtual TagString expFunc(const KrPanel*, const QStringList&, const bool&, Expander&) const = 0;
};

/**
  * expands %_Path% ('_' is replaced by 'a', 'o', 'r' or 'l' to indicate the active, other, right or left panel) with the path of the specified panel
  */
class exp_Path : public exp_simpleplaceholder
{
    static const exp_Path instance;
    exp_Path();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * expands %_Count% ('_' is replaced by 'a', 'o', 'r' or 'l' to indicate the active, other, right or left panel) with the number of items, which type is specified by the first Parameter
  */
class exp_Count : public exp_simpleplaceholder
{
    static const exp_Count instance;
    exp_Count();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * expands %_Filter% ('_' is replaced by 'a', 'o', 'r' or 'l' to indicate the active, other, right or left panel) with the correspondend filter (ie: "*.cpp")
  */
class exp_Filter : public exp_simpleplaceholder
{
    static const exp_Filter instance;
    exp_Filter();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * expands %_Current% ('_' is replaced by 'a', 'o', 'r' or 'l' to indicate the active, other, right or left panel) with the current item ( != the selected onec)
  */
class exp_Current : public exp_simpleplaceholder
{
    static const exp_Current instance;
    exp_Current();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * expands %_List% ('_' is replaced by 'a', 'o', 'r' or 'l' to indicate the active, other, right or left panel) with a list of items, which type is specified by the first Parameter
  */
class exp_List : public exp_simpleplaceholder
{
    static const exp_List instance;
    exp_List();
public:
    SIMPLE_EXP_FUNC;
};

/**
 * expands %_ListFile% ('_' is replaced by 'a', 'o', 'r' or 'l' to indicate
 * the active, other, right or left panel) with the name of a temporary file,
 * containing a list of items, which type is specified by the first Parameter
 */
class exp_ListFile : public exp_simpleplaceholder
{
    static const exp_ListFile instance;
    exp_ListFile();
public:
    SIMPLE_EXP_FUNC;
};

/**
 * expands %_Ask% ('_' is necessary because there is no panel needed)
 * with the return of an input-dialog
 */
class exp_Ask : public exp_simpleplaceholder
{
    static const exp_Ask instance;
    exp_Ask();
public:
    SIMPLE_EXP_FUNC;
};

/**
 * This copies it's first Parameter to the clipboard
 */
class exp_Clipboard : public exp_placeholder
{
    static const exp_Clipboard instance;
    exp_Clipboard();
public:
    EXP_FUNC;
};

/**
  * This selects all items by the mask given with the first Parameter
  */
class exp_Select : public exp_simpleplaceholder
{
    static const exp_Select instance;
    exp_Select();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * This changes the panel'spath to the value given with the first Parameter.
  */
class exp_Goto : public exp_simpleplaceholder
{
    static const exp_Goto instance;
    exp_Goto();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * This is equal to 'cp <first Parameter> <second Parameter>'.
  */
class exp_Copy : public exp_placeholder
{
    static const exp_Copy instance;
    exp_Copy();
public:
    EXP_FUNC;
};

/**
  * This is equal to 'mv <first Parameter> <second Parameter>'.
  */
class exp_Move : public exp_placeholder
{
    static const exp_Move instance;
    exp_Move();
public:
    EXP_FUNC;
};

/**
  * This opens the synchronizer with a given profile
  */
class exp_Sync : public exp_simpleplaceholder
{
    static const exp_Sync instance;
    exp_Sync();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * This opens the searchmodule with a given profile
  */
class exp_NewSearch : public exp_simpleplaceholder
{
    static const exp_NewSearch instance;
    exp_NewSearch();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * This loads the panel-profile with a given name
  */
class exp_Profile : public exp_simpleplaceholder
{
    static const exp_Profile instance;
    exp_Profile();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * This is setting marks in the string where he is later split up for each {all, selected, files, dirs}
  */
class exp_Each : public exp_simpleplaceholder
{
    static const exp_Each instance;
    exp_Each();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * This sets the sorting on a specific colunm
  */
class exp_ColSort : public exp_simpleplaceholder
{
    static const exp_ColSort instance;
    exp_ColSort();
public:
    SIMPLE_EXP_FUNC;
};

/**
  * This sets relation between the left and right panel
  */
class exp_PanelSize : public exp_simpleplaceholder
{
    static const exp_PanelSize instance;
    exp_PanelSize();
public:
    SIMPLE_EXP_FUNC;
};

#ifdef __KJSEMBED__
/**
  * This sets relation between the left and right panel
  */
class exp_Script : public exp_simpleplaceholder
{
    static const exp_Script instance;
    exp_Script();
public:
    SIMPLE_EXP_FUNC;
};

const exp_Script exp_Script::instance;

#endif

/**
  * This loads a file in the internal viewer
  */
class exp_View : public exp_simpleplaceholder
{
    static const exp_View instance;
    exp_View();
public:
    SIMPLE_EXP_FUNC;
};

const exp_View exp_View::instance;
const exp_PanelSize exp_PanelSize::instance;
const exp_ColSort exp_ColSort::instance;
const exp_Each exp_Each::instance;
const exp_Profile exp_Profile::instance;
const exp_NewSearch exp_NewSearch::instance;
const exp_Sync exp_Sync::instance;
const exp_Move exp_Move::instance;
const exp_Copy exp_Copy::instance;
const exp_Goto exp_Goto::instance;
const exp_Select exp_Select::instance;
const exp_Clipboard exp_Clipboard::instance;
const exp_Ask exp_Ask::instance;
const exp_ListFile exp_ListFile::instance;
const exp_List exp_List::instance;
const exp_Current exp_Current::instance;
const exp_Filter exp_Filter::instance;
const exp_Count exp_Count::instance;
const exp_Path exp_Path::instance;

////////////////////////////////////////////////////////////
//////////////////////// utils ////////////////////////
////////////////////////////////////////////////////////////

/**
 * escapes everything that confuses bash in filenames
 * @param s String to manipulate
 * @return escaped string
 */
QString bashquote(QString s)
{
    /*
    // we _can_not_ use this function because it _encloses_ the sting in single-quots!
    // In this case quotes strings could not be concaternated anymore
    return KrServices::quote(s);
    */

    static const QString evilstuff = "\\\"'`()[]{}!?;$&<>| \t\r\n";  // stuff that should get escaped

    for (int i = 0; i < evilstuff.length(); ++i)
        s.replace(evilstuff[ i ], ('\\' + evilstuff[ i ]));

    return s;
}

QString separateAndQuote(QStringList list, const QString& separator, const bool quote)
{
    if (quote)
        transform(list.begin(), list.end(), list.begin(), bashquote);
    return list.join(separator);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// expander classes ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

exp_Path::exp_Path()
{
    _expression = "Path";
    _description = i18n("Panel's Path...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Automatically escape spaces"), "__yes", false));
}
TagString exp_Path::expFunc(const KrPanel* panel, const QStringList& parameter, const bool& useUrl, Expander& exp) const
{
    NEED_PANEL

    QString result;

    if (useUrl)
        result = panel->func->files()->vfs_getOrigin().url() + '/';
    else
        result = panel->func->files()->vfs_getOrigin().path() + '/';

    if (parameter.count() > 0 && parameter[0].toLower() == "no")    // don't escape spaces
        return TagString(result);
    else
        return TagString(bashquote(result));
}

exp_Count::exp_Count()
{
    _expression = "Count";
    _description = i18n("Number of...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Count:"), "__choose:All;Files;Dirs;Selected", false));
}
TagString exp_Count::expFunc(const KrPanel* panel, const QStringList& parameter, const bool&, Expander& exp) const
{
    NEED_PANEL

    int n = -1;
    if (parameter.count() == 0 || parameter[ 0 ].isEmpty() || parameter[ 0 ].toLower() == "all")
        n = panel->view->numDirs() + panel->view->numFiles();
    else if (parameter[ 0 ].toLower() == "files")
        n = panel->view->numFiles();
    else if (parameter[ 0 ].toLower() == "dirs")
        n = panel->view->numDirs();
    else if (parameter[ 0 ].toLower() == "selected")
        n = panel->view->numSelected();
    else {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: Bad argument to Count: %1 is not valid item specifier", parameter[0])));
        return QString();
    }

    return TagString(QString("%1").arg(n));
}

exp_Filter::exp_Filter()
{
    _expression = "Filter";
    _description = i18n("Filter Mask (*.h, *.cpp, etc.)");
    _needPanel = true;
}
TagString exp_Filter::expFunc(const KrPanel* panel, const QStringList&, const bool&, Expander& exp) const
{
    NEED_PANEL

    return panel->view->filterMask().nameFilter();
}

exp_Current::exp_Current()
{
    _expression = "Current";
    _description = i18n("Current File (!= Selected File)...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Omit the current path (optional)"), "__no", false));
    addParameter(exp_parameter(i18n("Automatically escape spaces"), "__yes", false));
}
TagString exp_Current::expFunc(const KrPanel* panel, const QStringList& parameter, const bool& useUrl, Expander& exp) const
{
    NEED_PANEL

    QString item = panel->view->getCurrentItem();

    QString result;

    if (parameter.count() > 0 && parameter[0].toLower() == "yes")    // omit the current path
        result = item;
    else {
        KUrl url = panel->func->files()->vfs_getFile(item);
        if (useUrl)
            result = url.url();
        else
            result = url.path();
    }

    if (parameter.count() > 1 && parameter[1].toLower() == "no")    // don't escape spaces
        return result;
    else
        return bashquote(result);
}

exp_List::exp_List()
{
    _expression = "List";
    _description = i18n("Item List of...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Which items:"), "__choose:All;Files;Dirs;Selected", false));
    addParameter(exp_parameter(i18n("Separator between the items (optional):"), " ", false));
    addParameter(exp_parameter(i18n("Omit the current path (optional)"), "__no", false));
    addParameter(exp_parameter(i18n("Mask (optional, all but 'Selected'):"), "__select", false));
    addParameter(exp_parameter(i18n("Automatically escape spaces"), "__yes", false));
}
TagString exp_List::expFunc(const KrPanel* panel, const QStringList& parameter, const bool& useUrl, Expander& exp) const
{
    NEED_PANEL

    // get selected items from view
    QStringList items;
    QString mask;

    if (parameter.count() <= 3 || parameter[3].isEmpty())
        mask = '*';
    else
        mask = parameter[3];

    return separateAndQuote(
               fileList(panel,
                        parameter.isEmpty() ? QString() : parameter[0].toLower(),
                        mask, parameter.count() > 2 ? parameter[2].toLower() == "yes" : false,
                        useUrl, exp, "List"),
               parameter.count() > 1 ? parameter[1] : " ",
               parameter.count() > 4 ? parameter[4].toLower() == "yes" : true);
}

exp_ListFile::exp_ListFile()
{
    _expression = "ListFile";
    _description = i18n("Filename of an Item List...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Which items:"), "__choose:All;Files;Dirs;Selected", false));
    addParameter(exp_parameter(i18n("Separator between the items (optional)"), "\n", false));
    addParameter(exp_parameter(i18n("Omit the current path (optional)"), "__no", false));
    addParameter(exp_parameter(i18n("Mask (optional, all but 'Selected'):"), "__select", false));
    addParameter(exp_parameter(i18n("Automatically escape spaces"), "__no", false));
}
TagString exp_ListFile::expFunc(const KrPanel* panel, const QStringList& parameter, const bool& useUrl, Expander& exp) const
{
    NEED_PANEL

    // get selected items from view
    QStringList items;
    QString mask;

    if (parameter.count() <= 3 || parameter[3].isEmpty())
        mask = '*';
    else
        mask = parameter[3];
    KTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false);
    tmpFile.setSuffix(".itemlist");

    if (!tmpFile.open()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_WORLD, i18n("Expander: tempfile couldn't be opened (%1)", tmpFile.errorString())));
        return QString();
    }

    QTextStream stream(&tmpFile);
    stream << separateAndQuote(
        fileList(panel,
                 parameter.isEmpty() ? QString() : parameter[0].toLower(),
                 mask, parameter.count() > 2 ? parameter[2].toLower() == "yes" : false,
                 useUrl, exp, "ListFile"),
        parameter.count() > 1 ? parameter[1] : "\n",
        parameter.count() > 4 ? parameter[4].toLower() == "yes" : true)
    << "\n";
    tmpFile.close();

    return tmpFile.fileName();
}

exp_Select::exp_Select()
{
    _expression = "Select";
    _description = i18n("Manipulate the Selection...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Selection mask:"), "__select", true));
    addParameter(exp_parameter(i18n("Manipulate in which way:"), "__choose:Set;Add;Remove", false));
}
TagString exp_Select::expFunc(const KrPanel* panel, const QStringList& parameter, const bool& , Expander& exp) const
{
    NEED_PANEL

    KRQuery mask;
    if (parameter.count() <= 0 || parameter[0].isEmpty())
        mask = KRQuery("*");
    else
        mask = KRQuery(parameter[0]);

    if (parameter.count() > 1 && parameter[1].toLower() == "list-add")
        panel->view->select(mask);
    else if (parameter.count() > 1 && parameter[1].toLower() == "list-remove")
        panel->view->unselect(mask);
    else { // parameter[1].toLower() == "set" or isEmpty() or whatever
        panel->view->unselect(KRQuery("*"));
        panel->view->select(mask);
    }

    return QString();  // this doesn't return anything, that's normal!
}

exp_Goto::exp_Goto()
{
    _expression = "Goto";
    _description = i18n("Jump to a Location...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Choose a path:"), "__goto", true));
    addParameter(exp_parameter(i18n("Open location in a new tab"), "__no", false));
}
TagString exp_Goto::expFunc(const KrPanel* panel, const QStringList& parameter, const bool&, Expander& exp) const
{
    NEED_PANEL

    bool newTab = false;
    if (parameter.count() > 1 && parameter[1].toLower() == "yes")
        newTab = true;

    if (parameter.count() == 0) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: at least 1 parameter is required for Goto!")));
        return QString();
    }

    if (newTab) {
        if (panel == LEFT_PANEL)
            MAIN_VIEW->leftMng->slotNewTab(parameter[0]);
        else
            MAIN_VIEW->rightMng->slotNewTab(parameter[0]);
    } else {
        panel->func->openUrl(parameter[0], "");
        panel->gui->slotFocusOnMe();
    }

    return QString();  // this doesn't return anything, that's normal!
}

/*
exp_Search::exp_Search() {
   _expression = "Search";
   _description = i18n("Search for files");
   _needPanel = true;

   addParameter( new exp_parameter( i18n("please choose the setting"), "__searchprofile", true ) );
   addParameter( new exp_parameter( i18n("open the search in a new tab"), "__yes", false ) );  //TODO: add this also to panel-dependent as soon as vfs support the display of search-results
}
*/

exp_Ask::exp_Ask()
{
    _expression = "Ask";
    _description = i18n("Ask Parameter from User...");
    _needPanel = false;

    addParameter(exp_parameter(i18n("Question:"), "Where do you want do go today?", true));
    addParameter(exp_parameter(i18n("Preset (optional):"), "", false));
    addParameter(exp_parameter(i18n("Caption (optional):"), "", false));
}
TagString exp_Ask::expFunc(const KrPanel*, const QStringList& parameter, const bool&, Expander& exp) const
{
    QString caption, preset, result;

    if (parameter.count() == 0) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: at least 1 parameter is required for Ask!")));
        return QString();
    }

    if (parameter.count() <= 2 || parameter[2].isEmpty())
        caption = i18n("User Action");
    else
        caption = parameter[2];
    if (parameter.count() <= 1 || parameter[1].isEmpty())
        preset.clear();
    else
        preset = parameter[1];

    bool ok;
    result = KInputDialog::getText(
                 caption,
                 parameter[0],
                 preset,
                 &ok);

    if (ok)
        return result;
    else {
        setError(exp, Error(Error::exp_S_ERROR, Error::exp_C_USER, "User cancelled"));
        return QString();
    }
}

exp_Clipboard::exp_Clipboard()
{
    _expression = "Clipboard";
    _description = i18n("Copy to Clipboard...");
    _needPanel = false;

    addParameter(exp_parameter(i18n("What to copy:"), "__placeholder", true));
    addParameter(exp_parameter(i18n("Append to current clipboard content with this separator (optional):"), "", false));
}
TagString exp_Clipboard::expFunc(const KrPanel*, const TagStringList& parameter, const bool&, Expander& exp) const
{
//    kDebug() << "Expander::exp_Clipboard, parameter[0]: '" << parameter[0] << "', Clipboard: " << KApplication::clipboard()->text() << endl;
    if (parameter.count() == 0) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: at least 1 parameter is required for Clipboard!")));
        return QString();
    }

    QStringList lst = splitEach(parameter[0]);
    if (parameter.count() > 1 && !parameter[1].isSimple()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_SYNTAX, i18n("Expander: %Each% may not be in the second argument of %Clipboard%")));
        return QString();
    }
    if (parameter.count() <= 1 || parameter[1].string().isEmpty() || KApplication::clipboard()->text().isEmpty())
        KApplication::clipboard()->setText(lst.join("\n"));
    else
        KApplication::clipboard()->setText(KApplication::clipboard()->text() + parameter[1].string() + lst.join("\n"));

    return QString();  // this doesn't return anything, that's normal!
}

exp_Copy::exp_Copy()
{
    _expression = "Copy";
    _description = i18n("Copy a File/Directory...");
    _needPanel = false;

    addParameter(exp_parameter(i18n("What to copy:"), "__placeholder", true));
    addParameter(exp_parameter(i18n("Where to copy:"), "__placeholder", true));
}
TagString exp_Copy::expFunc(const KrPanel*, const TagStringList& parameter, const bool&, Expander& exp) const
{
    if (parameter.count() < 2) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: at least 2 parameter is required for Copy!")));
        return QString();
    }

    // basically the parameter can already be used as URL, but since KUrl has problems with ftp-proxy-urls (like ftp://username@proxyusername@url...) this is neccesary:
    QStringList lst = splitEach(parameter[0]);
    if (!parameter[1].isSimple()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_SYNTAX, i18n("Expander: %Each% may not be in the second argument of %Copy%")));
        return QString();
    }
    KUrl::List src;
    for (QStringList::const_iterator it = lst.constBegin(), end = lst.constEnd();it != end;++it)
        src.push_back(KUrl(*it));
    // or transform(...) ?
    KUrl dest = KUrl(parameter[1].string());

    if (!dest.isValid() || find_if(src.constBegin(), src.constEnd(), not1(mem_fun_ref(&KUrl::isValid))) != src.constEnd()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: invalid URLs in %_Copy(\"src\", \"dest\")%")));
        return QString();
    }

    PreservingCopyJob::createCopyJob(PM_DEFAULT, src, dest, KIO::CopyJob::Copy, false, true);

    return QString();  // this doesn't return everything, that's normal!
}

exp_Move::exp_Move()
{
    _expression = "Move";
    _description = i18n("Move/Rename a File/Directory...");
    _needPanel = false;

    addParameter(exp_parameter(i18n("What to move/rename:"), "__placeholder", true));
    addParameter(exp_parameter(i18n("New target/name:"), "__placeholder", true));
}
TagString exp_Move::expFunc(const KrPanel*, const TagStringList& parameter, const bool& , Expander& exp) const
{
    if (parameter.count() < 2) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: at least 2 parameter is required for Move!")));
        return QString();
    }

    // basically the parameter can already be used as URL, but since KUrl has problems with ftp-proxy-urls (like ftp://username@proxyusername@url...) this is neccesary:
    QStringList lst = splitEach(parameter[0]);
    if (!parameter[1].isSimple()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_SYNTAX, i18n("%Each% may not be in the second argument of %Move%")));
        return QString();
    }
    KUrl::List src;
    for (QStringList::const_iterator it = lst.constBegin(), end = lst.constEnd();it != end;++it)
        src.push_back(KUrl(*it));
    // or transform(...) ?
    KUrl dest = KUrl(parameter[1].string());

    if (!dest.isValid() || find_if(src.constBegin(), src.constEnd(), not1(mem_fun_ref(&KUrl::isValid))) != src.constEnd()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: invalid URLs in %_Move(\"src\", \"dest\")%")));
        return QString();
    }

    PreservingCopyJob::createCopyJob(PM_DEFAULT, src, dest, KIO::CopyJob::Move, false, true);

    return QString();  // this doesn't return anything, that's normal!
}

exp_Sync::exp_Sync()
{
    _expression = "Sync";
    _description = i18n("Load a Synchronizer Profile...");
    _needPanel = false;

    addParameter(exp_parameter(i18n("Choose a profile:"), "__syncprofile", true));
}
TagString exp_Sync::expFunc(const KrPanel*, const QStringList& parameter, const bool&, Expander& exp) const
{
    if (parameter.count() == 0 || parameter[0].isEmpty()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: no profile specified for %_Sync(profile)%")));
        return QString();
    }

    new SynchronizerGUI(0, parameter[0]);

    return QString();  // this doesn't return everything, that's normal!
}

exp_NewSearch::exp_NewSearch()
{
    _expression = "NewSearch";
    _description = i18n("Load a Searchmodule Profile...");
    _needPanel = false;

    addParameter(exp_parameter(i18n("Choose a profile:"), "__searchprofile", true));
}
TagString exp_NewSearch::expFunc(const KrPanel*, const QStringList& parameter, const bool&, Expander& exp) const
{
    if (parameter.count() == 0 || parameter[0].isEmpty()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: no profile specified for %_NewSearch(profile)%")));
        return QString();
    }

    new KrSearchDialog(parameter[0], krApp);

    return QString();  // this doesn't return everything, that's normal!
}

exp_Profile::exp_Profile()
{
    _expression = "Profile";
    _description = i18n("Load a Panel Profile...");
    _needPanel = false;

    addParameter(exp_parameter(i18n("Choose a profile:"), "__panelprofile", true));
}
TagString exp_Profile::expFunc(const KrPanel*, const QStringList& parameter, const bool&, Expander& exp) const
{
    if (parameter.count() == 0 || parameter[0].isEmpty()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: no profile specified for %_Profile(profile)%; abort...")));
        return QString();
    }

    MAIN_VIEW->profiles(parameter[0]);

    return QString();  // this doesn't return everything, that's normal!
}

exp_Each::exp_Each()
{
    _expression = "Each";
    _description = i18n("Separate Program Call for Each...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Which items:"), "__choose:All;Files;Dirs;Selected", false));
    addParameter(exp_parameter(i18n("Omit the current path (optional)"), "__no", false));
    addParameter(exp_parameter(i18n("Mask (optional, all but 'Selected'):"), "__select", false));
    addParameter(exp_parameter(i18n("Automatically escape spaces"), "__yes", false));
}
TagString exp_Each::expFunc(const KrPanel* panel, const QStringList& parameter, const bool& useUrl, Expander& exp) const
{
    NEED_PANEL

    QString mask;
    if (parameter.count() <= 2 || parameter[2].isEmpty())
        mask = '*';
    else
        mask = parameter[2];

    TagString ret;
    QStringList l = fileList(panel,
                             parameter.empty() ? QString() : parameter[0].toLower(),
                             mask, parameter.count() > 1 && parameter[1].toLower() == "yes",
                             useUrl, exp, "Each");

    if (!(parameter.count() <= 3 || parameter[3].toLower() != "yes"))
        transform(l.begin(), l.end(), l.begin(), bashquote);

    ret.insertTag(0, l);
    return ret;
}

exp_ColSort::exp_ColSort()
{
    _expression = "ColSort";
    _description = i18n("Set Sorting for This Panel...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Choose a column:"), "__choose:Name;Ext;Type;Size;Modified;Perms;rwx;Owner;Group", true));
    addParameter(exp_parameter(i18n("Choose a sort sequence:"), "__choose:Toggle;Asc;Desc", false));
}
TagString exp_ColSort::expFunc(const KrPanel* panel, const QStringList& parameter, const bool&, Expander& exp) const
{
    NEED_PANEL

    if (parameter.count() == 0 || parameter[0].isEmpty()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: no column specified for %_ColSort(column)%")));
        return QString();
    }

    KrViewProperties::ColumnType oldColumn = panel->view->properties()->sortColumn;
    KrViewProperties::ColumnType column = oldColumn;

    if (parameter[0].toLower() == "name") {
        column = KrViewProperties::Name;
    } else
        if (parameter[0].toLower() == "ext") {
            column = KrViewProperties::Ext;
        } else
            if (parameter[0].toLower() == "type") {
                column = KrViewProperties::Type;
            } else
                if (parameter[0].toLower() == "size") {
                    column = KrViewProperties::Size;
                } else
                    if (parameter[0].toLower() == "modified") {
                        column = KrViewProperties::Modified;
                    } else
                        if (parameter[0].toLower() == "perms") {
                            column = KrViewProperties::Permissions;
                        } else
                            if (parameter[0].toLower() == "rwx") {
                                column = KrViewProperties::KrPermissions;
                            } else
                                if (parameter[0].toLower() == "owner") {
                                    column = KrViewProperties::Owner;
                                } else
                                    if (parameter[0].toLower() == "group") {
                                        column = KrViewProperties::Group;
                                    } else {
                                        setError(exp, Error(Error::exp_S_WARNING, Error::exp_C_ARGUMENT, i18n("Expander: unknown column specified for %_ColSort(%1)%", parameter[0])));
                                        return QString();
                                    }

    bool descending = panel->view->properties()->sortOptions & KrViewProperties::Descending;

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

    panel->view->setSortMode(column, descending);

    return QString();  // this doesn't return anything, that's normal!
}

exp_PanelSize::exp_PanelSize()
{
    _expression = "PanelSize";
    _description = i18n("Set Relation Between the Panels...");
    _needPanel = true;

    addParameter(exp_parameter(i18n("Set the new size in percent:"), "__int:0;100;5;50", true));
}
TagString exp_PanelSize::expFunc(const KrPanel* panel, const QStringList& parameter, const bool&, Expander& exp) const
{
    NEED_PANEL
    int newSize;

    if (parameter.count() == 0 || parameter[0].isEmpty())
        newSize = 50; //default is 50%
    else
        newSize = parameter[0].toInt();

    if (newSize < 0 || newSize > 100) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: Value %1 out of range for %_PanelSize(percent)%. The first parameter has to be >0 and <100", newSize)));
        return QString();
    }

    QList<int> panelSizes = MAIN_VIEW->horiz_splitter->sizes();
    int totalSize = panelSizes[0] + panelSizes[1];

    if (panel == LEFT_PANEL) {
        panelSizes[0] = totalSize * newSize / 100;
        panelSizes[1] = totalSize * (100 - newSize) / 100;
    } else { // == RIGHT_PANEL
        panelSizes[0] = totalSize * (100 - newSize) / 100;
        panelSizes[1] = totalSize * newSize / 100;
    }

    MAIN_VIEW->horiz_splitter->setSizes(panelSizes);

    return QString();  // this doesn't return everything, that's normal!
}

#ifdef __KJSEMBED__
exp_Script::exp_Script()
{
    _expression = "Script";
    _description = i18n("Execute a JavaScript Extension...");
    _needPanel = false;

    addParameter(exp_parameter(i18n("Location of the script"), "", true));
    addParameter(exp_parameter(i18n("Set some variables for the execution (optional).\ni.e. \"return=return_var;foo=bar\", consult the handbook for more information"), "", false));
}
TagString exp_Script::expFunc(const KrPanel*, const QStringList& parameter, const bool&, Expander& exp) const
{
    if (parameter.count() == 0 || parameter[0].isEmpty()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: no script specified for %_Script(script)%")));
        return QString();
    }

    QString filename = parameter[0];
    if (filename.find('/') && KUrl::isRelativeUrl(filename)) {
        // this return the local version of the file if this exists. else the global one is returnd
        filename = locate("data", "krusader/js/" + filename);
    }

    if (! krJS)
        krJS = new KrJS();

    KJS::ExecState *exec = krJS->globalExec();

    QString jsReturn;
    if (parameter[1].toLower() == "yes")   // to stay compatible with the old-style parameter
        jsReturn = "cmd";
    else {
        QStringList jsVariables = parameter[1].split(';');
        QString jsVariable, jsValue;
        for (QStringList::Iterator it = jsVariables.begin(); it != jsVariables.end(); ++it) {
            jsVariable = (*it).section('=', 0, 0).trimmed();
            jsValue = (*it).section('=', 1);
            if (jsVariable == "return")
                jsReturn = jsValue.trimmed();
            else
                krJS->putValue(jsVariable, KJSEmbed::convertToValue(exec, jsValue));
        }
    }

    krJS->runFile(filename);

    if (! jsReturn.isEmpty())
        return krJS->getValue(jsReturn).toString(krJS->globalExec()).qstring();
    else
        return QString();
}
#endif

exp_View::exp_View()
{
    _expression = "View";
    _description = i18n("View File with Krusader's Internal Viewer...");
    _needPanel = false;

    addParameter(exp_parameter(i18n("Which file to view (normally '%aCurrent%'):"), "__placeholder", true));
    addParameter(exp_parameter(i18n("Choose a view mode:"), "__choose:generic;text;hex", false));
    //addParameter( exp_parameter( i18n("Choose a window-mode"), "__choose:tab;window;panel", false ) );
    //TODO: window-mode 'panel' should open the file in the third-hand viewer
    addParameter(exp_parameter(i18n("Choose a window mode:"), "__choose:tab;window", false));
}
TagString exp_View::expFunc(const KrPanel*, const QStringList& parameter, const bool&, Expander& exp) const
{
    if (parameter.count() == 0 || parameter[0].isEmpty()) {
        setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_ARGUMENT, i18n("Expander: no file to view in %_View(filename)%")));
        return QString();
    }

    QString viewMode, windowMode;
    if (parameter.count() <= 1 || parameter[1].isEmpty())
        viewMode = "generic";
    else
        viewMode = parameter[1];

    if (parameter.count() <= 2 || parameter[2].isEmpty())
        windowMode = "tab";
    else
        windowMode = parameter[2];

    KrViewer::Mode mode = KrViewer::Generic;
    if (viewMode == "text") mode = KrViewer::Text;
    else if (viewMode == "hex") mode = KrViewer::Hex;

    KrViewer::view(parameter[0], mode, (windowMode == "window"));
    //TODO: Call the viewer with viewMode and windowMode. Filename is in parameter[0].
    // It would be nice if parameter[0] could also be a space-separated filename-list (provided if the first parameter is %aList(selected)%)

    return QString();  // this doesn't return everything, that's normal!
}

/////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// end of expander classes ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

TagString exp_simpleplaceholder::expFunc(const KrPanel* p, const TagStringList& parameter, const bool& useUrl, Expander& exp) const
{
    QStringList lst;
    for (TagStringList::const_iterator it = parameter.begin(), end = parameter.end();it != end;++it)
        if ((*it).isSimple())
            lst.push_back((*it).string());
        else {
            setError(exp, Error(Error::exp_S_FATAL, Error::exp_C_SYNTAX, i18n("%Each% is not allowed in parameter to %1", description())));
            return QString();
        }
    return expFunc(p, lst, useUrl, exp);
}

}

KrPanel* Expander::getPanel(const char panelIndicator, const exp_placeholder* pl, Expander& exp)
{
    switch (panelIndicator) {
    case 'a':
        return ACTIVE_PANEL;
    case 'o':
        return OTHER_PANEL;
    case 'l':
        return LEFT_PANEL;
    case 'r':
        return RIGHT_PANEL;
    case '_':
        return 0;
    default:
        exp.setError(Error(Error::exp_S_FATAL, Error::exp_C_SYNTAX, i18n("Expander: Bad panel specifier %1 in placeholder %2", panelIndicator, pl->description())));
        return 0;
    }
}

void Expander::expand(const QString& stringToExpand, bool useUrl)
{
    TagString result = expandCurrent(stringToExpand, useUrl);
    if (error())
        return;

    if (!result.isSimple())
        resultList = splitEach(result);
    else
        resultList.append(result.string());

//    krOut << resultList[0] << endl;
}

TagString Expander::expandCurrent(const QString& stringToExpand, bool useUrl)
{
    TagString result;
    QString exp;
    TagString tmpResult;
    int begin, end, i;
//    int brackets = 0;
//    bool inQuotes = false;
    int idx = 0;
    while (idx < stringToExpand.length()) {
        if ((begin = stringToExpand.indexOf('%', idx)) == -1) break;
        if ((end = findEnd(stringToExpand, begin)) == -1) {
            // xgettext:no-c-format
            setError(Error(Error::exp_S_FATAL, Error::exp_C_SYNTAX, i18n("Error: unterminated % in Expander")));
            return QString();
        }

        result += stringToExpand.mid(idx, begin - idx);   // copy until the start of %exp%

        // get the expression, and expand it using the correct expander function
        exp = stringToExpand.mid(begin + 1, end - begin - 1);
//       kDebug() << "------------- exp: '" << exp << "'" << endl;
        if (exp.isEmpty())
            result += QString(QChar('%'));
        else {
            TagStringList parameter = separateParameter(&exp, useUrl);
            if (error())
                return QString();
            char panelIndicator = exp.toLower()[0].toLatin1();
            exp.replace(0, 1, "");
            for (i = 0; i < placeholderCount(); ++i)
                if (exp == placeholder(i)->expression()) {
//               kDebug() << "---------------------------------------" << endl;
                    tmpResult = placeholder(i)->expFunc(getPanel(panelIndicator, placeholder(i), *this), parameter, useUrl, *this);
                    if (error()) {
                        return QString();
                    } else
                        result += tmpResult;
//               kDebug() << "---------------------------------------" << endl;
                    break;
                }
            if (i == placeholderCount()) {   // didn't find an expander
                setError(Error(Error::exp_S_FATAL, Error::exp_C_SYNTAX, i18n("Error: unrecognized %%%1%2%% in Expander", panelIndicator, exp)));
                return QString();
            }
        } //else
        idx = end + 1;
    }
    // copy the rest of the string
    result += stringToExpand.mid(idx);
//    kDebug() << "============== result '" << result << "'" << endl;
    return result;
}

QStringList Expander::splitEach(TagString stringToSplit)
{
    if (stringToSplit.isSimple()) {
//   krOut << stringToSplit.string() << endl;
        QStringList l;
        l << stringToSplit.string();
        return l;
    }
    pair<uint, QStringList> pl = *stringToSplit.tagsBegin();
    stringToSplit.eraseTag(stringToSplit.tagsBegin());
    QStringList ret;
    for (QStringList::const_iterator it = pl.second.constBegin(), end = pl.second.constEnd();it != end;++it) {
        TagString s = stringToSplit;
        s.insert(pl.first, *it);
        ret += splitEach(s);
    }
    return ret;
//    kDebug() << "stringToSplit: " << stringToSplit << endl;
}

TagStringList Expander::separateParameter(QString* const exp, bool useUrl)
{
    TagStringList parameter;
    QStringList parameter1;
    QString result;
    int begin, end;
    if ((begin = exp->indexOf('(')) != -1) {
        if ((end = exp->lastIndexOf(')')) == -1) {
            setError(Error(Error::exp_S_FATAL, Error::exp_C_SYNTAX, i18n("Error: missing ')' in Expander")));
            return TagStringList();
        }
        result = exp->mid(begin + 1, end - begin - 1);
        *exp = exp->left(begin);

        bool inQuotes = false;
        int idx = 0;
        begin = 0;
        while (idx < result.length()) {
            if (result[ idx ].toLatin1() == '\\') {
                if (result[ idx+1 ].toLatin1() == '"')
                    result.replace(idx, 1, "");
            }
            if (result[ idx ].toLatin1() == '"')
                inQuotes = !inQuotes;
            if (result[ idx ].toLatin1() == ',' && !inQuotes) {
                parameter1.append(result.mid(begin, idx - begin));
                begin = idx + 1;
//             krOut << " ---- parameter: " << parameter.join(";") << endl;
            }
            idx++;
        }
        parameter1.append(result.mid(begin, idx - begin));     //don't forget the last one

        for (QStringList::Iterator it = parameter1.begin(); it != parameter1.end(); ++it) {
            *it = (*it).trimmed();
            if ((*it).left(1) == "\"")
                *it = (*it).mid(1, (*it).length() - 2);
            parameter.push_back(expandCurrent(*it, useUrl));
            if (error())
                return TagStringList();
        }
    }

//    krOut << "------- exp: " << *exp << " ---- parameter: " << parameter.join(";") << endl;
    return parameter;
}

int Expander::findEnd(const QString& str, int start)
{
    int end = str.indexOf('%', start + 1);
    if (end == -1)
        return end;
    int bracket = str.indexOf('(', start + 1);
    if (end < bracket || bracket == -1)
        return end;

    int idx = bracket + 1;
    bool inQuotes = false;
    int depth = 1;
    while (idx < str.length()) {
        switch (str[ idx ].toLatin1()) {
        case '\\':
            idx ++;
            break;
        case '"':
            inQuotes = !inQuotes;
            break;
        case '(':
            if (!inQuotes)
                depth++;
            break;
        case ')':
            if (!inQuotes)
                --depth;
            break;
        case '%':
            if (depth == 0)
                return idx;
        } //switch
        idx++;
    } //while
    // failsafe
    return -1;
}
