/***************************************************************************
  3dtiles.cpp
  --------------------------------------
  Date                 : Mars 2021
  Copyright            : (C) 2021 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "3dtiles.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <algorithm>
#include <fstream>
#include <streambuf>
#include <iosfwd>
#include "qgsapplication.h"
#include "qgssourcecache.h"
#include <QThread>
#include <QTimer>
#include "qgsnetworkcontentfetcherregistry.h"

QString waitForData( QUrl url, const Tileset *ts )
{
  QString outFileName = url.toString();
  if ( url.scheme().startsWith( "http" ) )
  {
    bool doDl = true;
    QString fn = url.fileName();
    QString cachedFileName = ( ts == NULL ? Tileset::getCacheDirectory( "no_name", fn ) : ts->getCacheDirectory( fn ) ) + fn ;
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
      /*
      QCoreApplication::processEvents( QEventLoop::ProcessEventsFlag::EventLoopExec );
      int i = 1000;
      while ( i > 0 && ( fetchedContent->status() == QgsFetchedContent::Downloading
                   || fetchedContent->status() == QgsFetchedContent::NotStarted ) )
      {
      QThread::msleep(50);
      QCoreApplication::processEvents( QEventLoop::ProcessEventsFlag::EventLoopExec );
      i--;
      }*/

      if ( fetchedContent->status() == QgsFetchedContent::Finished )
      {
        if ( fn.endsWith( ".b3dm" ) )
        {
          ts == NULL ? Tileset::createCacheDirectories( "no_name", fn ) : ts->createCacheDirectories( fn );

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

// =====================================================================
// QgsMatrix4x4
// =====================================================================

const QgsMatrix4x4 &ThreeDTiles::buildFromJson( QgsMatrix4x4 &matrix,
    const QJsonArray &array )
{
  // read matrix as a column major order
  for ( int i( 0 ), column( 0 ); column < 4; ++column )
  {
    QgsVector4D col;
    for ( int row( 0 ); row < 4; ++row, ++i )
    {
      col[row] = array[i].toDouble();
    }
    matrix.setColumn( column, col );
  }
  return matrix;
}

// =====================================================================
// Q3dPrimitive
// =====================================================================

Q3dPrimitive::Q3dPrimitive() :
  mPoints()
{
  // noop
}

void Q3dPrimitive::reproject( const QgsCoordinateTransform &coordTrans )
{
  std::vector<double> x, y, z;
  for ( int i = 0; i < mPoints.size(); ++i )
  {
    x.push_back( mPoints[i].x() );
    y.push_back( mPoints[i].y() );
    z.push_back( mPoints[i].z() );
  }
  coordTrans.transformCoords( mPoints.size(), x.data(), y.data(), z.data() );
  for ( int i = 0; i < mPoints.size(); ++i )
  {
    mPoints[i] = QgsVector4D( x[i], y[i], z[i], mPoints[i].w() );
  }
}

template<typename T, typename std::enable_if<
           std::is_base_of<Q3dPrimitive, T>::value>::type *>
T ThreeDTiles::operator*( const QgsMatrix4x4 &matrix, T &in )
{
  T out;
  out.mPoints.resize( in.mPoints.size() );
  for ( int i = 0; i < in.mPoints.size(); ++i )
  {
    out.mPoints[i] = matrix * in.mPoints[i];
  }
  return out;
}

// =====================================================================
// Q3dCube
// =====================================================================

Q3dCube::Q3dCube() :
  Q3dPrimitive()
{
  mPoints.append( QgsVector4D( 0.0, 0.0, 0.0, 1.0 ) );
  mPoints.append( QgsVector4D( 0.0, 0.0, 0.0, 1.0 ) );
}

Q3dCube::Q3dCube( const QgsVector3D &ll, const QgsVector3D &ur ) :
  Q3dPrimitive()
{
  mPoints.append( QgsVector4D( ll[0], ll[1], ll[2], 1.0 ) );
  mPoints.append( QgsVector4D( ur[0], ll[1], ll[2], 1.0 ) );
  mPoints.append( QgsVector4D( ur[0], ur[1], ll[2], 1.0 ) );
  mPoints.append( QgsVector4D( ll[0], ur[1], ll[2], 1.0 ) );
  mPoints.append( QgsVector4D( ll[0], ll[1], ur[2], 1.0 ) );
  mPoints.append( QgsVector4D( ur[0], ll[1], ur[2], 1.0 ) );
  mPoints.append( QgsVector4D( ur[0], ur[1], ur[2], 1.0 ) );
  mPoints.append( QgsVector4D( ll[0], ur[1], ur[2], 1.0 ) );
}

QVector<QgsPoint> Q3dCube::asQgsPoints()
{
  QVector<QgsPoint> pts;
  QVector<QgsPoint> cubePts;
  for ( int i = 0; i < mPoints.size(); i++ )
  {
    cubePts.append( QgsPoint( mPoints[i][0], mPoints[i][1], mPoints[i][2] ) );
  }

  if ( cubePts.size() == 8 )
  {
    pts << cubePts[0] << cubePts[1] << cubePts[2] << cubePts[3] << cubePts[0]
        << cubePts[4] << cubePts[5] << cubePts[6];
    pts << cubePts[7] << cubePts[4] << cubePts[3] << cubePts[7] << cubePts[2]
        << cubePts[6] << cubePts[1] << cubePts[5];
  }
  else
  {
    for ( int i = 0; i < cubePts.size(); i++ )
    {
      pts << cubePts[i] ;
    }
  }
  return pts;
}

QDebug &operator<<( QDebug &out, const Q3dCube &t )
{
  out << "[Q3dCube";
  out << "ll:" << t.ll();
  out << ", ur:" << t.ur();
  out << "] \n";
  return out;
}

// =====================================================================
// ThreedTilesContent
// =====================================================================
ThreeDTilesContent::ThreeDTilesContent() :
  mType( unmanaged ), mSource( QUrl() ), mDepth( 0 ), mParentTile( NULL )
{
  // noop
}

ThreeDTilesContent::ThreeDTilesContent( const TileType &t, const QUrl &source, Tile *parentTile, int depth ) :
  mType( t ), mSource( source ), mDepth( depth ), mParentTile( parentTile )
{
  // noop
}

// =====================================================================
// Asset
// =====================================================================
Asset::Asset() :
  mVersion()
{
  // noop
}

const Asset &Asset::setFrom( const QJsonObject &obj )
{
  mVersion = obj["version"].toString();
  return *this;
}

// =====================================================================
// BoundingVolume
// =====================================================================
BoundingVolume::BoundingVolume( const BoundingVolume::BVType &t ) :
  mType( t ), mEpsg( QgsCoordinateReferenceSystem::fromEpsgId( 4978 ) )
{
  //noop
}

bool BoundingVolume::checkCoordinateTransform( const QgsCoordinateTransform *coordTrans ) const
{
  if ( coordTrans != NULL && mEpsg.authid().compare( coordTrans->sourceCrs().authid() ) != 0 )  // if geom srs is different from the source srs, do not use coord trans
  {
    LOGTHROW( critical, std::runtime_error, QStringLiteral( "Bounding volume CRS (%1) does not match transform CRS (%2)!" )
              .arg( mEpsg.authid() )
              .arg( coordTrans->sourceCrs().authid() ) );
  }
  return true;
}



QgsGeometry BoundingVolume::asGeometry( const QgsMatrix4x4 &transform,
                                        const QgsCoordinateTransform *coordTrans )
{
  Q3dCube cube = asCube( transform, coordTrans );
  return QgsGeometry( new QgsLineString( cube.asQgsPoints() ) );
}

QgsAABB BoundingVolume::asQgsAABB( const QgsMatrix4x4 &transform, const QgsCoordinateTransform *coordTrans, bool flipZY )
{
  Q3dCube cube = asCube( transform, coordTrans );

  if ( flipZY )
  {
    cube = flipZYMat * cube;
  }

  return QgsAABB( cube.ll().x(), cube.ll().y(), cube.ll().z(),
                  cube.ur().x(), cube.ur().y(), cube.ur().z() );
}

// =====================================================================
// Box
// =====================================================================
Box::Box( const QJsonArray &value ) :
  BoundingVolume( BoundingVolume::box )
{
  mCenter.setX( value[0].toDouble() );
  mCenter.setY( value[1].toDouble() );
  mCenter.setZ( value[2].toDouble() );

  mU.setX( value[3].toDouble() );
  mU.setY( value[4].toDouble() );
  mU.setZ( value[5].toDouble() );

  mV.setX( value[6].toDouble() );
  mV.setY( value[7].toDouble() );
  mV.setZ( value[8].toDouble() );

  mW.setX( value[9].toDouble() );
  mW.setY( value[10].toDouble() );
  mW.setZ( value[11].toDouble() );

  if ( abs( mCenter[0] ) > 180.0 || abs( mCenter[1] ) > 180.0
       || abs( mCenter[2] ) > 180.0 )
  {
    mEpsg = QgsCoordinateReferenceSystem::fromEpsgId( 4978 );
  }
}

inline QgsMatrix4x4 Box::fromRotationTranslation()
{
  return QgsMatrix4x4( mU[0], mV[0], mW[0], mCenter[0], //
                       mU[1], mV[1], mW[1], mCenter[1], //
                       mU[2], mV[2], mW[2], mCenter[2], //
                       0.0, 0.0, 0.0, 1.0 );
}

Q3dCube Box::asCube( const QgsMatrix4x4 &transform,
                     const QgsCoordinateTransform *coordTrans )
{
  checkCoordinateTransform( coordTrans );
  Q3dCube cube( QgsVector3D( -1.0, -1.0, -1.0 ), QgsVector3D( 1.0, 1.0, 1.0 ) );
  QgsMatrix4x4 rotMat = fromRotationTranslation();
  //qDebug() << "Q3dCube rot matrix: " << rotMat << "\n";
  cube = transform * rotMat * cube;
  if ( coordTrans )
  {
    cube.reproject( *coordTrans );
  }
  //qDebug() << "Q3dCube cube: " << cube << "\n";
  return cube;
}

// =====================================================================
// Sphere
// =====================================================================
Sphere::Sphere() :
  BoundingVolume( BoundingVolume::sphere )
{
  // noop
}

Sphere::Sphere( const QJsonArray &value ) :
  BoundingVolume( BoundingVolume::sphere )
{
  mCenter.setX( value[0].toDouble() );
  mCenter.setY( value[1].toDouble() );
  mCenter.setZ( value[2].toDouble() );

  mRadius = value[3].toDouble();

  if ( abs( mCenter[0] ) > 180.0 || abs( mCenter[1] ) > 180.0
       || abs( mCenter[2] ) > 180.0 )
  {
    mEpsg = QgsCoordinateReferenceSystem::fromEpsgId( 4978 );
  }
}

Q3dCube Sphere::asCube( const QgsMatrix4x4 &transform,
                        const QgsCoordinateTransform *coordTrans )
{
  checkCoordinateTransform( coordTrans );
  Q3dCube cube(
    QgsVector3D( mCenter[0] - mRadius, mCenter[1] - mRadius, mCenter[2] - mRadius ),
    QgsVector3D( mCenter[0] + mRadius, mCenter[1] + mRadius, mCenter[2] + mRadius ) );
  cube = transform * cube;
  if ( coordTrans )
  {
    cube.reproject( *coordTrans );
  }
  return cube;
}

Extents3::Extents3() :
  mLl( QgsVector3D( FLT_MIN, FLT_MIN, FLT_MIN ) ), mUr( QgsVector3D( FLT_MAX, FLT_MAX, FLT_MAX ) )
{
  // noop
}

// =====================================================================
// Region
// =====================================================================
Region::Region() :
  BoundingVolume( BoundingVolume::region )
{
  mEpsg = QgsCoordinateReferenceSystem::fromEpsgId( 4979 );
}

Region::Region( const QJsonArray &value ) :
  BoundingVolume( BoundingVolume::region )
{
  mExtents.mLl.setX( value[0].toDouble() );
  mExtents.mLl.setY( value[1].toDouble() );
  mExtents.mLl.setZ( value[4].toDouble() );

  mExtents.mUr.setX( value[2].toDouble() );
  mExtents.mUr.setY( value[3].toDouble() );
  mExtents.mUr.setZ( value[5].toDouble() );

  mEpsg = QgsCoordinateReferenceSystem::fromEpsgId( 4979 );
}

Q3dCube Region::asCube( const QgsMatrix4x4 &transform,
                        const QgsCoordinateTransform *coordTrans )
{
  checkCoordinateTransform( coordTrans );
  Q3dCube cube( mExtents.mLl, mExtents.mUr );
  cube = transform * cube;
  if ( coordTrans )
  {
    cube.reproject( *coordTrans );
  }
  return cube;
}

// =====================================================================
// Tile
// =====================================================================
Tile::Tile() :
  mParentTileset( NULL ), mBv(), mRefine( Refinement::REPLACE ), mDepth( 0 ), mCombinedTransform( NULL )
{
  // noop
}

Tile::~Tile()
{
  if ( mCombinedTransform )
  {
    delete mCombinedTransform;
  }
}

Tile::Tile( const QJsonObject &obj, int depth, Tileset *parTs, Tile *par ) :
  mParentTileset( parTs ), mParentTile( par ), mRefine( Refinement::REPLACE ), mDepth( depth ), mLazyLoadChildren( true ), mCombinedTransform( NULL )
{

  const QJsonObject &bvJ = obj["boundingVolume"].toObject();
  if ( bvJ.find( "box" ) != bvJ.end() )
  {
    mBv = std::make_unique<Box>( bvJ["box"].toArray() );
  }
  else if ( bvJ.find( "region" ) != bvJ.end() )
  {
    mBv = std::make_unique<Region>( bvJ["region"].toArray() );
  }
  else if ( bvJ.find( "sphere" ) != bvJ.end() )
  {
    mBv = std::make_unique<Sphere>( bvJ["sphere"].toArray() );
  }
  else
  {
    QString str = QJsonDocument( bvJ ).toJson( QJsonDocument::Compact ).toStdString().c_str();
    LOGTHROW( critical, std::runtime_error, QString( "Invalid boundingVolume. bv:" + str ) );
  }

  if ( obj.find( "geometricError" ) != obj.end() )
  {
    mGeometricError = obj["geometricError"].toDouble();
  }

  if ( obj.find( "transform" ) == obj.end() )
  {
    mTransform.setToIdentity();
  }
  else
  {
    buildFromJson( mTransform, obj["transform"].toArray() );
  }

  if ( obj.find( "refine" ) != obj.end() )
  {
    const QString &r = obj["refine"].toString();
    if ( r.compare( "add", Qt::CaseInsensitive ) )
    {
      mRefine = Refinement::ADD;
    }
    else
    {
      mRefine = Refinement::REPLACE;
    }
  }

  if ( obj.find( "content" ) != obj.end() )
  {
    setContentFrom( obj["content"].toObject() );
  }

  if ( ! mContentUrl.toString().endsWith( ".json" ) && obj.find( "children" ) != obj.end() ) // tileset content can not have children
  {
    mChildrenJsonArray = obj["children"].toArray();
  }
}

const QList<std::shared_ptr<Tile>> Tile::children() const
{
  return mChildren;
}

const QList<std::shared_ptr<Tile>> Tile::children()
{
  if ( ! mContentUrl.toString().endsWith( ".json" ) && mLazyLoadChildren ) // tileset content can not have children
  {
    for ( const auto &c : mChildrenJsonArray )
    {
      mChildren.append( std::make_shared<Tile>( c.toObject(), mDepth + 1, mParentTileset, this ) );
    }
    mLazyLoadChildren = false;
  }

  return mChildren;
}

QgsMatrix4x4 Tile::getCombinedTransformRec( Tile *p )
{
  if ( p->mParentTile != NULL ) // current tile is then child of another tile
  {
    return getCombinedTransformRec( p->mParentTile ) * p->mTransform;
  }
  else
  {
    if ( p->mParentTileset != NULL )
    {
      return getCombinedTransformRec( p->mParentTileset ) * p->mTransform;
    }
    else
    {
      return p->mTransform;
    }
  }
}

QgsMatrix4x4 Tile::getCombinedTransformRec( Tileset *p )
{
  if ( p->mParentTile != NULL )
  {
    return getCombinedTransformRec( p->mParentTile );
  }
  else
  {
    QgsMatrix4x4 out;
    out.setToIdentity();
    return out;
  }
}

QgsMatrix4x4 *Tile::getCombinedTransform()
{
  if ( mCombinedTransform == NULL )
  {
    mCombinedTransform = new QgsMatrix4x4( getCombinedTransformRec( this ) );
  }
  return mCombinedTransform;
}

double Tile::getGeometricError() const
{
  if ( ! mParentTileset->getUseOriginalGeomError() && mDepth > 0 )
    return mGeometricError / ( ( mDepth ) * ( mDepth ) );
  else
    return mGeometricError ;
}

QgsGeometry Tile::getBoundingVolumeAsGeometry( const QgsCoordinateTransform *coordTrans )
{
  QgsMatrix4x4 *ct = getCombinedTransform();
  return mBv->asGeometry( *ct, coordTrans );
}

QgsAABB Tile::getBoundingVolumeAsAABB( const QgsCoordinateTransform *coordTrans )
{
  QgsMatrix4x4 *ct = getCombinedTransform();
  return mBv->asQgsAABB( *ct, coordTrans, mParentTileset->getFlipY() );
}

bool Tile::contains( const QgsVector3D &point )
{
  QgsMatrix4x4 *ct = getCombinedTransform();
  QgsAABB c = mBv->asQgsAABB( *ct, NULL, mParentTileset->getFlipY() );
  return ( point.x() >= c.minimum().x() && point.y() >= c.minimum().y() && point.z() >= c.minimum().z()
           && point.x() <= c.maximum().x() && point.y() <= c.maximum().y() && point.z() <= c.maximum().z() );
}


void Tile::setContentFrom( const QJsonObject &obj )
{
  if ( obj.find( "uri" ) != obj.end() )
  {
    mContentUrl = QUrl( obj["uri"].toString() );
  }
  else if ( obj.find( "url" ) != obj.end() )
  {
    mContentUrl = QUrl( obj["url"].toString() );
  }
  else
  {
    LOGTHROW( critical, std::runtime_error, QString( "No 'url' or 'uri' found in json." ) );
  }

  qDebug() << "Will lazy load tile (depth:" << mDepth + 1 << ") content from tileset: "
           << mParentTileset->mSource.resolved( mContentUrl );

}

ThreeDTilesContent *Tile::getContentConst() const
{
  return mContent.get();
}

ThreeDTilesContent *Tile::getContent()
{
  if ( mContent.get() == nullptr )
  {
    if ( mContentUrl.toString().endsWith( ".json" ) )
    {
      qDebug() << "Loading delayed tile (depth:" << mDepth + 1 << ") content from tileset: "
               << mParentTileset->mSource.resolved( mContentUrl );
      mContent = Tileset::fromUrl( mContentUrl,
                                   mParentTileset->mSource, mParentTile, mDepth + 1 );
    }
    else if ( mContentUrl.toString().endsWith( ".b3dm" ) )
    {
      qDebug() << "Loading delayed tile (depth:" << mDepth + 1 << ") content from b3dm: "
               << mParentTileset->mSource.resolved( mContentUrl );
      try
      {
        mContent = B3dmHolder::fromUrl( mContentUrl,
                                        mParentTileset->mSource, this, mDepth + 1 );
      }
      catch ( std::runtime_error *e )
      {
        qWarning() << "B3dmHolder caught exception: " << e->what();
        qWarning() << "Receive exception: couldn't create B3dmHolder!";
      }
    }
    else
    {
      LOGTHROW( critical, std::runtime_error, QString( "Should load tile content from: %1" ).arg( mParentTileset->mSource.resolved( mContentUrl ).toString() ) );
    }
  }

  return mContent.get();
}

// =====================================================================
// Tileset
// =====================================================================
Tileset::Tileset( const QJsonObject &obj, const QUrl &url, Tile *parentTile, int depth ) :
  ThreeDTilesContent( tileset, url, parentTile, depth ),
  mRootBb( NULL ), mCorrectTranslation( false ), mFlipY( false ), mUseFakeMaterial( false ),
  mUseOriginalGeomError( false ), mName( "no_name" )
{
  try
  {
    mAsset.setFrom( obj["asset"].toObject() );
    mGeometricError = obj["geometricError"].toDouble();
    mProperties = obj["properties"].toObject();
    mRootTile = std::unique_ptr<Tile>( new Tile( obj["root"].toObject(), depth, this ) );
  }
  catch ( std::runtime_error *e )
  {
    qWarning() << "Tileset caught exception: " << e->what();
    QString str = QJsonDocument( obj ).toJson();
    qWarning() << "Receive exception: couldn't create tileset! json:" << str;
    //LOGTHROW( critical, std::runtime_error, QString( "Receive exception: couldn't create tileset! json:" + str ) );
  }
}

Tileset::~Tileset()
{
  if ( mRootBb )
  {
    delete mRootBb;
  }
}

std::unique_ptr<ThreeDTilesContent> Tileset::fromUrl( const QUrl &url, const QUrl &base, Tile *parentTile, int depth )
{
  QUrl u = base.resolved( url );

  try
  {
    QString fileName = waitForData( u, ( parentTile == NULL ? NULL : parentTile->mParentTileset ) );
    QFile loadFile( fileName );
    loadFile.open( QIODevice::ReadOnly );
    QByteArray saveData = loadFile.readAll();
    QJsonDocument doc( QJsonDocument::fromJson( saveData ) );
    loadFile.close();

    std::unique_ptr<ThreeDTilesContent> out = std::make_unique<Tileset>( doc.object(), u, parentTile, depth );
    return out;
  }
  catch ( std::runtime_error *e )
  {
    qWarning() << "Tileset::fromUrl for" << u.toString() << "/e:" << e->what();
    LOGTHROW( critical, std::runtime_error, QString( "Receive exception: couldn't create tileset!" ) );
  }

}

Tile *Tileset::findTile( const QgsChunkNodeId &tileId )
{
  Tile *out = NULL;
  if ( tileId.d >= 0 )
  {
    if ( tileId.d == 0 )
    {
      out = mRootTile.get();
    }
    else
    {
      QgsVector3D tileCenter = decodeTileId( tileId );
      out = findTileRecInTileset( this, tileId.d, tileCenter );
    }
  }

  return out;
}

Tile *Tileset::findTileRecInTile( Tile *curTile, int depth, const QgsVector3D &tileCenter )
{
  Tile *out = NULL;

  if ( depth == curTile->mDepth )
  {
    out = curTile;
  }
  else
  {
    if ( curTile->children().isEmpty() ) // no children
    {
      if ( curTile->getContent()->mType == ThreeDTilesContent::tileset )
      {
        out = findTileRecInTileset( ( Tileset * )( curTile->getContent() ), depth + 1, tileCenter );

      }
      else if ( curTile->getContent()->mType == ThreeDTilesContent::b3dm )
      {
        out = curTile;
      }

    }
    else     // scan children
    {
      for ( std::shared_ptr<Tile> child : curTile->children() )
      {
        if ( child->contains( tileCenter ) )
        {
          out = findTileRecInTile( child.get(), depth, tileCenter );
          break;
        }
      }
    }
  }

  return out;
}

Tile *Tileset::findTileRecInTileset( Tileset *curTs, int depth, const QgsVector3D &tileCenter )
{
  Tile *out = NULL;
  Tile *curTile;

  if ( depth == curTs->mDepth )
  {
    out = curTs->mRootTile.get();
  }
  else
  {
    curTile = curTs->mRootTile.get();
    if ( curTile->contains( tileCenter ) )
    {
      out = findTileRecInTile( curTile, depth, tileCenter );
    }
  }

  return out;
}

QgsChunkNodeId Tileset::encodeTileId( int tileLevel, const QgsAABB &tileBb )
{
  buildRootBb();
  QgsChunkNodeId out( tileLevel,
                      ( tileBb.xCenter() - mRootBb->xCenter() ) * 1000.0,
                      ( tileBb.yCenter() - mRootBb->yCenter() ) * 1000.0,
                      ( tileBb.zCenter() - mRootBb->zCenter() ) * 1000.0 );

//  qDebug() << "encodeTileId tileBb:" << tileBb.center() << "/ mRootBb:" << mRootBb->center() << "/ tileId:" << out.text();
  return out;
}

QgsVector3D Tileset::decodeTileId( const QgsChunkNodeId &tileId )
{
  buildRootBb();
  QgsVector3D out( ( tileId.x / 1000.0 ) + mRootBb->xCenter(),
                   ( tileId.y / 1000.0 ) + mRootBb->yCenter(),
                   ( tileId.z / 1000.0 ) + mRootBb->zCenter() );
//  qDebug() << "decodeTileId tileBb:" << out << "/ mRootBb:" << mRootBb->center() << "/ tileId:" << tileId.text();
  return out;
}

void Tileset::buildRootBb()
{
  if ( mRootBb == NULL )
  {
    mRootBb = new QgsAABB( mRootTile->getBoundingVolumeAsAABB( NULL ) );
  }
}

double Tileset::getGeometricError() const
{
  if ( ! getUseOriginalGeomError() && mDepth > 0 )
    return mGeometricError / ( ( mDepth ) * ( mDepth ) );
  else
    return mGeometricError;
}

Tileset const *Tileset::getRootTileset() const
{
  Tileset const *curTS = this;
  while ( curTS->mParentTile != NULL )  // a tileset can be included in a tile
  {
    curTS = curTS->mParentTile->mParentTileset; // a tile is always in a tileset
  }
  return curTS;
}

void Tileset::setName( QString val )
{
  mName = val;
}

QString Tileset::getName() const
{
  return getRootTileset()->mName;
}


void Tileset::setFlipY( bool val )
{
  mFlipY = val;
}

bool Tileset::getFlipY() const
{
  return getRootTileset()->mFlipY;
}


void Tileset::setCorrectTranslation( bool val )
{
  mCorrectTranslation = val;
}

bool Tileset::getCorrectTranslation() const
{
  return getRootTileset()->mCorrectTranslation;
}

void Tileset::setUseFakeMaterial( bool val )
{
  mUseFakeMaterial = val;
}

bool Tileset::getUseFakeMaterial() const
{
  return getRootTileset()->mUseFakeMaterial;
}

void Tileset::setUseOriginalGeomError( bool val )
{
  mUseOriginalGeomError = val;
}

bool Tileset::getUseOriginalGeomError() const
{
  return getRootTileset()->mUseOriginalGeomError;
}

void Tileset::createCacheDirectories( const QString &tsName, const QString &fileName )
{
  QDir rootDir( cacheDir3dTiles );
  if ( !rootDir.exists() )
  {
    if ( !rootDir.mkdir( cacheDir3dTiles ) )
    {
      LOGTHROW( critical, std::runtime_error, QString( "Unable to create cache dir:" + cacheDir3dTiles ) );
    }
  }

  QDir tsDir( cacheDir3dTiles + tsName );
  if ( !tsDir.exists() )
  {
    if ( !tsDir.mkdir( cacheDir3dTiles + tsName ) )
    {
      LOGTHROW( critical, std::runtime_error, QString( "Unable to create tileset cache dir:" + cacheDir3dTiles + tsName ) );
    }
  }

  QDir tileDir( cacheDir3dTiles + tsName + "/" + fileName );
  if ( !tileDir.exists() )
  {
    if ( !tileDir.mkdir( cacheDir3dTiles + tsName + "/" + fileName ) )
    {
      LOGTHROW( critical, std::runtime_error, QString( "Unable to create tile cache dir:" + cacheDir3dTiles + tsName + "/" + fileName ) );
    }
  }
}

void Tileset::createCacheDirectories( const QString &fn ) const
{
  createCacheDirectories( getName(), fn );
}

QString Tileset::getCacheDirectory( const QString &tsName, const QString &fileName )
{
  return cacheDir3dTiles + tsName + "/" + fileName + "/" ;
}

QString Tileset::getCacheDirectory( const QString &fn ) const
{
  return getCacheDirectory( getName(), fn );
}



// =====================================================================
// B3dmHolder
// =====================================================================
B3dmHolder::B3dmHolder( QIODevice &dev, const QUrl &url, Tile *parentTile, int depth ) :
  ThreeDTilesContent( b3dm, url, parentTile, depth ), mModel()
{
  mModel.mUrl = url;
  mModel.load( dev );
}

std::unique_ptr<ThreeDTilesContent> B3dmHolder::fromUrl( const QUrl &url, const QUrl &base, Tile *parentTile, int depth )
{
  QUrl u = base.resolved( url );
  QString fileName = waitForData( u, parentTile->mParentTileset );

  QFile loadFile( fileName );
  loadFile.open( QIODevice::ReadOnly );
  std::unique_ptr<ThreeDTilesContent> out = std::make_unique < B3dmHolder > ( loadFile, u, parentTile, depth );
  loadFile.close();
  return out;
}

// =====================================================================
// QDebug OPERATORS
// =====================================================================

QDebug &operator<<( QDebug &out, const Tileset &ts )
{
  out << "[Tileset";
  out << "version:" << ts.mAsset.mVersion;
  out << ", root:" << *ts.mRootTile;
  out << "] \n";
  return out;
}

QDebug &operator<<( QDebug &out, const ThreeDTilesContent &t )
{
  if ( t.mType == ThreeDTilesContent::b3dm )
  {
    out << static_cast<const B3dmHolder &>( t );
  }
  else
  {
    out << static_cast<const Tileset &>( t );
  }
  return out;
}

QDebug &operator<<( QDebug &out, const Tile &t )
{
  out << "[Tile";
  out << "transform:" << t.mTransform;
  out << ", bv:" << *t.mBv;

  if ( t.getContentConst() != NULL )
  {
    out << ", url:" << t.mContentUrl;
    out << ", content:" << *( t.getContentConst() );
  }

  if ( !t.children().isEmpty() )
  {
    out << ", children: [";
    for ( const auto &c : t.children() )
    {
      out << "\n" << *c << "\n" << ",";
    }
    out << "]";
  }
  out << "] \n";
  return out;
}

QDebug &operator<<( QDebug &out, const B3dmHolder &t )
{
  out << "[B3dm";
  out << "url: " << t.mSource;
  out << ", header: [";
  out << "featureTableJsonByteLength:"
      << t.mModel.mHeader.featureTableJsonByteLength;
  out << ", featureTableBinaryByteLength:"
      << t.mModel.mHeader.featureTableBinaryByteLength;
  out << ", batchTableJsonByteLength:"
      << t.mModel.mHeader.batchTableJsonByteLength;
  out << ", batchTableBinaryByteLength:"
      << t.mModel.mHeader.batchTableBinaryByteLength;
  out << "]";
  out << ", featureTable: [";
  out << "batchLength:" << t.mModel.mFeatureTable.mBatchLength;
  out << ", rtcCenter:" << t.mModel.mFeatureTable.mRtcCenter;
  out << "]";
  out << "] \n";
  return out;
}

QDebug &operator<<( QDebug &out, const BoundingVolume &t )
{
  if ( t.mType == BoundingVolume::box )
  {
    out << static_cast<const Box &>( t );

  }
  else if ( t.mType == BoundingVolume::region )
  {
    out << static_cast<const Region &>( t );

  }
  else if ( t.mType == BoundingVolume::sphere )
  {
    out << static_cast<const Sphere &>( t );

  }
  else
  {
    out << "[Unmanaged BV]";
  }

  return out;
}

QDebug &operator<<( QDebug &out, const Region &t )
{
  out << "[Region";
  out << "ll:" << t.mExtents.mLl;
  out << ", ur:" << t.mExtents.mUr;
  out << "] \n";
  return out;
}

QDebug &operator<<( QDebug &out, const Sphere &t )
{
  out << "[Sphere";
  out << "center:" << t.mCenter;
  out << ", radius:" << t.mRadius;
  out << "] \n";
  return out;
}

QDebug &operator<<( QDebug &out, const Box &t )
{
  out << "[Box";
  out << "center:" << t.mCenter;
  out << ", x:" << t.mU;
  out << ", y:" << t.mV;
  out << ", z:" << t.mW;
  out << "] \n";
  return out;
}
