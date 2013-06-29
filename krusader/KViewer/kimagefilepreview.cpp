/*
* This file is part of the KDE project
* Copyright (C) 2001 Martin R. Jones <mjones@kde.org>
*               2001 Carsten Pfeiffer <pfeiffer@kde.org>
*
* You can Freely distribute this program under the GNU Library General Public
* License. See the file "COPYING" for the exact licensing terms.
*/

#include "kimagefilepreview.h"

#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtCore/QTimer>
#include <QPixmap>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QFrame>

#include <kapplication.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kio/previewjob.h>

KrusaderImageFilePreview::KrusaderImageFilePreview(QWidget *parent)
        : KPreviewWidgetBase(parent),
        m_job(0L)
{
    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setContentsMargins(KDialog::marginHint(), KDialog::marginHint(), KDialog::marginHint(), KDialog::marginHint());

    imageLabel = new QLabel(this);
    imageLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    imageLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    imageLabel->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored));
    vb->addWidget(imageLabel, 1);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(showPreview()));

    setSupportedMimeTypes(KIO::PreviewJob::supportedMimeTypes());
}

KrusaderImageFilePreview::~KrusaderImageFilePreview()
{
    if (m_job)
        m_job->kill(KJob::EmitResult);
}

void KrusaderImageFilePreview::showPreview()
{
    // Pass a copy since clearPreview() will clear currentURL
    KUrl url = currentURL;
    showPreview(url, true);
}

// called via KPreviewWidgetBase interface
void KrusaderImageFilePreview::showPreview(const KUrl& url)
{
    showPreview(url, false);
}

void KrusaderImageFilePreview::showPreview(const KUrl &url, bool force)
{
    if (!url.isValid()) {
        clearPreview();
        return ;
    }

    if (url != currentURL || force) {
        clearPreview();
        currentURL = url;

        int w = imageLabel->contentsRect().width() - 4;
        int h = imageLabel->contentsRect().height() - 4;

        m_job = createJob(url, w, h);
        connect(m_job, SIGNAL(result(KJob *)),
                this, SLOT(slotResult(KJob *)));
        connect(m_job, SIGNAL(gotPreview(const KFileItem&,
                                         const QPixmap&)),
                SLOT(gotPreview(const KFileItem&, const QPixmap&)));

        connect(m_job, SIGNAL(failed(const KFileItem&)),
                this, SLOT(slotFailed(const KFileItem&)));
    }
}

void KrusaderImageFilePreview::resizeEvent(QResizeEvent *)
{
    timer->setSingleShot(true);
    timer->start(100);   // forces a new preview
}

QSize KrusaderImageFilePreview::sizeHint() const
{
    return QSize(20, 200);   // otherwise it ends up huge???
}

KIO::PreviewJob * KrusaderImageFilePreview::createJob(const KUrl& url, int w, int h)
{
    KFileItemList fileItemList;
    fileItemList.append(KFileItem(KFileItem::Unknown,KFileItem::Unknown, url, true));
    QStringList allPlugins = KIO::PreviewJob::availablePlugins();
    KIO::PreviewJob * job = new KIO::PreviewJob(fileItemList, QSize(w, h), &(allPlugins));
        job->setOverlayIconAlpha(0);
        job->setOverlayIconSize(0);
        job->setScaleType(KIO::PreviewJob::Scaled);
    return job;
}

void KrusaderImageFilePreview::gotPreview(const KFileItem& item, const QPixmap& pm)
{
    if (item.url() == currentURL)     // should always be the case
        imageLabel->setPixmap(pm);
}

void KrusaderImageFilePreview::slotFailed(const KFileItem& item)
{
    if (item.isDir())
        imageLabel->clear();
    else if (item.url() == currentURL) // should always be the case
        imageLabel->setPixmap(SmallIcon("image-missing", KIconLoader::SizeLarge,
                                        KIconLoader::DisabledState));
}

void KrusaderImageFilePreview::slotResult(KJob *job)
{
    if (job == m_job)
        m_job = 0L;
}

void KrusaderImageFilePreview::clearPreview()
{
    if (m_job) {
        m_job->kill(KJob::EmitResult);
        m_job = 0L;
    }

    imageLabel->clear();
    currentURL = KUrl();
}

#include "kimagefilepreview.moc"
