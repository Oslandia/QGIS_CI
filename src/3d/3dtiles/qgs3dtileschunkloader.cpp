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

Qgs3dTilesChunkLoader::Qgs3dTilesChunkLoader( Qgs3dTilesChunkLoaderFactory *factory, QgsChunkNode *node  )
    : QgsChunkLoader( node )
    , mFactory( factory )

{
    //QgsChunkNodeId nodeId = node->tileId();

    QgsDebugMsgLevel( QStringLiteral( "loading entity %1" ).arg( node->tileId().text() ), 2 );


    QgsChunkNodeId tileId = node->tileId();
    //
    // this will be run in a background thread
    //
    QFuture<void> future = QtConcurrent::run( [tileId, this]
    {
        QgsEventTracing::ScopedEvent e( QStringLiteral( "3D" ), QStringLiteral( "3DTiles chunk load" ) );

        if ( mCanceled )
        {
            QgsDebugMsgLevel( QStringLiteral( "canceled" ), 2 );
            return;
        }
        //QgsDebugMsgLevel( QStringLiteral( "canceled" ), 2 );
        //mHandler->processNode( pc, pcNode, mContext );
        qDebug() << "Qgs3dTilesChunkLoader: QUITTING QtConcurrent::run for tileId" << tileId.text();
    } );

    // emit finished() as soon as the handler is populated with features
    mFutureWatcher = new QFutureWatcher<void>( this );
    mFutureWatcher->setFuture( future );
    connect( mFutureWatcher, &QFutureWatcher<void>::finished, this, &QgsChunkQueueJob::finished );
}

Qgs3dTilesChunkLoader::~Qgs3dTilesChunkLoader()
{
    if ( mFutureWatcher && !mFutureWatcher->isFinished() )
    {
        disconnect( mFutureWatcher, &QFutureWatcher<void>::finished, this, &QgsChunkQueueJob::finished );
        mFutureWatcher->waitForFinished();
    }
}

void Qgs3dTilesChunkLoader::cancel()
{
    mCanceled = true;
}

Qt3DCore::QEntity *Qgs3dTilesChunkLoader::createEntity( Qt3DCore::QEntity *parent )
{
    qDebug() << "Qgs3dTilesChunkLoader: createEntity for tileId" << mNode->tileId().text();
    Tile * tile = mFactory->mTile->mParentTileset->findTile(mNode->tileId(), &mFactory->mCoordinateTransform);
    qDebug() << "Qgs3dTilesChunkLoader: createEntity tile have" << tile->mChildren.length() <<"children";
    qDebug() << "Qgs3dTilesChunkLoader: createEntity tile have a content of type" << tile->mContent.mSubContent->mType;

    Qgs3dTilesChunkLoaderFactory * facto;
//    if (tile->mChildren.length() == 0 && tile->mContent.mSubContent->mType == ThreeDTilesContent::tileset) {
        facto = new Qgs3dTilesChunkLoaderFactory(mFactory->mMap, mFactory->mCoordinateTransform,
                                                 tile);
  /*  } else {
        facto = mFactory;
    }*/

    Qgs3dTilesChunkedEntity *entity = new Qgs3dTilesChunkedEntity( parent, tile, facto, true);

    return entity;
}

///////////////


Qgs3dTilesChunkLoaderFactory::Qgs3dTilesChunkLoaderFactory( const Qgs3DMapSettings &map,
                                                            const QgsCoordinateTransform &coordinateTransform, Tile *tile )
    : mMap( map )
    , mCoordinateTransform( coordinateTransform )
    , mTile(tile)
{
    // noop
}

Qgs3dTilesChunkLoaderFactory::~Qgs3dTilesChunkLoaderFactory() {
    // noop
}

QgsChunkLoader *Qgs3dTilesChunkLoaderFactory::createChunkLoader( QgsChunkNode *node ) const
{
    return new Qgs3dTilesChunkLoader( (Qgs3dTilesChunkLoaderFactory *)this, node );
}

int Qgs3dTilesChunkLoaderFactory::primitivesCount( QgsChunkNode *node ) const
{
    return mTile->mParentTileset->findTile(node->tileId(), &mCoordinateTransform)->mChildren.length();
}


QgsChunkNode *Qgs3dTilesChunkLoaderFactory::createRootNode() const
{
    qDebug() << "Qgs3dTilesChunkedEntity: create ROOT NODE\n";
    QgsAABB bb = mTile->getBoundingVolumeAsAABB(&mCoordinateTransform);
    QgsChunkNodeId tileId = mTile->mParentTileset->encodeTileId(mTile->mDepth, bb, mCoordinateTransform);
    return new QgsChunkNode( tileId, bb, mTile->getGeometryError() );
}

void Qgs3dTilesChunkLoaderFactory::createChildrenRec (QVector<QgsChunkNode *> & out, Tile * tile, QgsChunkNode *node, int level) const
{
    for ( std::shared_ptr<Tile> childTile : tile->mChildren)
    {
        QgsAABB bb = childTile->getBoundingVolumeAsAABB(&mCoordinateTransform);
        QgsChunkNodeId childId = mTile->mParentTileset->encodeTileId(level, bb, mCoordinateTransform);
        QgsChunkNode *childNode = new QgsChunkNode( childId, bb, childTile->getGeometryError(), node );
        out << childNode;
        //createChildrenRec (out, childTile.get(), childNode, level + 1);
    }
}


QVector<QgsChunkNode *> Qgs3dTilesChunkLoaderFactory::createChildren( QgsChunkNode *node ) const
{
    QVector<QgsChunkNode *> children;
    QgsChunkNodeId tileId = node->tileId();

    qDebug() << "Qgs3dTilesChunkedEntity: create children from tileId" << tileId.text();
    Tile * tile = mTile->mParentTileset->findTile(tileId, &mCoordinateTransform);
    createChildrenRec (children, tile, node, tileId.d + 1);
    //int i = 0;
  /*  for ( std::shared_ptr<Tile> childTile : tile->mChildren)
    {
        QgsAABB bb = childTile->getBoundingVolumeAsAABB(&mCoordinateTransform);
        QgsChunkNodeId childId = mTile->mParentTileset->encodeTileId(tileId.d + 1, bb, mCoordinateTransform);
        children << new QgsChunkNode( childId, bb, childTile->getGeometryError(), node );
        //i++;
    }*/

    if (tile->mChildren.length() == 0 && tile->mContent.mSubContent->mType == ThreeDTilesContent::tileset) {
        tile = ((Tileset*)tile->mContent.mSubContent.get())->mRoot.get();
        createChildrenRec (children, tile, node, tileId.d + 1);
    }
    return children;
}


///////////////



Qgs3dTilesChunkedEntity::Qgs3dTilesChunkedEntity( Qt3DCore::QEntity * parent, Tile * tile,
                                                  Qgs3dTilesChunkLoaderFactory *factory, bool showBoundingBoxes )
    : QgsChunkedEntity( tile->getGeometryError()
                        , factory
                        , false)
{
    setUsingAdditiveStrategy( tile->mRefine ==  Refinement::ADD);
    setShowBoundingBoxes( showBoundingBoxes );

    qDebug() << "NEW Qgs3dTilesChunkedEntity: bbox: "
             << tile->getBoundingVolumeAsAABB(&factory->mCoordinateTransform).toString()
             <<"\n";
    if (true) {
        AABBMesh * bbox  = new AABBMesh();
        QList<QgsAABB> l;
        l << tile->getBoundingVolumeAsAABB(&factory->mCoordinateTransform);
        bbox->setBoxes(l);
        bbox->setEnabled(true);
        addComponent(bbox);

        Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial;
        material->setAmbient( Qt::red );
        material->setEnabled(true);
        material->setDiffuse(Qt::red);
        addComponent( material );

        setEnabled(true);
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
    cancelActiveJobs();
}

/// @endcond
