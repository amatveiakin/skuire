/*****************************************************************************
 * Copyright (C) 2003 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2003 Rafi Yanai <yanai@users.sourceforge.net>               *
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

#ifndef PANELPOPUP_H
#define PANELPOPUP_H

#include <qwidget.h>
#include <qstackedwidget.h>
#include <QtGui/QPixmap>
#include <QDropEvent>
#include <QLabel>
#include <QtCore/QPointer>
#include <qtreeview.h>

#include <kfileitem.h>
#include <kio/previewjob.h>
#include <kurl.h>

class QButtonGroup;
class QLabel;
class QSplitter;
class QToolButton;
class KrSqueezedTextLabel;
class KLineEdit;
class KComboBox;
class KrusaderImageFilePreview;
class PanelViewer;
class DiskUsageViewer;
class KrFileTreeView;
class KDirModel;
class KDirSortFilterProxyModel;
class QPoint;
class vfile;
class FileManagerWindow;

class PanelPopup: public QWidget
{
    Q_OBJECT

    enum Parts { Tree, Preview, QuickPanel, View, DskUsage, Last = 0xFF };
public:
    PanelPopup(QSplitter *splitter, bool left, FileManagerWindow *mainWindow);
    ~PanelPopup();
    inline int currentPage() const {
        return stack->currentWidget()->property("KrusaderWidgetId").toInt();
    }

    void saveSizes();

public slots:
    void update(KFileItem item);
    void show();
    void hide();

signals:
    void selection(const KUrl &url);
    void hideMe();

protected slots:
    void tabSelected(int id);
    void treeSelection();
    void handleOpenUrlRequest(const KUrl &url);
    void quickSelect();
    void quickSelect(const QString &);
    void quickSelectStore();

protected:
    virtual void focusInEvent(QFocusEvent*);

    bool _left;
    bool _hidden;
    FileManagerWindow *_mainWindow;
    QStackedWidget *stack;
    KrusaderImageFilePreview *viewer;
    KrSqueezedTextLabel *dataLine;
    QPointer<KIO::PreviewJob> pjob;
    KrFileTreeView *tree;
    QToolButton *treeBtn, *previewBtn, *quickBtn, *viewerBtn, *duBtn;
    QButtonGroup *btns;
    KLineEdit *quickFilter;
    KComboBox *quickSelectCombo;
    PanelViewer *panelviewer;
    DiskUsageViewer *diskusage;
    QWidget *quickPanel;
    QList<int> splitterSizes;
    QSplitter *splitter;
};


class KrFileTreeView : public QTreeView
{
    friend class KrDirModel;
    Q_OBJECT

public:
    KrFileTreeView(QWidget *parent = 0);
    virtual ~KrFileTreeView() {}

    KUrl currentUrl() const;
    KUrl selectedUrl() const;
    KUrl::List selectedUrls() const;
    KUrl rootUrl() const;

public Q_SLOTS:
    void setDirOnlyMode(bool enabled);
    void setShowHiddenFiles(bool enabled);
    void setCurrentUrl(const KUrl &url);
    void setRootUrl(const KUrl &url);

Q_SIGNALS:
    void activated(const KUrl &url);
    void currentChanged(const KUrl &url);

private Q_SLOTS:
    void slotActivated(const QModelIndex&);
    void slotCurrentChanged(const QModelIndex&, const QModelIndex&);
    void slotExpanded(const QModelIndex&);

private:
    KUrl urlForProxyIndex(const QModelIndex &index) const;
    void dropMimeData(const KUrl::List & lst, const KUrl & url, const QModelIndex & ind);

    KDirModel *mSourceModel;
    KDirSortFilterProxyModel *mProxyModel;
};

#endif
