/***************************************************************************
  qgs3dtileschunkloader_p.cpp
  --------------------------------------
  Date                 : October 2020
  Copyright            : (C) 2020 by Peter Petrik
  Email                : zilolv dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3dtileschunkloader.h"

#include "qgs3dutils.h"
#include "qgschunknode_p.h"
#include "qgslogger.h"
#include "3dtiles.h"
#include "qgspointcloudindex.h"
#include "qgseventtracing.h"

#include "qgspoint3dsymbol.h"
#include "qgsphongmaterialsettings.h"

#include "qgspointcloud3dsymbol.h"
#include "qgsapplication.h"
#include "qgs3dsymbolregistry.h"
#include "qgspointcloudattribute.h"
#include "qgspointcloudrequest.h"
#include "qgscolorramptexture.h"
#include "qgspointcloud3dsymbol_p.h"

#include <QtConcurrent>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QGraphicsApiFilter>
#include <QPointSize>
#include <Qt3DRender/QSceneLoader>
#include <Qt3DExtras/QPhongMaterial>

#include "qgschunkboundsentity_p.h"
///@cond PRIVATE


///////////////

Qgs3dTilesChunkLoader::Qgs3dTilesChunkLoader( const Qgs3dTilesChunkLoaderFactory *factory, QgsChunkNode *node  )
  : QgsChunkLoader( node )
  , mFactory( factory )

{
  //QgsChunkNodeId nodeId = node->tileId();

  QgsDebugMsgLevel( QStringLiteral( "loading entity %1" ).arg( node->tileId().text() ), 2 );


  //
  // this will be run in a background thread
  //
  /*
  QFuture<void> future = QtConcurrent::run( [pc, pcNode, this]
  {
    QgsEventTracing::ScopedEvent e( QStringLiteral( "3D" ), QStringLiteral( "PC chunk load" ) );

    if ( mCanceled )
    {
      QgsDebugMsgLevel( QStringLiteral( "canceled" ), 2 );
      return;
    }
    mHandler->processNode( pc, pcNode, mContext );
  } );

  // emit finished() as soon as the handler is populated with features
  mFutureWatcher = new QFutureWatcher<void>( this );
  mFutureWatcher->setFuture( future );
  connect( mFutureWatcher, &QFutureWatcher<void>::finished, this, &QgsChunkQueueJob::finished );
*/
}

Qgs3dTilesChunkLoader::~Qgs3dTilesChunkLoader()
{
    /*
  if ( mFutureWatcher && !mFutureWatcher->isFinished() )
  {
    disconnect( mFutureWatcher, &QFutureWatcher<void>::finished, this, &QgsChunkQueueJob::finished );
    mFutureWatcher->waitForFinished();
  }*/
}

void Qgs3dTilesChunkLoader::cancel()
{
  mCanceled = true;
}

Qt3DCore::QEntity *Qgs3dTilesChunkLoader::createEntity( Qt3DCore::QEntity *parent )
{
  Tile * tile = mFactory->mTileset->findTile(mNode->tileId(), &mFactory->mCoordinateTransform);
  Qgs3dTilesChunkedEntity *entity = new Qgs3dTilesChunkedEntity( parent, tile, mFactory->mMap, mFactory->mCoordinateTransform, true);
  return entity;
}

///////////////


Qgs3dTilesChunkLoaderFactory::Qgs3dTilesChunkLoaderFactory( const Qgs3DMapSettings &map, const QgsCoordinateTransform &coordinateTransform, Tileset *tileset )
  : mMap( map )
  , mCoordinateTransform( coordinateTransform )
  , mTileset(tileset)
{
    // noop
}

Qgs3dTilesChunkLoaderFactory::~Qgs3dTilesChunkLoaderFactory() {
    // noop
}

QgsChunkLoader *Qgs3dTilesChunkLoaderFactory::createChunkLoader( QgsChunkNode *node ) const
{
  return new Qgs3dTilesChunkLoader( this, node );
}

int Qgs3dTilesChunkLoaderFactory::primitivesCount( QgsChunkNode *node ) const
{
  return mTileset->findTile(node->tileId(), &mCoordinateTransform)->mChildren.length();
}


QgsChunkNode *Qgs3dTilesChunkLoaderFactory::createRootNode() const
{
    QgsAABB bb = mTileset->mRoot->getBoundingVolumeAsAABB(&mCoordinateTransform);
    return new QgsChunkNode( QgsChunkNodeId( 0, 0, 0, 0 ), bb, mTileset->mGeometricError );
}

QVector<QgsChunkNode *> Qgs3dTilesChunkLoaderFactory::createChildren( QgsChunkNode *node ) const
{
    QVector<QgsChunkNode *> children;
    QgsChunkNodeId tileId = node->tileId();

    Tile * tile = mTileset->findTile(tileId, &mCoordinateTransform);
    int i = 0;
    for ( std::shared_ptr<Tile> subTile : tile->mChildren)
    {
        QgsAABB bb = subTile->getBoundingVolumeAsAABB(&mCoordinateTransform);
        QgsChunkNodeId childId( tileId.d + 1, bb.xCenter(), bb.yCenter(), bb.zCenter() );
        children << new QgsChunkNode( childId, bb, subTile->mGeometricError, node );
        i++;
    }
    return children;
}

///////////////



Qgs3dTilesChunkedEntity::Qgs3dTilesChunkedEntity( Qt3DCore::QEntity * parent, Tile * tile, const Qgs3DMapSettings &map, const QgsCoordinateTransform &coordinateTransform, bool showBoundingBoxes )
    : QgsChunkedEntity( tile->mGeometricError
                        , new Qgs3dTilesChunkLoaderFactory( map, coordinateTransform, tile->mParentTileset)
                        , false , 0, parent)
{
    setUsingAdditiveStrategy( tile->mRefine ==  Refinement::ADD);
    //setShowBoundingBoxes( showBoundingBoxes );

    if (true) {
        AABBMesh * bbox  = new AABBMesh();
        QList<QgsAABB> l;
        l << tile->getBoundingVolumeAsAABB(&coordinateTransform);
        bbox->setBoxes(l);
        addComponent(bbox);
        Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial;
        material->setAmbient( Qt::blue );
        addComponent( material );
    } else {

        // TODO: load data
        if (tile->mContent.mSubContent->mType == ThreeDTilesContent::b3dm) {
            Qt3DRender::QSceneLoader *loader = new Qt3DRender::QSceneLoader;
            loader->setSource( tile->mContent.mUrl );
            addComponent( loader );
        }
    }
}

Qgs3dTilesChunkedEntity::~Qgs3dTilesChunkedEntity()
{
  // cancel / wait for jobs
  //cancelActiveJobs();
}

/// @endcond
