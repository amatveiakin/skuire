/*****************************************************************************
 * Copyright (C) 2003-2004 Max Howell <max.howell@methylblue.com>            *
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

#include "fileTree.h"
#include "radialMap.h"   //class Segment
#include "widget.h"

#include <kcursor.h>     //::mouseMoveEvent()
#include <kiconeffect.h> //::mousePressEvent()
#include <kiconloader.h> //::mousePressEvent()
#include <kio/job.h>     //::mousePressEvent()
#include <kio/deletejob.h>
#include <kio/jobuidelegate.h>
#include <klocale.h>
#include <kmessagebox.h> //::mousePressEvent()
#include <kmenu.h>  //::mousePressEvent()
#include <krun.h>        //::mousePressEvent()
#include <math.h>        //::segmentAt()
#include <QApplication>//QApplication::setOverrideCursor()
#include <QPainter>
#include <QTimer>      //::resizeEvent()
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>



void
RadialMap::Widget::resizeEvent(QResizeEvent*)
{
    if (m_map.resize(rect())) {
        m_timer.setSingleShot(true);
        m_timer.start(500);   //will cause signature to rebuild for new size
    }

    //always do these as they need to be initialised on creation
    m_offset.rx() = (width() - m_map.width()) / 2;
    m_offset.ry() = (height() - m_map.height()) / 2;
}

void
RadialMap::Widget::paintEvent(QPaintEvent*)
{
    //bltBit for some Qt setups will bitBlt _after_ the labels are painted. Which buggers things up!
    //shame as bitBlt is faster, possibly Qt bug? Should report the bug? - seems to be race condition
    //bitBlt( this, m_offset, &m_map );

    QPainter paint(this);

    paint.drawPixmap(m_offset, m_map);

    //vertical strips
    if (m_map.width() < width()) {
        paint.eraseRect(0, 0, m_offset.x(), height());
        paint.eraseRect(m_map.width() + m_offset.x(), 0, m_offset.x() + 1, height());
    }
    //horizontal strips
    if (m_map.height() < height()) {
        paint.eraseRect(0, 0, width(), m_offset.y());
        paint.eraseRect(0, m_map.height() + m_offset.y(), width(), m_offset.y() + 1);
    }

    //exploded labels
    if (!m_map.isNull() && !m_timer.isActive())
        paintExplodedLabels(paint);
}

const RadialMap::Segment*
RadialMap::Widget::segmentAt(QPoint &e) const
{
    //determine which segment QPoint e is above

    e -= m_offset;

    if (e.x() <= m_map.width() && e.y() <= m_map.height()) {
        //transform to cartesian coords
        e.rx() -= m_map.width() / 2; //should be an int
        e.ry()  = m_map.height() / 2 - e.y();

        double length = hypot(e.x(), e.y());

        if (length >= m_map.m_innerRadius) { //not hovering over inner circle
            uint depth  = ((int)length - m_map.m_innerRadius) / m_map.m_ringBreadth;

            if (depth <= m_map.m_visibleDepth) { //**** do earlier since you can //** check not outside of range
                //vector calculation, reduces to simple trigonometry
                //cos angle = (aibi + ajbj) / albl
                //ai = x, bi=1, aj=y, bj=0
                //cos angle = x / (length)

                uint a  = (uint)(acos((double)e.x() / length) * 916.736);    //916.7324722 = #radians in circle * 16

                //acos only understands 0-180 degrees
                if (e.y() < 0) a = 5760 - a;

#define ring (m_map.m_signature + depth)
                for (ConstIterator<Segment> it = ring->constIterator(); it != ring->end(); ++it)
                    if ((*it)->intersects(a))
                        return *it;
#undef ring
            }
        } else return m_rootSegment; //hovering over inner circle
    }

    return 0;
}

void
RadialMap::Widget::mouseMoveEvent(QMouseEvent *e)
{
    //set m_focus to what we hover over, update UI if it's a new segment

    Segment const * const oldFocus = m_focus;
    QPoint p = e->pos();

    m_focus = segmentAt(p);   //NOTE p is passed by non-const reference

    if (m_focus && m_focus->file() != m_tree) {
        if (m_focus != oldFocus) { //if not same as last time
            setCursor(QCursor(Qt::PointingHandCursor));
            m_tip.updateTip(m_focus->file(), m_tree);
            emit mouseHover(m_focus->file()->fullPath());

            //repaint required to update labels now before transparency is generated
            repaint();
        }

        // updates tooltip pseudo-tranparent background
        m_tip.moveto(e->globalPos(), *this, (p.y() < 0));
    } else if (oldFocus && oldFocus->file() != m_tree) {
        unsetCursor();
        m_tip.hide();
        update();

        emit mouseHover(QString());
    }
}

void
RadialMap::Widget::mousePressEvent(QMouseEvent *e)
{
    //m_tip is hidden already by event filter
    //m_focus is set correctly (I've been strict, I assure you it is correct!)

    if (m_focus && !m_focus->isFake()) {
        const KUrl url   = Widget::url(m_focus->file());
        const bool isDir = m_focus->file()->isDir();

        if (e->button() == Qt::RightButton) {
            KMenu popup;
            popup.setTitle(m_focus->file()->fullPath(m_tree));

            QAction * actKonq = 0, * actKonsole = 0, *actViewMag = 0, * actFileOpen = 0, * actEditDel = 0;

            if (isDir) {
                actKonq = popup.addAction(KIcon("konqueror"), i18n("Open &Konqueror Here"));
                if (url.protocol() == "file")
                    actKonsole = popup.addAction(KIcon("konsole"), i18n("Open &Konsole Here"));

                if (m_focus->file() != m_tree) {
                    popup.addSeparator();
                    actViewMag = popup.addAction(KIcon("zoom-original"), i18n("&Center Map Here"));
                }
            } else
                actFileOpen = popup.addAction(KIcon("document-open"), i18n("&Open"));

            popup.addSeparator();
            actEditDel = popup.addAction(KIcon("edit-delete"), i18n("&Delete"));

            QAction * result = popup.exec(e->globalPos());
            if (result == 0)
                result = (QAction *) - 1;  // sanity

            if (result == actKonq)
                //KRun::runCommand will show an error message if there was trouble
                KRun::runCommand(QString("kfmclient openURL '%1'").arg(url.url()), this);
            else if (result == actKonsole)
                KRun::runCommand(QString("konsole --workdir '%1'").arg(url.url()), this);
            else if (result == actViewMag || result == actFileOpen)
                goto sectionTwo;
            else if (result == actEditDel) {
                const KUrl url = Widget::url(m_focus->file());
                const QString message = (m_focus->file()->isDir()
                                         ? i18n("<qt>The directory at <i>'%1'</i> will be <b>recursively</b> and <b>permanently</b> deleted.</qt>", url.prettyUrl())
                                         : i18n("<qt><i>'%1'</i> will be <b>permanently</b> deleted.</qt>", url.prettyUrl()));
                const int userIntention = KMessageBox::warningContinueCancel(this, message, QString(), KGuiItem(i18n("&Delete"), "edit-delete"));

                if (userIntention == KMessageBox::Continue) {
                    KIO::Job *job = KIO::del(url);
                    job->ui()->setWindow(this);
                    connect(job, SIGNAL(result(KJob*)), SLOT(deleteJobFinished(KJob*)));
                    QApplication::setOverrideCursor(Qt::BusyCursor);
                }
            } else
                //ensure m_focus is set for new mouse position
                sendFakeMouseEvent();

        } else {

        sectionTwo:

            const QRect rect(e->x() - 20, e->y() - 20, 40, 40);

            m_tip.hide(); //user expects this

            if (!isDir || e->button() == Qt::MidButton) {
#if 0 // TODO: PORTME
                KIconEffect::visualActivate(this, rect);
#endif
                new KRun(url, this, true);   //FIXME see above
            } else if (m_focus->file() != m_tree) { //is left mouse button
#if 0 // TODO: PORTME
                KIconEffect::visualActivate(this, rect);
#endif
                emit activated(url);   //activate first, this will cause UI to prepare itself
                if (m_focus)
                    createFromCache((Directory *)m_focus->file());
            }
        }
    }
}

void
RadialMap::Widget::deleteJobFinished(KJob *job)
{
    QApplication::restoreOverrideCursor();
    if (!job->error())
        invalidate();
    else
        job->uiDelegate()->showErrorMessage();
}
