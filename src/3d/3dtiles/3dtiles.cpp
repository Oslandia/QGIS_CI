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
#include <algorithm>
#include <fstream>
#include <streambuf>
#include <iosfwd>


const QgsMatrix4x4 &buildFromJson( QgsMatrix4x4 &matrix,
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

// =======================
// Q3dCube
// =======================

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
T operator*( const QgsMatrix4x4 &matrix, T &in )
{
  T out;
  out.mPoints.resize( in.mPoints.size() );
  for ( int i = 0; i < in.mPoints.size(); ++i )
  {
    out.mPoints[i] = matrix * in.mPoints[i];
  }
  return out;
}

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

  pts << cubePts[0] << cubePts[1] << cubePts[2] << cubePts[3] << cubePts[0]
      << cubePts[4] << cubePts[5] << cubePts[6];
  pts << cubePts[7] << cubePts[4] << cubePts[3] << cubePts[7] << cubePts[2]
      << cubePts[6] << cubePts[1] << cubePts[5];
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

// =======================
// ThreedTilesContent
// =======================
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

// =======================
// Asset
// =======================
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

// =======================
// BoundingVolume
// =======================
BoundingVolume::BoundingVolume( const BoundingVolume::BVType &t ) :
  mType( t ), mEpsg( QgsCoordinateReferenceSystem::fromEpsgId( 4978 ) )
{
  mFlipZYMat.setToIdentity();
  mFlipZYMat.rotate( -90.0, 1.0, 0.0, 0.0 );
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
    cube = mFlipZYMat * cube;
  }

  return QgsAABB( cube.ll().x(), cube.ll().y(), cube.ll().z(),
                  cube.ur().x(), cube.ur().y(), cube.ur().z() );
}

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

  /*  if (abs(mCenter[0]) > 180.0 || abs(mCenter[1]) > 180.0
          || abs(mCenter[2]) > 180.0) {
      mEpsg = QgsCoordinateReferenceSystem::fromEpsgId(4978);
  }*/
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

  /*    if (abs(mCenter[0]) > 180.0 || abs(mCenter[1]) > 180.0
          || abs(mCenter[2]) > 180.0) {
      mEpsg = QgsCoordinateReferenceSystem::fromEpsgId(4978);
  }*/
}

Q3dCube Sphere::asCube( const QgsMatrix4x4 &transform,
                        const QgsCoordinateTransform *coordTrans )
{
  Q3dCube cube(
    QgsVector3D( mCenter[0] - mRadius, mCenter[1] - mRadius,
                 mCenter[2] - mRadius ),
    QgsVector3D( mCenter[0] + mRadius, mCenter[1] + mRadius,
                 mCenter[2] + mRadius ) );
  cube = transform * cube;
  if ( coordTrans )
  {
    cube.reproject( *coordTrans );
  }
  return cube;
}

Extents3::Extents3() :
  mLl( QgsVector3D( FLT_MIN, FLT_MIN, FLT_MIN ) ), mUr(
    QgsVector3D( FLT_MAX, FLT_MAX, FLT_MAX ) )
{
  // noop
}

Region::Region() :
  BoundingVolume( BoundingVolume::region )
{
  // noopmake_shared
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

  /*   if (abs(mExtents.mLl[0]) > 180.0 || abs(mExtents.mLl[1]) > 180.0
          || abs(mExtents.mLl[2]) > 180.0 || abs(mExtents.mUr[0]) > 180.0
          || abs(mExtents.mUr[1]) > 180.0 || abs(mExtents.mUr[2]) > 180.0) {
      mEpsg = QgsCoordinateReferenceSystem::fromEpsgId(4978);
  }*/
}

Q3dCube Region::asCube( const QgsMatrix4x4 &transform,
                        const QgsCoordinateTransform *coordTrans )
{
  Q3dCube cube( mExtents.mLl, mExtents.mUr );
  cube = transform * cube;
  if ( coordTrans )
  {
    cube.reproject( *coordTrans );
  }
  return cube;
}

// =======================
// Tile
// =======================
Tile::Tile() :
  mParentTileset( NULL ), mBv(), mDepth( 0 ), mCombinedTransform( NULL )
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
  mParentTileset( parTs ), mParentTile( par ), mDepth( depth ), mCombinedTransform( NULL )
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
    LOGTHROW( fatal, std::runtime_error, QString( "Invalid boundingVolume." ) );
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
    mContent.setFrom( obj["content"].toObject(), depth + 1, this );
  }

  if ( obj.find( "children" ) != obj.end() )
  {
    QJsonArray a = obj["children"].toArray();
    for ( const auto &c : a )
    {
      mChildren.append(
        std::make_shared<Tile>( c.toObject(), depth + 1, parTs,
                                this ) );
    }
  }
}

QgsMatrix4x4 Tile::getCombinedTransformRec( Tile *p )
{
  if ( p->mParentTile != NULL )
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

QgsGeometry Tile::getBoundingVolumeAsGeometry(
  const QgsCoordinateTransform *coordTrans )
{
  QgsMatrix4x4 *ct = getCombinedTransform();
  QgsCoordinateTransform *coordTrans2 = NULL;
  if ( coordTrans != NULL
       && mBv->mEpsg.authid().compare( coordTrans->sourceCrs().authid() )
       == 0 )  // if geom srs is the source srs, use coord trans
  {
    coordTrans2 = ( QgsCoordinateTransform * ) coordTrans;
  }
  return mBv->asGeometry( *ct, coordTrans2 );
}

QgsAABB Tile::getBoundingVolumeAsAABB(
  const QgsCoordinateTransform *coordTrans )
{
  QgsMatrix4x4 *ct = getCombinedTransform();
  QgsCoordinateTransform *coordTrans2 = NULL;
  if ( coordTrans != NULL
       && mBv->mEpsg.authid().compare( coordTrans->sourceCrs().authid() )
       == 0 )  // if geom srs is the source srs, use coord trans
  {
    coordTrans2 = ( QgsCoordinateTransform * ) coordTrans;
  }
  return mBv->asQgsAABB( *ct, coordTrans2, true );
}

bool Tile::contains( const QgsVector3D &point, const QgsCoordinateTransform *coordTrans )
{
  QgsMatrix4x4 *ct = getCombinedTransform();
  QgsAABB c = mBv->asQgsAABB( *ct, coordTrans, true );
  return ( point.x() >= c.minimum().x() && point.y() >= c.minimum().y() && point.z() >= c.minimum().z()
           && point.x() <= c.maximum().x() && point.y() <= c.maximum().y() && point.z() <= c.maximum().z() );
}


// =======================
// TileContent
// =======================
TileContent::TileContent() :
  mParentTile( NULL ), mUrl(), mSubContent()
{
  // noop
}

TileContent::~TileContent()
{
  mSubContent.release();
}

const TileContent &TileContent::setFrom( const QJsonObject &obj, int depth,
    Tile *parentTile )
{
  mParentTile = parentTile;

  if ( obj.find( "uri" ) != obj.end() )
  {
    mUrl = QUrl( obj["uri"].toString() );
  }
  else if ( obj.find( "url" ) != obj.end() )
  {
    mUrl = QUrl( obj["url"].toString() );
  }
  else
  {
    LOGTHROW( fatal, std::runtime_error,
              QString( "No 'url' or 'uri' found in json." ) );
  }

  if ( mUrl.toString().endsWith( ".json" ) )
  {
    qDebug() << "Loading tile (depth:" << depth << ") subcontent from tileset: "
             << mParentTile->mParentTileset->mSource.resolved( mUrl );
    mSubContent = Tileset::fromUrl( mUrl,
                                    mParentTile->mParentTileset->mSource, parentTile, depth );

  }
  else if ( mUrl.toString().endsWith( ".b3dm" ) )
  {
    qDebug() << "Loading tile (depth:" << depth << ") subcontent from b3dm: "
             << mParentTile->mParentTileset->mSource.resolved( mUrl );
    mSubContent = B3dmHolder::fromUrl( mUrl,
                                       mParentTile->mParentTileset->mSource, parentTile, depth );

  }
  else
  {
    qWarning() << "Should load tile subcontent from: "
               << mParentTile->mParentTileset->mSource.resolved( mUrl );
  }

  return *this;
}

// =======================
// Tileset
// =======================
Tileset::Tileset( const QJsonObject &obj, const QUrl &url, Tile *parentTile, int depth ) :
  ThreeDTilesContent( tileset, url, parentTile, depth ),
  mRootBb( NULL )
{

  mAsset.setFrom( obj["asset"].toObject() );
  mGeometricError = obj["geometricError"].toDouble();
  mProperties = obj["properties"].toObject();
  mRoot = std::unique_ptr<Tile>(
            new Tile( obj["root"].toObject(), depth, this ) );
}

std::unique_ptr<ThreeDTilesContent> Tileset::fromUrl( const QUrl &url, const QUrl &base, Tile *parentTile, int depth )
{
  QUrl u = base.resolved( url );
  if ( u.isLocalFile() || u.scheme().isEmpty() )
  {
    QFile loadFile( u.toString() );

    if ( !loadFile.open( QIODevice::ReadOnly ) )
    {
      LOGTHROW( critical, std::runtime_error,
                QString( "Couldn't open json file: " + u.toString() ) );
    }

    QByteArray saveData = loadFile.readAll();
    QJsonDocument doc( QJsonDocument::fromJson( saveData ) );

    return std::make_unique<Tileset>( doc.object(), u, parentTile, depth );

  }
  else
  {
    LOGTHROW( critical, std::runtime_error,
              QString( "Couldn't open json url " + u.toString() ) );
  }
}

Tile *Tileset::findTile( const QgsChunkNodeId &tileId, const QgsCoordinateTransform *coordTrans )
{
  Tile *out = NULL;
  if ( tileId.d == 0 )
  {
    out = mRoot.get();
  }
  else
  {
    QgsVector3D tileCenter = decodeTileId( tileId, *coordTrans );
    out = findTileRecInTileset( this, tileId.d, tileCenter, coordTrans );
  }

  return out;
}

Tile *Tileset::findTileRecInTile( Tile *curTile, int depth, const QgsVector3D &tileCenter, const QgsCoordinateTransform *coordTrans )
{
  Tile *out = NULL;

  if ( depth == curTile->mDepth )
  {
    out = curTile;
  }
  else
  {
    if ( curTile->mChildren.isEmpty() ) // no children
    {
      if ( curTile->mContent.mSubContent->mType == ThreeDTilesContent::tileset )
      {
        out = findTileRecInTileset( ( Tileset * )( curTile->mContent.mSubContent.get() ), depth + 1, tileCenter, coordTrans );

      }
      else if ( curTile->mContent.mSubContent->mType == ThreeDTilesContent::b3dm )
      {
        out = curTile;
      }

    }
    else     // scan children
    {
      for ( std::shared_ptr<Tile> child : curTile->mChildren )
      {
        if ( child->contains( tileCenter, coordTrans ) )
        {
          out = findTileRecInTile( child.get(), depth, tileCenter, coordTrans );
          break;
        }
      }
    }
  }

  return out;
}

Tile *Tileset::findTileRecInTileset( Tileset *curTs, int depth, const QgsVector3D &tileCenter, const QgsCoordinateTransform *coordTrans )
{
  Tile *out = NULL;
  Tile *curTile;

  if ( depth == curTs->mDepth )
  {
    out = curTs->mRoot.get();
  }
  else
  {
    curTile = curTs->mRoot.get();
    if ( curTile->contains( tileCenter, coordTrans ) )
    {
      out = findTileRecInTile( curTile, depth, tileCenter, coordTrans );
    }
  }

  return out;
}

QgsChunkNodeId Tileset::encodeTileId( int tileLevel, const QgsAABB &tileBb, const QgsCoordinateTransform &coordinateTransform )
{
  buildRootBb( coordinateTransform );

  return QgsChunkNodeId( tileLevel,
                         ( tileBb.xCenter() - mRootBb->xCenter() ) * 1000.0,
                         ( tileBb.yCenter() - mRootBb->yCenter() ) * 1000.0,
                         ( tileBb.zCenter() - mRootBb->zCenter() ) * 1000.0 );
}

QgsVector3D Tileset::decodeTileId( const QgsChunkNodeId &tileId, const QgsCoordinateTransform &coordinateTransform )
{
  buildRootBb( coordinateTransform );

  return QgsVector3D( ( tileId.x / 1000 ) + mRootBb->xCenter(),
                      ( tileId.y / 1000 ) + mRootBb->yCenter(),
                      ( tileId.z / 1000 ) + mRootBb->zCenter() );
}

void Tileset::buildRootBb( const QgsCoordinateTransform &coordinateTransform )
{
  if ( mRootBb == NULL )
  {
    mRootBb = new QgsAABB( mRoot->getBoundingVolumeAsAABB( &coordinateTransform ) );
  }
}

// =======================
// B3dmHolder
// =======================
B3dmHolder::B3dmHolder( QIODevice &dev, const QUrl &url, Tile *parentTile, int depth ) :
  ThreeDTilesContent( b3dm, url, parentTile, depth ), mModel()
{
  mModel.mUrl = url;
  mModel.load( dev );
}

std::unique_ptr<ThreeDTilesContent> B3dmHolder::fromUrl( const QUrl &url, const QUrl &base, Tile *parentTile, int depth )
{
  QUrl u = base.resolved( url );
  if ( u.isLocalFile() || u.scheme().isEmpty() )
  {
    QFile loadFile( u.toString() );

    if ( !loadFile.open( QIODevice::ReadOnly ) )
    {
      LOGTHROW( critical, std::runtime_error,
                QString( "Couldn't open b3dm file: " + u.toString() ) );
    }

    return std::make_unique < B3dmHolder > ( loadFile, u, parentTile, depth );
  }
  else
  {
    LOGTHROW( critical, std::runtime_error,
              QString( "Couldn't open b3dm url " + u.toString() ) );
  }
}

// =======================
// QDebug OPERATORS
// =======================

QDebug &operator<<( QDebug &out, const Tileset &ts )
{
  out << "[Tileset";
  out << "version:" << ts.mAsset.mVersion;
  out << ", root:" << *ts.mRoot;
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

  if ( t.mContent.mSubContent != NULL )
  {
    out << ", url:" << t.mContent.mUrl;
    out << ", content:" << *( t.mContent.mSubContent );
  }

  if ( !t.mChildren.isEmpty() )
  {
    out << ", children: [";
    for ( const auto &c : t.mChildren )
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
