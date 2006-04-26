/***************************************************************************
                   synchronizerdirlist.cpp  -  description
                             -------------------
    copyright            : (C) 2006 + by Csaba Karai
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

                                                     H e a d e r    F i l e

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "synchronizerdirlist.h"
#include "../VFS/vfs.h"
#include "../VFS/krpermhandler.h"
#include <dirent.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kfileitem.h>
#include <klargefile.h>
#include <qapplication.h>

SynchronizerDirList::SynchronizerDirList( QWidget *w ) : QObject(), QDict<vfile>(), fileIterator( 0 ),
                                   parentWidget( w ), busy( false ), result( false ), currentUrl() {
  setAutoDelete( true );
}

SynchronizerDirList::~SynchronizerDirList() {
  if( fileIterator )
    delete fileIterator;
}

vfile * SynchronizerDirList::search( const QString &name, bool ignoreCase ) {
  if( !ignoreCase )
    return (*this)[ name ];

  vfile *item = first();
  QString file = name.lower();

  while( item )
  {
    if( file == item->vfile_getName().lower() )
      return item;
    item = next();
  }
  return 0;
}

vfile * SynchronizerDirList::first() {
  return fileIterator->toFirst();
}

vfile * SynchronizerDirList::next() {
  return ++(*fileIterator);
}

bool SynchronizerDirList::load( const QString &urlIn, bool wait ) {
  if( busy )
    return false;

  currentUrl = urlIn;
  KURL url = vfs::fromPathOrURL( urlIn );

  if( fileIterator == 0 )
    fileIterator = new QDictIterator<vfile> ( *this );

  clear();

  if( url.isLocalFile() ) {
    QString path = url.path( -1 );
    DIR* dir = opendir(path.local8Bit());
    if(!dir)  {
      KMessageBox::error(parentWidget, i18n("Can't open the %1 directory!").arg( path ), i18n("Error"));
      emit finished( result = false );
      return false;
    }

    struct dirent* dirEnt;
    QString name;

    while( (dirEnt=readdir(dir)) != NULL ){
      name = QString::fromLocal8Bit(dirEnt->d_name);

      if (name=="." || name == "..") continue;

      QString fullName = path + "/" + name;

      KDE_struct_stat stat_p;
      KDE_lstat(fullName.local8Bit(),&stat_p);

      bool symLink= S_ISLNK(stat_p.st_mode);
      char symDest[256];
      bzero(symDest,256); 
      if( symLink ){  // who the link is pointing to ?
        int endOfName=0;
        endOfName=readlink(fullName.local8Bit(),symDest,256);
      }

      QString symlinkDest = QString::fromLocal8Bit( symDest );

      QString mime = QString::null;
      QString perm = KRpermHandler::mode2QString(stat_p.st_mode);

      KURL fileURL = KURL::fromPathOrURL( fullName );

      vfile* item=new vfile(name,stat_p.st_size,perm,stat_p.st_mtime,symLink,stat_p.st_uid,
                        stat_p.st_gid,mime,QString::fromLocal8Bit( symDest ),stat_p.st_mode);
      item->vfile_setUrl( fileURL );

      insert( name, item );
    }

    closedir( dir );
    emit finished( result = true );
    return true;
  } else {
    KIO::Job *job = KIO::listDir( url, false, true );
    connect( job, SIGNAL( entries( KIO::Job*, const KIO::UDSEntryList& ) ),
             this, SLOT( slotEntries( KIO::Job*, const KIO::UDSEntryList& ) ) );
    connect( job, SIGNAL( result( KIO::Job* ) ),
             this, SLOT( slotListResult( KIO::Job* ) ) );
    busy = true;

    if( !wait )
      return true;

    while( busy )
      qApp->processEvents();
    return result;
  }
}

void SynchronizerDirList::slotEntries( KIO::Job * job, const KIO::UDSEntryList& entries ) 
{
  KIO::UDSEntryListConstIterator it = entries.begin();
  KIO::UDSEntryListConstIterator end = entries.end();

  while( it != end )
  {
    KFileItem kfi( *it, (( KIO::ListJob *)job )->url(), true, true );
    QString key = kfi.text();
    if( key != "." && key != ".." ) {
      mode_t mode = kfi.mode() | kfi.permissions();
      QString perm = KRpermHandler::mode2QString( mode );
      if ( kfi.isDir() ) 
        perm[ 0 ] = 'd';

      vfile *item = new vfile( kfi.text(), kfi.size(), perm, kfi.time( KIO::UDS_MODIFICATION_TIME ),
          kfi.isLink(), kfi.user(), kfi.group(), kfi.user(), 
          kfi.mimetype(), kfi.linkDest(), mode );
      insert( key, item );
    }
    ++it;
  }
}

void SynchronizerDirList::slotListResult( KIO::Job *job ) {
  busy = false;
  if ( job && job->error() ) {
    job->showErrorDialog( parentWidget );
    emit finished( result = false );
    return;
  }
  emit finished( result = true );
}

#include "synchronizerdirlist.moc"