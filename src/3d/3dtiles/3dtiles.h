/***************************************************************************
  3dtiles.h
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

#ifndef threedtiles_3dtiles_hpp_included_
#define threedtiles_3dtiles_hpp_included_

#include "qgsvector3d.h"
#include "qgsvector4d.h"
#include <QUrl>
#include <QMap>
#include <QJsonObject>
#include "qgsmatrix4x4.h"
#include "qgspoint.h"
#include "qgsgeometry.h"
#include "qgslinestring.h"
#include <QVector>
#include "qgscoordinatetransform.h"

#include "qgschunkedentity_p.h"
#include "b3dm.h"
#include "cachemanager.h"

namespace ThreeDTiles
{

  const QgsMatrix4x4 &buildFromJson( QgsMatrix4x4 &mat, const QJsonArray &array );

// from 3d tiles specifications:
  static const QgsMatrix4x4 flipToYUpMat( 1.0, 0.0,  0.0, 0.0,
                                          0.0, 0.0, 1.0, 0.0,
                                          0.0, -1.0,  0.0, 0.0,
                                          0.0, 0.0,  0.0, 1.0 );

  static const QgsMatrix4x4 flipToZUpMat( 1.0, 0.0,  0.0, 0.0,
                                          0.0, 0.0, -1.0, 0.0,
                                          0.0, 1.0,  0.0, 0.0,
                                          0.0, 0.0,  0.0, 1.0 );

  enum Refinement
  {
    REPLACE,
    ADD
  };

  class Tile;
  class Tileset;

  /**
   * @brief The Q3dCube class
   */
  class Q3dPrimitive
  {
    public:
      QVector<QgsVector4D> mPoints;

    public:
      Q3dPrimitive();
      void reproject( const QgsCoordinateTransform &coordTrans );
  };

  template<typename T, typename std::enable_if<std::is_base_of<Q3dPrimitive, T>::value>::type * = nullptr>
  T operator*( const QgsMatrix4x4 &matrix, T &in );

  /**
   * @brief The Q3dCube class
   */
  class Q3dCube : public Q3dPrimitive
  {

    public:
      Q3dCube();
      Q3dCube( const QgsVector3D &ll, const QgsVector3D &ur );

      QVector<QgsPoint> asQgsPoints() ;

      QgsVector4D ll() const { return mPoints[0];};
      QgsVector4D ur() const { return mPoints[6];};
  };

  /**
   * @brief base class for tile content
   */
  class ThreeDTilesContent
  {
    public:
      enum TileType
      {
        unmanaged,
        tileset,
        b3dm
      };
      TileType mType;
      QUrl mSource;
      int mDepth;
      Tile *mParentTile;

    public:
      ThreeDTilesContent();
      ThreeDTilesContent( const TileType &t, const QUrl &source, Tile *parentTile, int depth );

      virtual ~ThreeDTilesContent() = default;
  };


  /**
   * @brief Tileset description
   */
  class Asset
  {
    public:
      QString mVersion;

    public:
      Asset();

      const Asset &setFrom( const QJsonObject &obj );
  };


  /**
   * @brief base class for all bounding volume
   */
  class _3D_EXPORT BoundingVolume
  {
    public:
      enum BVType
      {
        unmanaged,
        box,
        region,
        sphere
      };
      BVType mType;
      QgsCoordinateReferenceSystem mEpsg;

    protected:
      bool checkCoordinateTransform( const QgsCoordinateTransform *coordTrans ) const;

    public:
      BoundingVolume( const BVType &t );
      virtual ~BoundingVolume() {}
      //BoundingVolume &operator=(const BoundingVolume &other);

      virtual Q3dCube asCube( const QgsMatrix4x4 &transform, const QgsCoordinateTransform *coordTrans = NULL ) = 0;
      virtual QgsGeometry asGeometry( const QgsMatrix4x4 &transform, const QgsCoordinateTransform *coordTrans = NULL );
      virtual QgsAABB asQgsAABB( const QgsMatrix4x4 &transform, const QgsCoordinateTransform *coordTrans = NULL, bool flipToYUpMat = false );
  };


  /**
   * @brief The Box BoundingVolume
   */
  class _3D_EXPORT Box: public BoundingVolume
  {
    public:
      QgsVector3D mCenter;
      QgsVector3D mU;
      QgsVector3D mV;
      QgsVector3D mW;

    public:
      Box();
      Box( const QJsonArray &value );
      virtual ~Box() {}

      QgsMatrix4x4 fromRotationTranslation();
      virtual Q3dCube asCube( const QgsMatrix4x4 &transform, const QgsCoordinateTransform *coordTrans = NULL );
  };


  /**
   * @brief The Sphere BoundingVolume
   */
  class _3D_EXPORT Sphere: public BoundingVolume
  {
    public:
      QgsVector3D mCenter;
      double mRadius;

    public:
      Sphere();
      Sphere( const QJsonArray &value );
      virtual ~Sphere() {}

      virtual Q3dCube asCube( const QgsMatrix4x4 &transform, const QgsCoordinateTransform *coordTrans = NULL );
  };

  struct Extents3
  {
    QgsVector3D mLl;
    QgsVector3D mUr;

    Extents3();
  };

  /**
   * @brief The Region BoundingVolume
   */
  class _3D_EXPORT Region: public BoundingVolume
  {
    public:
      Extents3 mExtents;

    public:
      Region();
      Region( const QJsonArray &value );
      virtual ~Region() {}

      virtual Q3dCube asCube( const QgsMatrix4x4 &transform, const QgsCoordinateTransform *coordTrans = NULL );
  };


  /**
   * @brief a 3D tile
   */
  class _3D_EXPORT Tile
  {
    public:

      /**
       * \brief always included in a tileset
       */
      Tileset *mParentTileset;

      /**
       * \brief only when tile is the child of another one
       */
      Tile *mParentTile;

      std::unique_ptr<BoundingVolume> mBv;
      Refinement mRefine;
      QgsMatrix4x4 mTransform;
      QUrl mContentUrl;
      double mGeometricError;
      int mDepth;

    private:
      std::unique_ptr<ThreeDTilesContent> mContent;

      // children part
      bool mLazyLoadChildren;
      QJsonArray mChildrenJsonArray;
      QList<std::shared_ptr<Tile>> mChildren;

      // transform part
      QgsMatrix4x4 *mCombinedTransform;

      QgsMatrix4x4 getCombinedTransformRec( Tile *p );
      QgsMatrix4x4 getCombinedTransformRec( Tileset *p );

    public:
      Tile();
      virtual ~Tile();
      Tile( const QJsonObject &obj, int depth, Tileset *parentTileset, Tile *parentTile = NULL );

      void setContentFrom( const QJsonObject &obj );
      ThreeDTilesContent *getContentConst() const;
      ThreeDTilesContent *getContent();

      const QList<std::shared_ptr<Tile>> children();
      const QList<std::shared_ptr<Tile>> children() const;

      /**
       * \brief contains
       * \param point in tile original CRS
       * \return TRUE if tile contains point
       */
      bool contains( const QgsVector3D &point );
      QgsGeometry getBoundingVolumeAsGeometry( const QgsCoordinateTransform *coordTrans = NULL );
      QgsAABB getBoundingVolumeAsAABB( const QgsCoordinateTransform *coordTrans = NULL );
      QgsMatrix4x4 *getCombinedTransform();
      double getGeometricError() const;
  };

  /**
   * @brief a tile content for B3dm
   */
  class B3dmHolder : public ThreeDTilesContent
  {
    public:
      B3dm mModel;

    public:
      B3dmHolder( QIODevice &dev, const QUrl &url, Tile *parentTile = NULL, int depth = 0 );
      virtual ~B3dmHolder() = default;

      static std::unique_ptr<ThreeDTilesContent> fromUrl( const QUrl &url, const QUrl &base = QUrl(), Tile *parentTile = NULL, int depth = 0 );

  };


  /**
   * @brief a tile content for json tile set
   */
  class _3D_EXPORT Tileset : public ThreeDTilesContent
  {
    public:
      Asset mAsset;
      double mGeometricError;
      QJsonObject mProperties;
      std::unique_ptr<Tile> mRootTile;

    private:
      QgsAABB *mRootBb;
      void buildRootBb();
      bool mCorrectTranslation;
      bool mFlipToYUp;
      bool mUseFakeMaterial;
      bool mUseOriginalGeomError;
      QString mName;

    public:
      Tileset( const QJsonObject &obj, const QUrl &url, Tile *parentTile = NULL, int depth = 0 );
      virtual ~Tileset();

      static std::unique_ptr<ThreeDTilesContent> fromUrl( const QUrl &url, const QUrl &base = QUrl(), Tile *parentTile = NULL, int depth = 0 );

      Tile *findTile( const QgsChunkNodeId &tileId );
      QgsChunkNodeId encodeTileId( int tileLevel, const QgsAABB &tileBb );
      QgsVector3D decodeTileId( const QgsChunkNodeId &tileId );
      double getGeometricError() const;

      Tileset const *getRootTileset() const;

      void setName( QString val );
      QString getName() const;

      void setFlipToYUp( bool flipY );
      bool getFlipToYUp() const;

      void setCorrectTranslation( bool correct );
      bool getCorrectTranslation() const;

      void setUseFakeMaterial( bool val );
      bool getUseFakeMaterial() const;

      void setUseOriginalGeomError( bool val );
      bool getUseOriginalGeomError() const;

      void createCacheDirectories( const QString &filename ) const;
      QString getCacheDirectory( const QString &filename ) const;

    private:
      Tile *findTileRecInTileset( Tileset *curTs, int depth, const QgsVector3D &tileCenter );
      Tile *findTileRecInTile( Tile *curTile, int depth, const QgsVector3D &tileCenter );
  };

}

Q_CORE_EXPORT QDebug &operator<<( QDebug &, const ThreeDTiles::B3dmHolder & );
Q_CORE_EXPORT QDebug &operator<<( QDebug &, const ThreeDTiles::Tile & );
Q_CORE_EXPORT QDebug &operator<<( QDebug &, const ThreeDTiles::Tileset & );
Q_CORE_EXPORT QDebug &operator<<( QDebug &, const ThreeDTiles::Q3dCube & );
Q_CORE_EXPORT QDebug &operator<<( QDebug &, const ThreeDTiles::ThreeDTilesContent & );
Q_CORE_EXPORT QDebug &operator<<( QDebug &, const ThreeDTiles::BoundingVolume & );
Q_CORE_EXPORT QDebug &operator<<( QDebug &, const ThreeDTiles::Box & );
Q_CORE_EXPORT QDebug &operator<<( QDebug &out, const ThreeDTiles::Sphere &t );
Q_CORE_EXPORT QDebug &operator<<( QDebug &out, const ThreeDTiles::Region &t );

using namespace ThreeDTiles;

#endif // 3dtiles_3dtiles_hpp_included_
