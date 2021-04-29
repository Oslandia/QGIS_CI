/***************************************************************************
  qgspointcloudlayerchunkloader.h
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

#ifndef QGS3DTILESCHUNKLOADER_P_H
#define QGS3DTILESCHUNKLOADER_P_H

///@cond PRIVATE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QGIS API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//

#include "qgschunkloader_p.h"
#include "qgsfeature3dhandler_p.h"
#include "qgschunkedentity_p.h"
#include "3dtiles.h"

#include <Qt3DRender/QSceneLoader>
#include <QFutureWatcher>


#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief This loader factory is responsible for creation of loaders for individual tiles
 * of Qgs3dTilesChunkedEntity whenever a new tile is requested by the entity.
 *
 * \since QGIS 3.18
 */
class _3D_EXPORT Qgs3dTilesChunkLoaderFactory : public QgsChunkLoaderFactory
{
  public:

    /**
     * Constructs the factory
     * The factory takes ownership over the passed \a symbol
     */
    Qgs3dTilesChunkLoaderFactory( const Qgs3DMapSettings &map, const QgsCoordinateTransform &coordinateTransform,
                                  Tile *tile );

    virtual ~Qgs3dTilesChunkLoaderFactory();

    //! Creates loader for the given chunk node. Ownership of the returned is passed to the caller.
    virtual QgsChunkLoader *createChunkLoader( QgsChunkNode *node ) const ;
    virtual QgsChunkNode *createRootNode() const;
    virtual QVector<QgsChunkNode *> createChildren( QgsChunkNode *node ) const ;
    virtual int primitivesCount( QgsChunkNode *node ) const ;

  private:
    void createChildrenRec( QVector<QgsChunkNode *> &out, Tile *tile, QgsChunkNode *node, int level ) const;

  public:
    const Qgs3DMapSettings &mMap;
    QgsCoordinateTransform mCoordinateTransform;
    Tile *mTile;
};


/**
 * \ingroup 3d
 * \brief This loader class is responsible for async loading of data for a single tile
 * of Qgs3dTilesChunkedEntity and creation of final 3D entity from the data
 * previously prepared in a worker thread.
 *
 * \since QGIS 3.18
 */
class Qgs3dTilesChunkLoader : public QgsChunkLoader
{
  public:

    /**
     * Constructs the loader
     * Qgs3dTilesChunkLoader takes ownership over symbol
     */
    Qgs3dTilesChunkLoader( Qgs3dTilesChunkLoaderFactory *factory, QgsChunkNode *node );
    ~Qgs3dTilesChunkLoader() override;

    virtual void cancel() override;
    virtual Qt3DCore::QEntity *createEntity( Qt3DCore::QEntity *parent ) override;

  private:
    Qgs3dTilesChunkLoaderFactory *mFactory;
    bool mCanceled = false;
    QFutureWatcher<void> *mFutureWatcher = nullptr;
};


/**
 * \ingroup 3d
 * \brief 3D entity used for rendering of point cloud layers with a single 3D symbol for all points.
 *
 * It is implemented using tiling approach with QgsChunkedEntity. Internally it uses
 * Qgs3dTilesChunkLoaderFactory and Qgs3dTilesChunkLoader to do the actual work
 * of loading and creating 3D sub-entities for each tile.
 *
 * \since QGIS 3.18
 */
class Qgs3dTilesChunkedEntity : public QgsChunkedEntity
{
    Q_OBJECT
  public:
    Qgs3dTilesChunkedEntity( Qt3DCore::QEntity *parent, Tile *tile, Qgs3dTilesChunkLoaderFactory *factory,
                             bool showBoundingBoxes );

    virtual ~Qgs3dTilesChunkedEntity();

    void setGeometryError( double err ) { mTau = err;}

  private:
    Qt3DRender::QSceneLoader *mSceneLoader;
    QgsAABB mBbox;

  public slots:
    void loaderChanged( Qt3DRender::QSceneLoader::Status status );
};

/// @endcond

#endif // QGS3DTILESCHUNKLOADER_P_H
