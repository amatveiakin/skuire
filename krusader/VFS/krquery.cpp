/***************************************************************************
                                 krquery.cpp
                             -------------------
    copyright            : (C) 2001 by Shie Erlich & Rafi Yanai
    email                : krusader@users.sourceforge.net
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



#include "krquery.h"
#include "../krusader.h"
#include "../resources.h"
#include "../VFS/vfs.h"
#include "krarchandler.h"

#include <qtextstream.h>
#include <qregexp.h>
#include <klargefile.h>
#include <klocale.h>
#include <kmimetype.h>
#include <qfile.h>
#include <kurlcompletion.h>

// set the defaults
KRQuery::KRQuery(): matchesCaseSensitive(true), bNull( true ),
                    contain(QString::null),containCaseSensetive(true),
                    containWholeWord(false),
                    minSize(0),maxSize(0),newerThen(0),olderThen(0),
                    owner(QString::null),group(QString::null),
                    perm(QString::null),type(QString::null),
                    inArchive(false),recurse(true),followLinksP(true) {}

// set the defaults
KRQuery::KRQuery( QString name, bool matchCase ):
                    bNull( true ),contain(QString::null),
                    containCaseSensetive(true),containWholeWord(false),
                    minSize(0),maxSize(0),newerThen(0),olderThen(0),
                    owner(QString::null),group(QString::null),
                    perm(QString::null),type(QString::null),
                    inArchive(false),recurse(true),followLinksP(true) {
  setNameFilter( name, matchCase );
}

bool KRQuery::checkPerm( QString filePerm ) const
{
  for ( int i = 0; i < 9; ++i )
    if ( perm[ i ] != '?' && perm[ i ] != filePerm[ i + 1 ] ) return false;
  return true;
}

bool KRQuery::checkType( QString mime ) const
{
  if ( type == mime ) return true;
  if ( type == i18n( "Archives" ) ) return KRarcHandler::arcSupported( mime.right( 4 ) );
  if ( type == i18n( "Directories" ) ) return mime.contains( "directory" );
  if ( type == i18n( "Image Files" ) ) return mime.contains( "image/" );
  if ( type == i18n( "Text Files" ) ) return mime.contains( "text/" );
  if ( type == i18n( "Video Files" ) ) return mime.contains( "video/" );
  if ( type == i18n( "Audio Files" ) ) return mime.contains( "audio/" );
  if ( type == i18n( "Custom" ) ) return customType.contains( mime );
  return false;
}

bool KRQuery::match( const QString nameIn ) const
{
  if( excludes.count() == 0 && matches.count() == 0 ) /* true if there's no match condition */
    return true;

  QString name( nameIn );
  int ndx = nameIn.findRev( '/' ); // virtual filenames may contain '/'
  if( ndx != -1 )                  // but the end of the filename is OK
    name = nameIn.mid( ndx + 1 );

  unsigned int len;
  for ( unsigned int i = 0; i < excludes.count(); ++i )
  {
    QRegExp( *excludes.at( i ), matchesCaseSensitive, true ).match( name, 0, ( int* ) & len );
    if ( len == name.length() ) return false;
  }
  for ( unsigned int i = 0; i < matches.count(); ++i )
  {
    QRegExp( *matches.at( i ), matchesCaseSensitive, true ).match( name, 0, ( int* ) & len );
    if ( len == name.length() ) return true;
  }
  return false;
}

bool KRQuery::match( vfile *vf ) const
{
  // see if the name matches
  if ( !match( vf->vfile_getName() ) ) return false;
  // checking the mime
  if( !type.isEmpty() && !checkType( vf->vfile_getMime() ) ) return false;
  // check that the size fit
  KIO::filesize_t size = vf->vfile_getSize();
  if ( minSize && size < minSize ) return false;
  if ( maxSize && size > maxSize ) return false;
  // check the time frame
  time_t mtime = vf->vfile_getTime_t();
  if ( olderThen && mtime > olderThen ) return false;
  if ( newerThen && mtime < newerThen ) return false;
  // check owner name
  if ( !owner.isEmpty() && vf->vfile_getOwner() != owner ) return false;
  // check group name
  if ( !group.isEmpty() && vf->vfile_getGroup() != group ) return false;
  //check permission
  if ( !perm.isEmpty() && !checkPerm( vf->vfile_getPerm() ) ) return false;

  if ( !contain.isEmpty() )
  {
    if( vf->vfile_getUrl().isLocalFile() )
    {
      if( !containsContent( vf->vfile_getUrl().path() ) ) return false;
    }
    else
    {
      /* TODO: search in remote vfs is not yet implemented */
    }
  }

  return true;
}

// takes the string and adds BOLD to it, so that when it is displayed,
// the grepped text will be bold
void fixFoundTextForDisplay(QString& haystack, int start, int length) {
	haystack.insert(start+length, "</b>");
	haystack.insert(start, "<b>");
	haystack = ("<qt>"+haystack);
}

bool KRQuery::containsContent( QString file ) const
{
  QFile qf( file );

  qf.open( IO_ReadOnly );
  QTextStream text( &qf );
  text.setEncoding( QTextStream::Locale );
  QString line;
  int ndx = 0;
  while ( !text.atEnd() )
  {
    ndx = 0;
    line = text.readLine();
    if ( line.isNull() ) break;
    if ( containWholeWord )
    {
      while ( ( ndx = line.find( contain, ndx, containCaseSensetive ) ) != -1 )
      {
        QChar before = line.at( ndx - 1 );
        QChar after = line.at( ndx + contain.length() );

        if ( !before.isLetterOrNumber() && !after.isLetterOrNumber() &&
          after != '_' && before != '_' ) {
          	lastSuccessfulGrep = line;
          	fixFoundTextForDisplay(lastSuccessfulGrep, ndx, contain.length());
            return true;
           }
        ndx++;
      }
    }
    else if ( (ndx = line.find( contain, 0, containCaseSensetive )) != -1 ) {
    	lastSuccessfulGrep = line;
    	fixFoundTextForDisplay(lastSuccessfulGrep, ndx, contain.length());
      return true;
    }

  }
  lastSuccessfulGrep = QString::null; // nothing was found
  return false;
}

void KRQuery::setNameFilter( QString text, bool cs )
{
  bNull = false;
  matchesCaseSensitive = cs;
  origFilter = text;

  QString matchText = text;
  QString excludeText;
  
  int excludeNdx = matchText.find( '|' );
  if( excludeNdx > -1 )
  {
    excludeText = matchText.mid( excludeNdx + 1 ).stripWhiteSpace();
    matchText.truncate( excludeNdx );
    matchText = matchText.stripWhiteSpace();
    if( matchText.isEmpty() )
      matchText = "*";
  }

  matches  = QStringList::split(QChar(' '), matchText );
  
  if( !matchText.contains( "*" ) && !matchText.contains( "?" ) ) {
    for ( unsigned int i = 0; i < matches.count(); ++i )
      matches[ i ] = "*" + matches[ i ] + "*";
  }
  
  excludes = QStringList::split(QChar(' '), excludeText );
  if( !excludeText.contains( "*" ) && !excludeText.contains( "?" ) ) {
    for ( unsigned int i = 0; i < excludes.count(); ++i )
      excludes[ i ] = "*" + excludes[ i ] + "*";
  }
}

void KRQuery::setContent( QString content, bool cs, bool wholeWord )
{
  bNull = false;
  contain = content;
  containCaseSensetive = cs;
  containWholeWord = wholeWord;
}

void KRQuery::setMinimumFileSize( KIO::filesize_t minimumSize )
{
  bNull = false;
  minSize = minimumSize;
}

void KRQuery::setMaximumFileSize( KIO::filesize_t maximumSize )
{
  bNull = false;
  maxSize = maximumSize;
}

void KRQuery::setNewerThan( time_t time )
{
  bNull = false;
  newerThen = time;
}

void KRQuery::setOlderThan( time_t time )
{
  bNull = false;
  olderThen = time;
}

void KRQuery::setOwner( QString ownerIn )
{
  bNull = false;
  owner = ownerIn;
}

void KRQuery::setGroup( QString groupIn )
{
  bNull = false;
  group = groupIn;
}

void KRQuery::setPermissions( QString permIn )
{
  bNull = false;
  perm = permIn;
}

void KRQuery::setMimeType( QString typeIn, QStringList customList )
{
  bNull = false;
  type = typeIn;
  customType = customList;
}

bool KRQuery::isExcluded( KURL url )
{
  for ( unsigned int i = 0; i < whereNotToSearch.count(); ++i )
    if( whereNotToSearch [ i ].isParentOf( url ) || url.equals( whereNotToSearch [ i ], true ) )
      return true;

  return false;
}

void KRQuery::setSearchInDirs( KURL::List urls ) { 
  whereToSearch.clear();
  for( unsigned int i = 0; i < urls.count(); ++i ) {
    QString url = urls[ i ].url();
    KURL completed = vfs::fromPathOrURL( KURLCompletion::replacedPath( url, true, true ) );
    whereToSearch.append( completed );
  }
}

void KRQuery::setDontSearchInDirs( KURL::List urls ) { 
  whereNotToSearch.clear();
  for( unsigned int i = 0; i < urls.count(); ++i ) {
    QString url = urls[ i ].url();
    KURL completed = vfs::fromPathOrURL( KURLCompletion::replacedPath( url, true, true ) );
    whereNotToSearch.append( completed );
  }
}
