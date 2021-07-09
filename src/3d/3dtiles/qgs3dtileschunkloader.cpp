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
#include <QAttribute>

///////////////

Qgs3dTilesChunkLoader::Qgs3dTilesChunkLoader( Qgs3dTilesChunkLoaderFactory *factory, QgsChunkNode *node )
  : QgsChunkLoader( node )
  , mFactory( factory ), mSceneLoader( NULL ), mTile( NULL )

{
  QgsDebugMsgLevel( QStringLiteral( "3DTiles loading entity %1" ).arg( node->tileId().text() ), 2 );

  //
  // this will be run in a background thread
  //
  QgsChunkNodeId tileId = mNode->tileId();
  Tile *tile = mFactory->mRootTile->mParentTileset->findTile( mNode->tileId(), &mFactory->mCoordinateTransform );
  QFuture<void> future = QtConcurrent::run( [tile, tileId, this]
  {
    QgsEventTracing::ScopedEvent e( QStringLiteral( "3D" ), QStringLiteral( "3DTiles chunk load" ) );

    if ( mCanceled )
    {
      QgsDebugMsgLevel( QStringLiteral( "canceled" ), 2 );
      return;
    }

    prepareSceneLoader( tile );

    /*    if (mSceneLoader->status() != Qt3DRender::QSceneLoader::Status::Ready) {
            qWarning() << "Qgs3dTilesChunkLoader(QtConcurrent::run): ================ Timeout! Unable to load scene for tileId" << tileId.text();
        } else {
            qDebug() << "Qgs3dTilesChunkLoader(QtConcurrent::run): Scene loaded for tileId" << tileId.text();
        }*/
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

  if ( mSceneLoader )
    disconnect( mSceneLoader, &Qt3DRender::QSceneLoader::statusChanged, this, &Qgs3dTilesChunkLoader::sceneLoaderChanged );
}

void Qgs3dTilesChunkLoader::cancel()
{
  mCanceled = true;
}

Qt3DCore::QEntity *Qgs3dTilesChunkLoader::createEntity( Qt3DCore::QEntity *chunkedEntity )
{
  Tile *tile = mFactory->mRootTile->mParentTileset->findTile( mNode->tileId(), &mFactory->mCoordinateTransform );
  qDebug() << "Qgs3dTilesChunkLoader::createEntity for tileId" << mNode->tileId().text();
  qDebug() << "Qgs3dTilesChunkLoader::createEntity found tile url" << tile->mContent.mUrl;
  qDebug() << "Qgs3dTilesChunkLoader::createEntity found tile has" << tile->mChildren.length() << "children";
  qDebug() << "Qgs3dTilesChunkLoader::createEntity found tile has a content of type" << tile->mContent.mSubContent->mType;

  Qt3DCore::QEntity *sceneEntity = new Qt3DCore::QEntity( chunkedEntity );
// if (mSceneLoader->status() == Qt3DRender::QSceneLoader::Status::Ready) {
  finalizeEntity( sceneEntity );
  //}
  return sceneEntity;
}

void Qgs3dTilesChunkLoader::finalizeEntity( Qt3DCore::QEntity *sceneEntity )
{
  qDebug() << "Qgs3dTilesChunkLoader::finalizeEntity: Will add..."; // << mSceneLoader->entityNames().size() << "elements:";
  mSceneLoader = new Qt3DRender::QSceneLoader( sceneEntity );
  //mSceneLoader->setEnabled(true);
  connect( mSceneLoader, &Qt3DRender::QSceneLoader::statusChanged, this, &Qgs3dTilesChunkLoader::sceneLoaderChanged );
  mSceneLoader->setSource( mGltfUrl );

  // UGLY PART
  for ( int i = 0 ; i < 200 && mSceneLoader->status() != Qt3DRender::QSceneLoader::Status::Ready; i++ )
  {
    QCoreApplication::processEvents();
    QThread::yieldCurrentThread();
    QThread::msleep( 100 );
  }
  sceneEntity->setObjectName( QString( "Scene for " ) + mTile->mContent.mUrl.fileName() );
  mSceneLoader->setObjectName( QString( "Loader for " ) + mTile->mContent.mUrl.fileName() );
  for ( auto entName : mSceneLoader->entityNames() )
  {
    Qt3DCore::QEntity *ent = mSceneLoader->entity( entName );
    ent->setParent( sceneEntity );
  }
  /*mSceneLoader->setParent(sceneEntity);
  sceneEntity->addComponent(mSceneLoader);*/
  qDebug() << "Qgs3dTilesChunkLoader::finalizeEntity: added" << mSceneLoader->entityNames().size() << "elements:";
}

void Qgs3dTilesChunkLoader::prepareSceneLoader( Tile *tile )
{
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

  mTile = t;

  mBbox = t->getBoundingVolumeAsAABB( &mFactory->mCoordinateTransform );
  qDebug() << "Qgs3dTilesChunkLoader::prepareSceneLoader: bbox: "
           << mBbox.toString()
           << "\n";

  QString rootDirStr( "/tmp/3dtiles/" );
  QDir rootDir( rootDirStr );
  if ( !rootDir.exists() )
  {
    bool ret = rootDir.mkdir( rootDirStr );
    if ( !ret )
    {
      qDebug() <<  "Qgs3dTilesChunkLoader::prepareSceneLoader: Failed to create directory:" << rootDir ;
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
      qDebug() <<  "Qgs3dTilesChunkLoader::prepareSceneLoader: Failed to create directory:" << gltfDir ;
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
      qDebug() <<  "Qgs3dTilesChunkLoader::prepareSceneLoader: Failed to read gltf data:" << gltfFilePath ;
      gltfDir.removeRecursively();
      return;
    }
    tinygltf::TinyGLTF writer;
    bool ret = writer.WriteGltfSceneToFile( h->mModel.getGltfModel(), gltfFilePath.toStdString(), false, false, true, false ); //, true, true, true, false);
    if ( !ret )
    {
      qDebug() <<  "Qgs3dTilesChunkLoader::prepareSceneLoader: Failed to write gltf file:" << gltfFilePath ;
      gltfDir.removeRecursively();
      return;
    }
    qDebug() <<  "Qgs3dTilesChunkLoader::prepareSceneLoader: Created gltf file:" << gltfFilePath ;
  }
  else
  {
    qDebug() <<  "Qgs3dTilesChunkLoader::prepareSceneLoader: Already created gltf file:" << gltfFilePath ;
  }

  mGltfUrl = QUrl::fromLocalFile( gltfFilePath );
  /*    mSceneLoader->setObjectName( gltfFilePath );
      mSceneLoader->setEnabled(true);*/
}



void Qgs3dTilesChunkLoader::sceneLoaderChanged( Qt3DRender::QSceneLoader::Status status )
{
  qDebug() <<  "Qgs3dTilesChunkLoader::sceneLoaderChanged >>>>>>>" << mSceneLoader->objectName() << ">>>>>>";
  qDebug() <<  "   STATUS:" << status;
  if ( mSceneLoader && status == Qt3DRender::QSceneLoader::Status::Ready )
  {
    Qt3DCore::QTransform *torusTransform;
    Qt3DExtras::QPhongMaterial *torusMaterial = new Qt3DExtras::QPhongMaterial();

    qDebug() <<  "   ENTITIES" << mSceneLoader->entityNames().size() << "elements:";
    for ( auto entName : mSceneLoader->entityNames() )
    {
      Qt3DCore::QEntity *ent = mSceneLoader->entity( entName );
      qDebug() <<  "      -" << entName << ":" << ent;

      bool hasGeom = false;
      bool hasMaterial = false;
      for ( auto subent : ent->components() )
      {
        if ( dynamic_cast<Qt3DRender::QGeometryRenderer *>( subent ) )
        {
          hasGeom = true;
        }
        else if ( dynamic_cast<Qt3DRender::QMaterial *>( subent ) )
        {
          hasMaterial = true;
        }
      }

      for ( auto subent : ent->components() )
      {
        qDebug() <<  "        *" << subent;
        Qt3DRender::QGeometryRenderer *geomR = dynamic_cast<Qt3DRender::QGeometryRenderer *>( subent );
        if ( geomR )
        {
//          geomR->setEnabled( true );
          qDebug() <<  "           t" << geomR->primitiveType();
          bool hasNormal = false;
          for ( auto att : geomR->geometry()->attributes() )
          {
            if ( att->name().compare( "vertexNormal" ) == 0 )
            {
              hasNormal = true;
            }
          }
          for ( auto att : geomR->geometry()->attributes() )
          {
            qDebug() <<  "           a" << att->name() << "\tvs:" << att->vertexSize() << "\tc:" << att->count()  << "\tst:" << att->byteStride();
            if ( att->name().compare( "vertexPosition" ) == 0 )
            {
              float *ptrFloat = reinterpret_cast<float *>( att->buffer()->data().data() + att->byteOffset() );
              for ( int i = 0; i < 10 && i < ( int ) att->count(); i++ )
              {
                qDebug() <<  "              pos (" << *( ptrFloat + 0 ) << "" << *( ptrFloat + 1 ) << "" << *( ptrFloat + 2 ) << ")";
                ptrFloat += 3;
              }
              if ( !hasNormal )
              {
                Qt3DRender::QBuffer *b = new Qt3DRender::QBuffer();
                b->setData( QByteArray( att->buffer()->data().size(), 0 ) );
                Qt3DRender::QAttribute *norm = new Qt3DRender::QAttribute( b, "vertexNormal", att->vertexBaseType(),
                    att->vertexSize(), att->count(), att->byteOffset(),
                    att->byteStride() );
                float *ptrFloat = reinterpret_cast<float *>( norm->buffer()->data().data() + norm->byteOffset() );
                for ( int i = 0; i < ( int ) norm->count(); i++ )
                {
                  QVector3D v( 0.01, 0.01, -1.01 );
                  v.normalize();
                  *ptrFloat = v.x();
                  *( ptrFloat + 1 ) = v.y();
                  *( ptrFloat + 2 ) = v.z();
                  ptrFloat += 3;
                }
                geomR->geometry()->addAttribute( norm );
                qDebug() <<  "           a ++++ normal" ;
              }

            }
            else if ( att->name().compare( "vertexNormal" ) == 0 )
            {
              float *ptrFloat = reinterpret_cast<float *>( att->buffer()->data().data() + att->byteOffset() );
              for ( int i = 0; i < 10 && i < ( int ) att->count(); i++ )
              {
                /*
                *ptrFloat=*ptrFloat * -1.0;
                *(ptrFloat+1)=*(ptrFloat+1) * -1.0;
                *(ptrFloat+2)=*(ptrFloat+2) * -1.0;*/
                qDebug() <<  "              norm (" << *( ptrFloat + 0 ) << "" << *( ptrFloat + 1 ) << "" << *( ptrFloat + 2 ) << ")";
                ptrFloat += 3;
              }
            }
            else if ( att->name().compare( "" ) == 0 )
            {
              quint16 *ptrFloat = reinterpret_cast<quint16 *>( att->buffer()->data().data() + att->byteOffset() );
              for ( int i = 0; i < 10 && i < ( int ) att->count(); i++ )
              {
                /*
                *ptrFloat=*ptrFloat * -1.0;
                *(ptrFloat+1)=*(ptrFloat+1) * -1.0;
                *(ptrFloat+2)=*(ptrFloat+2) * -1.0;*/
                qDebug() <<  "              idx (" << *( ptrFloat + 0 ) << ")";
                ptrFloat += 1;
              }
            }
          }

          hasNormal = false;
          for ( auto att : geomR->geometry()->attributes() )
          {
            if ( att->name().compare( "vertexNormal" ) == 0 )
            {
              hasNormal = true;
            }
          }
          if ( !hasNormal )
          {
            qDebug() <<  "  ===============================================================  NO normal!" ;
          }

        }
      }

      if ( hasGeom )
      {
        torusTransform = new Qt3DCore::QTransform();
        torusTransform->setMatrix( mTile->getCombinedTransform()->toQMatrix4x4( NULL/*
                                        &((Qgs3dTilesChunkLoaderFactory*)mChunkLoaderFactory)->mCoordinateTransform*/ ) );
        //torusTransform->setScale( 100.0f );
        torusTransform->setTranslation( QVector3D( mBbox.xCenter(), mBbox.yCenter(), mBbox.zCenter() ) );
        ent->addComponent( torusTransform );
        qDebug() <<  "        * ++++ transform" << torusTransform->matrix();

        //if (!hasMaterial) {
        torusMaterial = new Qt3DExtras::QPhongMaterial();
        if ( mTile->mParentTile == NULL )
        {
          torusMaterial->setDiffuse( QColor( QRgb( 0x0000ff ) ) );
        }
        else
        {
          torusMaterial->setDiffuse( QColor( QRgb( 0xff0000 ) ) );
        }
        ent->addComponent( torusMaterial );
        qDebug() <<  "        * ++++ material";
        //}
      }
    }
  }
  qDebug() <<  "Qgs3dTilesChunkLoader::sceneLoaderChanged: <<<<<<";
}

///////////////


Qgs3dTilesChunkLoaderFactory::Qgs3dTilesChunkLoaderFactory( const Qgs3DMapSettings &map,
    const QgsCoordinateTransform &coordinateTransform, Tile *tile )
  : mMap( map )
  , mCoordinateTransform( coordinateTransform )
  , mRootTile( tile )
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
  return mRootTile->mParentTileset->findTile( node->tileId(), &mCoordinateTransform )->mChildren.length();
}


QgsChunkNode *Qgs3dTilesChunkLoaderFactory::createRootNode() const
{
  qDebug() << "Qgs3dTilesChunkLoaderFactory: create ROOT NODE\n";
  QgsAABB bb = mRootTile->getBoundingVolumeAsAABB( &mCoordinateTransform );
  QgsChunkNodeId tileId = mRootTile->mParentTileset->encodeTileId( mRootTile->mDepth, bb, mCoordinateTransform );
  return new QgsChunkNode( tileId, bb, mRootTile->getGeometryError() );
}

void Qgs3dTilesChunkLoaderFactory::createChildrenRec( QVector<QgsChunkNode *> &out, Tile *tile, QgsChunkNode *node, int level ) const
{
  for ( std::shared_ptr<Tile> childTile : tile->mChildren )
  {
    QgsAABB bb = childTile->getBoundingVolumeAsAABB( &mCoordinateTransform );
    QgsChunkNodeId childId = mRootTile->mParentTileset->encodeTileId( level, bb, mCoordinateTransform );
    QgsChunkNode *childNode = new QgsChunkNode( childId, bb, childTile->getGeometryError(), node );
    out << childNode;
    //createChildrenRec (out, childTile.get(), childNode, level + 1);
  }
}


QVector<QgsChunkNode *> Qgs3dTilesChunkLoaderFactory::createChildren( QgsChunkNode *node ) const
{
  QVector<QgsChunkNode *> children;
  QgsChunkNodeId tileId = node->tileId();

  qDebug() << "Qgs3dTilesChunkLoaderFactory: create children from tileId" << tileId.text();
  Tile *tile = mRootTile->mParentTileset->findTile( tileId, &mCoordinateTransform );

  if ( tile->mChildren.isEmpty() )
  {
    if ( tile->mContent.mSubContent->mType == ThreeDTilesContent::tileset )
    {
      Tile *subTilesetContent = ( ( Tileset * )tile->mContent.mSubContent.get() )->mRoot.get();
      createChildrenRec( children, subTilesetContent, node, tileId.d + 1 );
    }

  }
  else     // only children without direct sub tileset
  {
    createChildrenRec( children, tile, node, tileId.d + 1 );
  }

  return children;
}


///////////////



Qgs3dTilesChunkedEntity::Qgs3dTilesChunkedEntity( Qgs3dTilesChunkLoaderFactory *factory, Tile *tile, bool showBoundingBoxes )
  : QgsChunkedEntity( tile->getGeometryError(), factory, false )
{
  setUsingAdditiveStrategy( tile->mRefine ==  Refinement::ADD );
  setShowBoundingBoxes( showBoundingBoxes );

  /*mSceneLoader = new Qt3DRender::QSceneLoader(this);
  connect( mSceneLoader, &Qt3DRender::QSceneLoader::statusChanged, this, &Qgs3dTilesChunkedEntity::sceneLoaderChanged);
  mSceneLoader->setSource(QUrl("file:///tmp/3dtiles/dragon_low.b3dm/dragon_low.b3dm.json"));*/
}
/*
void Qgs3dTilesChunkedEntity::sceneLoaderChanged( Qt3DRender::QSceneLoader::Status s ){
     qDebug() << "================================ Qgs3dTilesChunkedEntity sceneloader" << s;
}
*/

Qgs3dTilesChunkedEntity::~Qgs3dTilesChunkedEntity()
{
  // cancel / wait for jobs
  cancelActiveJobs();
}

/// @endcond
