/***************************************************************************
  qgs3dtileschunkloader.cpp
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

#include "qgs3dtileschunkloader.h"

#include <QtConcurrent>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QBuffer>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DCore/QTransform>
#include "qgseventtracing.h"

///////////////

Qgs3dTilesChunkLoader::Qgs3dTilesChunkLoader( Qgs3dTilesChunkLoaderFactory *factory, QgsChunkNode *node )
  : QgsChunkLoader( node )
  , mFactory( factory )

{
  QgsDebugMsgLevel( QStringLiteral( "3DTiles loading entity %1" ).arg( node->tileId().text() ), 2 );

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
  Tile *tile = mFactory->mTile->mParentTileset->findTile( mNode->tileId(), &mFactory->mCoordinateTransform );
  qDebug() << "Qgs3dTilesChunkLoader: createEntity found tile url" << tile->mContent.mUrl;
  qDebug() << "Qgs3dTilesChunkLoader: createEntity found tile has" << tile->mChildren.length() << "children";
  qDebug() << "Qgs3dTilesChunkLoader: createEntity found tile has a content of type" << tile->mContent.mSubContent->mType;

  Qgs3dTilesChunkLoaderFactory *facto;
  facto = new Qgs3dTilesChunkLoaderFactory( mFactory->mMap, mFactory->mCoordinateTransform,
      tile );

  Qgs3dTilesChunkedEntity *entity = new Qgs3dTilesChunkedEntity( parent, tile, facto, true );

  return entity;
}

///////////////


Qgs3dTilesChunkLoaderFactory::Qgs3dTilesChunkLoaderFactory( const Qgs3DMapSettings &map,
    const QgsCoordinateTransform &coordinateTransform, Tile *tile )
  : mMap( map )
  , mCoordinateTransform( coordinateTransform )
  , mTile( tile )
{
  // noop
}

Qgs3dTilesChunkLoaderFactory::~Qgs3dTilesChunkLoaderFactory()
{
  // noop
}

QgsChunkLoader *Qgs3dTilesChunkLoaderFactory::createChunkLoader( QgsChunkNode *node ) const
{
  return new Qgs3dTilesChunkLoader( ( Qgs3dTilesChunkLoaderFactory * )this, node );
}

int Qgs3dTilesChunkLoaderFactory::primitivesCount( QgsChunkNode *node ) const
{
  return mTile->mParentTileset->findTile( node->tileId(), &mCoordinateTransform )->mChildren.length();
}


QgsChunkNode *Qgs3dTilesChunkLoaderFactory::createRootNode() const
{
  qDebug() << "Qgs3dTilesChunkedEntity: create ROOT NODE\n";
  QgsAABB bb = mTile->getBoundingVolumeAsAABB( &mCoordinateTransform );
  QgsChunkNodeId tileId = mTile->mParentTileset->encodeTileId( mTile->mDepth, bb, mCoordinateTransform );
  return new QgsChunkNode( tileId, bb, mTile->getGeometryError() );
}

void Qgs3dTilesChunkLoaderFactory::createChildrenRec( QVector<QgsChunkNode *> &out, Tile *tile, QgsChunkNode *node, int level ) const
{
  for ( std::shared_ptr<Tile> childTile : tile->mChildren )
  {
    QgsAABB bb = childTile->getBoundingVolumeAsAABB( &mCoordinateTransform );
    QgsChunkNodeId childId = mTile->mParentTileset->encodeTileId( level, bb, mCoordinateTransform );
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
  Tile *tile = mTile->mParentTileset->findTile( tileId, &mCoordinateTransform );
  createChildrenRec( children, tile, node, tileId.d + 1 );

  if ( tile->mChildren.length() == 0 && tile->mContent.mSubContent->mType == ThreeDTilesContent::tileset )
  {
    tile = ( ( Tileset * )tile->mContent.mSubContent.get() )->mRoot.get();
    createChildrenRec( children, tile, node, tileId.d + 1 );
  }
  return children;
}


///////////////



Qgs3dTilesChunkedEntity::Qgs3dTilesChunkedEntity( Qt3DCore::QEntity *parent, Tile *tile,
    Qgs3dTilesChunkLoaderFactory *factory, bool showBoundingBoxes )
  : QgsChunkedEntity( tile->getGeometryError()
                      , factory
  , false ), mSceneLoader( NULL )
{
  setUsingAdditiveStrategy( tile->mRefine ==  Refinement::ADD );
  setShowBoundingBoxes( showBoundingBoxes );

  // load data
  Tile *t = NULL;
  if ( tile->mContent.mSubContent->mType == ThreeDTilesContent::b3dm )
  {
    t = tile;
  }
  else if ( tile->mContent.mSubContent->mType == ThreeDTilesContent::tileset )
  {
    t = ( ( Tileset * )tile->mContent.mSubContent.get() )->mRoot.get();
    if ( t->mContent.mSubContent->mType == ThreeDTilesContent::tileset )
      return;
  }

  mBbox = t->getBoundingVolumeAsAABB( &factory->mCoordinateTransform );
  qDebug() << "NEW Qgs3dTilesChunkedEntity: bbox: "
           << mBbox.toString()
           << "\n";

  QString rootDirStr( "/tmp/3dtiles/" );
  QDir rootDir( rootDirStr );
  if ( !rootDir.exists() )
  {
    bool ret = rootDir.mkdir( rootDirStr );
    if ( !ret )
    {
      qDebug() <<  "Qgs3dTilesChunkedEntity: Failed to create directory:" << rootDir ;
      return;
    }
  }

  QString gltfDirStr = rootDir.absoluteFilePath( t->mContent.mUrl.fileName() );
  QDir gltfDir( gltfDirStr );
  if ( !gltfDir.exists() )
  {
    bool ret = gltfDir.mkdir( gltfDirStr );
    if ( !ret )
    {
      qDebug() <<  "Qgs3dTilesChunkedEntity: Failed to create directory:" << gltfDir ;
      return;
    }
  }

  QString gltfFilePath = gltfDir.absoluteFilePath( t->mContent.mUrl.fileName() + ".json" );
  setObjectName( QString( "Chunk " ) + t->mContent.mUrl.fileName() );
  QFile f( gltfFilePath );
  if ( !f.exists() )
  {

    B3dmHolder *h = ( B3dmHolder * )t->mContent.mSubContent.get();
    if ( h->mModel.getGltfModel() == NULL )
    {
      qDebug() <<  "Qgs3dTilesChunkedEntity: Failed to read gltf data:" << gltfFilePath ;
      gltfDir.removeRecursively();
      return;
    }
    tinygltf::TinyGLTF writer;
    bool ret = writer.WriteGltfSceneToFile( h->mModel.getGltfModel(), gltfFilePath.toStdString(), false, false, true, false ); //, true, true, true, false);
    if ( !ret )
    {
      qDebug() <<  "Qgs3dTilesChunkedEntity: Failed to write gltf file:" << gltfFilePath ;
      gltfDir.removeRecursively();
      return;
    }
    qDebug() <<  "Qgs3dTilesChunkedEntity: Created gltf file:" << gltfFilePath ;
  }
  else
  {
    qDebug() <<  "Qgs3dTilesChunkedEntity: Already created gltf file:" << gltfFilePath ;
  }

  QUrl gltfUrl = QUrl::fromLocalFile( gltfFilePath );
  mSceneLoader = new Qt3DRender::QSceneLoader( this );
  mSceneLoader->setObjectName( gltfFilePath );
  addComponent( mSceneLoader );
  mSceneLoader->setSource( gltfUrl );
  mSceneLoader->setEnabled( true );

  connect( mSceneLoader, &Qt3DRender::QSceneLoader::statusChanged, this, &Qgs3dTilesChunkedEntity::loaderChanged );

  setEnabled( true );
  setParent( parent );
}

void Qgs3dTilesChunkedEntity::loaderChanged( Qt3DRender::QSceneLoader::Status status )
{
  qDebug() <<  "Qgs3dTilesChunkedEntity: LOADER " << mSceneLoader->objectName() << ">>>>>>";
  qDebug() <<  "   STATUS:" << status;
  if ( mSceneLoader != NULL && status == Qt3DRender::QSceneLoader::Status::Ready )
  {

    // Torus shape data
    //! [0]
    Qt3DExtras::QTorusMesh *m_torus = new Qt3DExtras::QTorusMesh();
    double r = std::fmin( mBbox.xMax - mBbox.xCenter(),
                          std::fmin( mBbox.yMax - mBbox.yCenter(), mBbox.zMax - mBbox.zCenter() ) );
    m_torus->setRadius( r );
    m_torus->setMinorRadius( r * 0.5 );
    m_torus->setRings( 100 );
    m_torus->setSlices( 20 );
    //! [0]

    // TorusMesh Transform
    //! [1]
    Qt3DCore::QTransform *torusTransform = new Qt3DCore::QTransform();
    torusTransform->setScale( .75f );
    //torusTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 25.0f));
    torusTransform->setTranslation( QVector3D( mBbox.xCenter(), mBbox.yCenter(), mBbox.zCenter() ) );
    //! [1]

    /*  //! [2]
    Qt3DExtras::QPhongMaterial *torusMaterial = new Qt3DExtras::QPhongMaterial();
    torusMaterial->setDiffuse(QColor(QRgb(0xbeb32b)));
    //! [2]

    // Torus
    //! [3]
    Qt3DCore::QEntity * m_torusEntity = new Qt3DCore::QEntity(mLoader);
    m_torusEntity->addComponent(m_torus);
    m_torusEntity->addComponent(torusMaterial);
    m_torusEntity->addComponent(torusTransform);
    */

    qDebug() <<  "   ENTITIES" << mSceneLoader->entityNames().size() << "elements:";
    for ( auto entName : mSceneLoader->entityNames() )
    {
      Qt3DCore::QEntity *ent = mSceneLoader->entity( entName );
      qDebug() <<  "      -" << entName << ":" << ent;

      ent->addComponent( m_torus );
      ent->addComponent( torusTransform );

      for ( auto subent : ent->components() )
      {
        qDebug() <<  "        *" << subent;
        if ( dynamic_cast<Qt3DRender::QGeometryRenderer *>( subent ) )
        {
          Qt3DRender::QGeometryRenderer *geomR = dynamic_cast<Qt3DRender::QGeometryRenderer *>( subent );
          geomR->setEnabled( true );
          qDebug() <<  "           t" << geomR->primitiveType();
          for ( auto att : geomR->geometry()->attributes() )
          {
            qDebug() <<  "           a" << att->name() << "\tvs:" << att->vertexSize() << "\tc:" << att->count();
            if ( att->name().compare( "vertexPosition" ) == 0 )
            {
              const float *ptrFloat = reinterpret_cast<const float *>( att->buffer()->data().constData() );
              for ( int i = 0; i < 10 && i < ( int ) att->count(); i++ )
              {
                qDebug() <<  "              pos (" << *( ptrFloat + 0 ) << "" << *( ptrFloat + 1 ) << "" << *( ptrFloat + 2 ) << ")";
                ptrFloat += 3;
              }

            }
          }

        }
      }
    }
  }
  qDebug() <<  "Qgs3dTilesChunkedEntity: <<<<<<";
}

Qgs3dTilesChunkedEntity::~Qgs3dTilesChunkedEntity()
{
  // cancel / wait for jobs
  cancelActiveJobs();
  disconnect( mSceneLoader, &Qt3DRender::QSceneLoader::statusChanged, this, &Qgs3dTilesChunkedEntity::loaderChanged );
}

/// @endcond
