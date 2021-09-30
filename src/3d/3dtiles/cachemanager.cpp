#include "b3dm.h"
#include "cachemanager.h"
#include <QFile>
#include <QDir>
#include "qgsnetworkcontentfetcherregistry.h"
#include "qgsapplication.h"

// TODO should be settable elsewhere
QString CacheManager::cacheDir3dTiles( "/home/bde/.cache/3dtiles/" );

CacheManager::CacheManager()
{
  // noop
}


QString CacheManager::retrieveFileContent( QUrl url, const QString &tsName )
{
  QString outFileName = url.toString();
  if ( url.scheme().startsWith( "http" ) )
  {
    bool doDl = true;
    QString fn = url.fileName();
    QString cachedFileName = CacheManager::getCacheDirectory( tsName, fn ) + fn ;
    if ( fn.endsWith( ".b3dm" ) )
    {
      QFile cachedFile( cachedFileName );
      if ( cachedFile.exists() )
      {
        outFileName = cachedFileName;
        qDebug() << "DL file (cached)" << url.toString() << "to" << outFileName;
        doDl = false;
      }
    }

    if ( doDl )
    {
      QEventLoop loop;
      QgsFetchedContent *fetchedContent = QgsApplication::instance()->networkContentFetcherRegistry()->fetch( url.toString(), Qgis::ActionStart::Immediate );
      QObject::connect( fetchedContent, &QgsFetchedContent::fetched, &loop, &QEventLoop::quit );

      loop.exec();

      if ( fetchedContent->status() == QgsFetchedContent::Finished )
      {
        if ( fn.endsWith( ".b3dm" ) )
        {
          CacheManager::createCacheDirectories( tsName, fn );

          QFile dLFile( fetchedContent->filePath() );
          if ( !dLFile.rename( cachedFileName ) )
          {
            LOGTHROW( critical, std::runtime_error, QString( "Unable to create cache file:" + cachedFileName ) );
          }
          dLFile.close();

          outFileName = cachedFileName;
        }
        else
        {
          outFileName = fetchedContent->filePath();
        }

        qDebug() << "DL file" << url.toString() << "to" << outFileName;
      }
      else
      {
        LOGTHROW( critical, std::runtime_error, QString( "Data not ready for:" + url.toString() ) );
      }
    }
  }

  return outFileName;
}


QString CacheManager::getCacheDirectory( const QString &tsName, const QString &fileName )
{
  return cacheDir3dTiles + tsName + "/" + fileName + "/" ;
}

void CacheManager::createCacheDirectories( const QString &tsName, const QString &fileName )
{
  QDir rootDir( cacheDir3dTiles );
  if ( !rootDir.exists() )
  {
    if ( !rootDir.mkdir( cacheDir3dTiles ) )
    {
      if ( rootDir.exists() )
      {
        // created by another thread
      }
      else
      {
        LOGTHROW( critical, std::runtime_error, QString( "Unable to create cache dir:" + cacheDir3dTiles ) );
      }
    }
  }

  QDir tsDir( cacheDir3dTiles + tsName );
  if ( !tsDir.exists() )
  {
    if ( !tsDir.mkdir( cacheDir3dTiles + tsName ) )
    {
      if ( tsDir.exists() )
      {
        // created by another thread
      }
      else
      {
        LOGTHROW( critical, std::runtime_error, QString( "Unable to create tileset cache dir:" + cacheDir3dTiles + tsName ) );
      }
    }
  }

  QDir tileDir( cacheDir3dTiles + tsName + "/" + fileName );
  if ( !tileDir.exists() )
  {
    if ( !tileDir.mkdir( cacheDir3dTiles + tsName + "/" + fileName ) )
    {
      if ( tileDir.exists() )
      {
        // created by another thread
      }
      else
      {
        LOGTHROW( critical, std::runtime_error, QString( "Unable to create tile cache dir:" + cacheDir3dTiles + tsName + "/" + fileName ) );
      }
    }
  }
}
