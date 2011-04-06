/*****************************************************************************
 * Copyright (C) 2002 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2002 Rafi Yanai <yanai@users.sourceforge.net>               *
 * Copyright (C) 2010 Jan Lepper <dehtris@yahoo.de>                          *
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

#include "krinterview.h"
#include "krvfsmodel.h"
#include "krinterviewitem.h"
#include "krcolorcache.h"
#include "krmousehandler.h"
#include "krpreviews.h"
#include "../VFS/vfilecontainer.h"

KrInterView::KrInterView(KrViewInstance &instance, KConfig *cfg,
                         QAbstractItemView *itemView) :
        KrView(instance, cfg), _itemView(itemView), _mouseHandler(0)
{
    _model = new KrVfsModel(this);
    _mouseHandler = new KrMouseHandler(this);
}

KrInterView::~KrInterView()
{
    // any references to the model should be cleared ar this point,
    // but sometimes for some reason it is still referenced by
    // QPersistentModelIndex instances held by QAbstractItemView and/or QItemSelectionModel(child object) -
    // so schedule _model for later deletion
    _model->clear();
    _model->deleteLater();
    _model = 0;
    delete _mouseHandler;
    _mouseHandler = 0;
    QHashIterator< vfile *, KrInterViewItem *> it(_itemHash);
    while (it.hasNext())
        delete it.next().value();
    _itemHash.clear();
}

void KrInterView::selectRegion(KrViewItem *i1, KrViewItem *i2, bool select)
{
    vfile* vf1 = (vfile *)i1->getVfile();
    QModelIndex mi1 = _model->vfileIndex(vf1);
    vfile* vf2 = (vfile *)i2->getVfile();
    QModelIndex mi2 = _model->vfileIndex(vf2);

    if (mi1.isValid() && mi2.isValid()) {
        int r1 = mi1.row();
        int r2 = mi2.row();

        if (r1 > r2) {
            int t = r1;
            r1 = r2;
            r2 = t;
        }

        op()->setMassSelectionUpdate(true);
        for (int row = r1; row <= r2; row++)
            setSelected(_model->vfileAt(_model->index(row, 0)), select);
        op()->setMassSelectionUpdate(false);

        redraw();

    } else if (mi1.isValid() && !mi2.isValid())
        i1->setSelected(select);
    else if (mi2.isValid() && !mi1.isValid())
        i2->setSelected(select);
}

void KrInterView::selectRegion(KUrl item1, KUrl item2, bool select)
{
    QModelIndex mi1 = _model->indexFromUrl(item1);
    QModelIndex mi2 = _model->indexFromUrl(item2);

    if (mi1.isValid() && mi2.isValid()) {
        int r1 = mi1.row();
        int r2 = mi2.row();

        if (r1 > r2) {
            int t = r1;
            r1 = r2;
            r2 = t;
        }

        op()->setMassSelectionUpdate(true);
        for (int row = r1; row <= r2; row++)
            setSelected(_model->vfileAt(_model->index(row, 0)), select);
        op()->setMassSelectionUpdate(false);

        redraw();

    } else if (mi1.isValid() && !mi2.isValid())
        setSelected(_model->vfileAt(mi1), select);
    else if (mi2.isValid() && !mi1.isValid())
        setSelected(_model->vfileAt(mi1), (select));
}

void KrInterView::intSetSelected(const vfile* vf, bool select)
{
    if(select)
        _selection.insert(vf);
    else
        _selection.remove(vf);
}

bool KrInterView::isSelected(const QModelIndex &ndx)
{
    return isSelected(_model->vfileAt(ndx));
}

KrViewItem* KrInterView::findItemByName(const QString &name)
{
    if (!_model->ready())
        return 0;

    QModelIndex ndx = _model->nameIndex(name);
    if (!ndx.isValid())
        return 0;
    return getKrInterViewItem(ndx);
}

QString KrInterView::getCurrentItem() const
{
    if (!_model->ready())
        return QString();

    vfile *vf = _model->vfileAt(_itemView->currentIndex());
    if (vf == 0)
        return QString();
    return vf->vfile_getName();
}

KrViewItem* KrInterView::getCurrentKrViewItem()
{
    if (!_model->ready())
        return 0;

    return getKrInterViewItem(_itemView->currentIndex());
}

KrViewItem* KrInterView::getFirst()
{
    if (!_model->ready())
        return 0;

    return getKrInterViewItem(_model->index(0, 0, QModelIndex()));
}

KrViewItem* KrInterView::getLast()
{
    if (!_model->ready())
        return 0;

    return getKrInterViewItem(_model->index(_model->rowCount() - 1, 0, QModelIndex()));
}

KrViewItem* KrInterView::getNext(KrViewItem *current)
{
    vfile* vf = (vfile *)current->getVfile();
    QModelIndex ndx = _model->vfileIndex(vf);
    if (ndx.row() >= _model->rowCount() - 1)
        return 0;
    return getKrInterViewItem(_model->index(ndx.row() + 1, 0, QModelIndex()));
}

KrViewItem* KrInterView::getPrev(KrViewItem *current)
{
    vfile* vf = (vfile *)current->getVfile();
    QModelIndex ndx = _model->vfileIndex(vf);
    if (ndx.row() <= 0)
        return 0;
    return getKrInterViewItem(_model->index(ndx.row() - 1, 0, QModelIndex()));
}

KrViewItem* KrInterView::getKrViewItemAt(const QPoint &vp)
{
    if (!_model->ready())
        return 0;

    return getKrInterViewItem(_itemView->indexAt(vp));
}

KrViewItem *KrInterView::findItemByVfile(vfile *vf) {
    return getKrInterViewItem(vf);
}

KrInterViewItem * KrInterView::getKrInterViewItem(vfile *vf)
{
    QHash<vfile *, KrInterViewItem*>::iterator it = _itemHash.find(vf);
    if (it == _itemHash.end()) {
        KrInterViewItem * newItem =  new KrInterViewItem(this, vf);
        _itemHash[ vf ] = newItem;
        return newItem;
    }
    return *it;
}

KrInterViewItem * KrInterView::getKrInterViewItem(const QModelIndex & ndx)
{
    if (!ndx.isValid())
        return 0;
    vfile * vf = _model->vfileAt(ndx);
    if (vf == 0)
        return 0;
    else
        return getKrInterViewItem(vf);
}

void KrInterView::makeCurrentVisible()
{
    _itemView->scrollTo(_itemView->currentIndex());
}

void KrInterView::makeItemVisible(const KrViewItem *item)
{
    if (item == 0)
        return;
    vfile* vf = (vfile *)item->getVfile();
    QModelIndex ndx = _model->vfileIndex(vf);
    if (ndx.isValid())
        _itemView->scrollTo(ndx);
}

void KrInterView::setCurrentItem(const QString& name)
{
    setCurrentIndex(_model->nameIndex(name));
}

void KrInterView::setCurrentKrViewItem(KrViewItem *item)
{
    if (item == 0) {
        setCurrentIndex(QModelIndex());
        return;
    }
    vfile* vf = (vfile *)item->getVfile();
    setCurrentIndex(_model->vfileIndex(vf));
}

void KrInterView::sort()
{
    _model->sort();
}

void KrInterView::clear()
{
    _selection.clear();
    _itemView->clearSelection();
    _itemView->setCurrentIndex(QModelIndex());
    _model->clear();
    QHashIterator< vfile *, KrInterViewItem *> it(_itemHash);
    while (it.hasNext())
        delete it.next().value();
    _itemHash.clear();

    KrView::clear();
}

void KrInterView::populate(const QList<vfile*> &vfiles, vfile *dummy)
{
    _model->populate(vfiles, dummy);
    _itemView->setCurrentIndex(_model->index(0, 0));
}

KrViewItem* KrInterView::preAddItem(vfile *vf)
{
    QModelIndex idx = _model->addItem(vf);
    if(_model->rowCount() == 1) // if this is the fist item to be added, make it current
        _itemView->setCurrentIndex(idx);
    return getKrInterViewItem(idx);
}

void KrInterView::preDelItem(KrViewItem *item)
{
    setSelected(item->getVfile(), false);
    QModelIndex ndx = _model->removeItem((vfile *)item->getVfile());
    if (ndx.isValid())
        _itemView->setCurrentIndex(ndx);
    _itemHash.remove((vfile *)item->getVfile());
}

void KrInterView::preUpdateItem(vfile *vf)
{
    _model->updateItem(vf);
}

void KrInterView::prepareForActive()
{
    KrView::prepareForActive();
    _itemView->setFocus();
    KrViewItem * current = getCurrentKrViewItem();
    if (current != 0) {
        QString desc = current->description();
        op()->emitItemDescription(desc);
    }
}

void KrInterView::prepareForPassive()
{
    KrView::prepareForPassive();
    _mouseHandler->cancelTwoClickRename();
    //if ( renameLineEdit() ->isVisible() )
    //renameLineEdit() ->clearFocus();
}

void KrInterView::redraw()
{
    _itemView->viewport()->update();
}

void KrInterView::refreshColors()
{
    QPalette p(_itemView->palette());
    KrColorGroup cg;
    KrColorCache::getColorCache().getColors(cg, KrColorItemType(KrColorItemType::File,
        false, _focused, false, false));
    p.setColor(QPalette::Text, cg.text());
    p.setColor(QPalette::Base, cg.background());
    _itemView->setPalette(p);
    redraw();
}

void KrInterView::showContextMenu()
{
    showContextMenu(_itemView->viewport()->mapToGlobal(QPoint(0,0)));
}

void KrInterView::sortModeUpdated(int column, Qt::SortOrder order)
{
    KrView::sortModeUpdated(static_cast<KrViewProperties::ColumnType>(column),
                            order == Qt::DescendingOrder);
}

KIO::filesize_t KrInterView::calcSize()
{
    KIO::filesize_t size = 0;
    foreach(vfile *vf, _model->vfiles()) {
        size += vf->vfile_getSize();
    }
    return size;
}

KIO::filesize_t KrInterView::calcSelectedSize()
{
    KIO::filesize_t size = 0;
    foreach(const vfile *vf, _selection) {
        size += vf->vfile_getSize();
    }
    return size;
}

FileItemList KrInterView::getItems(KRQuery mask, bool dirs, bool files)
{
    FileItemList list;
    foreach(vfile *vf, _model->vfiles()) {
        if (vf != _dummyVfile)
            if ((!vf->vfile_isDir() && files) || (vf->vfile_isDir() && dirs))
                if(mask.isNull() || mask.match(vf))
                    list << vf->toFileItem();
    }
    return list;
}

FileItemList KrInterView::getSelectedItems(bool currentIfNoSelection)
{
    FileItemList list;

    if(_selection.count()) {
        foreach(const vfile *vf, _selection)
            list << vf->toFileItem();
    } else if(currentIfNoSelection) {
        vfile *vf = _model->vfileAt(_itemView->currentIndex());
        if (vf && vf != _dummyVfile)
            list << vf->toFileItem();
    }

    return list;
}

void KrInterView::makeItemVisible(KUrl url)
{
    QModelIndex ndx = _model->indexFromUrl(url);
    if (ndx.isValid())
        _itemView->scrollTo(ndx);
}

FileItem KrInterView::currentItem()
{
    vfile *vf = _model->vfileAt(_itemView->currentIndex());
    if(vf && vf != _dummyVfile)
        return vf->toFileItem();
    else
        return FileItem();
}

bool KrInterView::currentItemIsUpUrl()
{
    return _model->vfileAt(_itemView->currentIndex()) == _dummyVfile;
}

void KrInterView::currentChanged(const QModelIndex &current)
{
    vfile *vf =_model->vfileAt(_itemView->currentIndex());
    FileItem item = vf ? vf->toFileItem() : FileItem();
    op()->emitCurrentChanged(item);
}

QRect KrInterView::itemRect(KUrl itemUrl)
{
    vfile *vf = _model->vfileAt(_model->indexFromUrl(itemUrl));
    return vf ? itemRect(vf) : QRect();
}

FileItem KrInterView::itemAt(const QPoint &vp, bool *isUpUrl)
{
    vfile *vf = _model->vfileAt(_itemView->indexAt(vp));
    if(isUpUrl)
        *isUpUrl = (vf && (vf == _dummyVfile));
    return (vf && (vf != _dummyVfile)) ? vf->toFileItem() : FileItem();
}

void KrInterView::setCurrentItem(KUrl url)
{
    setCurrentIndex(_model->indexFromUrl(url));
}

void KrInterView::setCurrentIndex(QModelIndex index)
{
    if((index.isValid() && index.row() != _itemView->currentIndex().row()) ||
            !index.isValid()) {
        _mouseHandler->cancelTwoClickRename();
        _itemView->setCurrentIndex(index);
    }
}

void KrInterView::changeSelection(KUrl::List urls, bool select, bool clearFirst)
{
    op()->setMassSelectionUpdate(true);

    if(clearFirst)
        _selection.clear();

    foreach(const KUrl &url, urls) {
        QModelIndex idx = _model->indexFromUrl(url);
        if(idx.isValid())
            setSelected(_model->vfileAt(idx), select);
    }

    op()->setMassSelectionUpdate(false);

    redraw();
}

bool KrInterView::isItemSelected(KUrl url)
{
    vfile *vf = _model->vfileAt(_model->indexFromUrl(url));
    return vf ? _selection.contains(vf) : false;
}

vfile *KrInterView::vfileFromUrl(KUrl url)
{
    return _model->vfileAt(_model->indexFromUrl(url));
}

void KrInterView::updateItemSize(KUrl url, KIO::filesize_t newSize)
{
    vfile *vf = vfileFromUrl(url);
    if(vf) {
        vf->vfile_setSize(newSize);
        _itemView->viewport()->update(itemRect(vf));
    }
}

void KrInterView::setCurrentItem(ItemSpec item)
{
    QModelIndex newIndex, currentIndex = _itemView->currentIndex();
    switch (item) {
        case First:
            newIndex = _model->index(0, 0, QModelIndex());
            break;
        case Last:
            newIndex = _model->index(_model->rowCount() - 1, 0, QModelIndex());
            break;
        case Prev:
            if (currentIndex.row() <= 0)
                return;
            newIndex = _model->index(currentIndex.row() - 1, 0, QModelIndex());
            break;
        case Next:
            if (currentIndex.row() >= _model->rowCount() - 1)
                return;
            newIndex = _model->index(currentIndex.row() + 1, 0, QModelIndex());
            break;
        case UpUrl:
            if(!_dummyVfile)
                return;
            newIndex = _model->vfileIndex(_dummyVfile);
            break;
        default:
            return;
    }
    setCurrentIndex(newIndex);
    _itemView->scrollTo(newIndex);
}

bool KrInterView::isItemVisible(KUrl url)
{
    return _itemView->viewport()->rect().intersects(itemRect(url));
}

QRect KrInterView::itemRectGlobal(KUrl url)
{
    QRect rect = itemRect(url);
    rect.moveTo(_itemView->viewport()->mapToGlobal(rect.topLeft()));
    return rect;
}
