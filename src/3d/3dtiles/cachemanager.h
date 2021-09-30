#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include <QUrl>

namespace ThreeDTiles
{


  class CacheManager
  {
    private:
      static QString cacheDir3dTiles;

    public:
      CacheManager();

      /**
       * \brief Synchonously retrieves file/url content and saves it to cached file
       * \param url input
       * \param tsName cache name (ie. tileset name)
       * \return cache file name
       */
      static QString retrieveFileContent( QUrl url, const QString &tsName );

      static void createCacheDirectories( const QString &tsName, const QString &fileName );
      static QString getCacheDirectory( const QString &tsName, const QString &filename );

  };


}

using namespace ThreeDTiles;

#endif // CACHEMANAGER_H
