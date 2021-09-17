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
#include <Qt3DRender/qgeometryrenderer.h>
#include <Qt3DCore/QTransform>
#include "qgseventtracing.h"
#include <QAttribute>
#include "qgs3dutils.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

Qgs3dTilesChunkLoader::Qgs3dTilesChunkLoader( Qgs3dTilesChunkLoaderFactory *factory, QgsChunkNode *node )
  : QgsChunkLoader( node )
  , mFactory( factory ), mSceneLoader( NULL ), mTile( NULL )

{
  QgsDebugMsgLevel( QStringLiteral( "3DTiles loading entity %1" ).arg( node->tileId().text() ), 2 );

  //
  // this will be run in a background thread
  //
  Tile *tile = mFactory->mRootTile->mParentTileset->findTile( mNode->tileId() );
  QFuture<void> future = QtConcurrent::run( [tile,  this]
  {
    QgsEventTracing::ScopedEvent e( QStringLiteral( "3D" ), QStringLiteral( "3DTiles chunk load" ) );

    if ( mCanceled )
    {
      QgsDebugMsgLevel( QStringLiteral( "canceled" ), 2 );
      return;
    }

    prepareSceneLoader( tile );
  } );

  // emit finished() as soon as the handler is populated with features
  mFutureWatcher = new QFutureWatcher<void>( this );
  mFutureWatcher->setFuture( future );
  connect( mFutureWatcher, &QFutureWatcher<void>::finished, this, &QgsChunkQueueJob::finished );
}

Qgs3dTilesChunkLoader::~Qgs3dTilesChunkLoader()
{
  qDebug() << mNode->tileId().text() << "ChunkLoader::~Qgs3dTilesChunkLoader(): destruction..." ;
  if ( mFutureWatcher && !mFutureWatcher->isFinished() )
  {
    disconnect( mFutureWatcher, &QFutureWatcher<void>::finished, this, &QgsChunkQueueJob::finished );
    mFutureWatcher->waitForFinished();
  }

  //if ( mSceneLoader )
  //  disconnect( mSceneLoader, &Qt3DRender::QSceneLoader::statusChanged, this, &Qgs3dTilesChunkLoader::sceneLoaderChanged );
  qDebug() << mNode->tileId().text() << "ChunkLoader::~Qgs3dTilesChunkLoader(): destruction Done!" ;
}

void Qgs3dTilesChunkLoader::cancel()
{
  mCanceled = true;
}

void Qgs3dTilesChunkLoader::prepareSceneLoader( Tile *tile )
{
  if ( tile == nullptr ) // when called for root node
    return;

  if ( mFactory->mCreatedEntity.contains( mNode->tileId().text() ) )
  {
    if ( mTile == NULL )
      qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader using cache for tile NULLLL";
    else
      qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader using cache for tile url" << mTile->mContentUrl;
    return;
  }

  // load data
  Tile *t = nullptr;
  if ( tile->getContent()->mType == ThreeDTilesContent::b3dm )
  {
    t = tile;
  }
  else if ( tile->getContent()->mType == ThreeDTilesContent::tileset )
  {
    t = ( ( Tileset * )tile->getContent() )->mRootTile.get();
    if ( t->getContent()->mType == ThreeDTilesContent::tileset )
    {
      qWarning() << "WTF?";
      return;
    }
  }

  mTile = t;
  Q_ASSERT( mTile != NULL );
  //mTile = tile;

  mBbox = mTile->getBoundingVolumeAsAABB( &mFactory->mCoordinateTransform );
  qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader: " << mFactory->mRootTile->mParentTileset->encodeTileId( mTile->mDepth, mBbox ).text();
  qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader: bbox: "
           << mBbox.toString()
           << "\n";

  mTile->mParentTileset->createCacheDirectories( mTile->mContentUrl.fileName() );

  QString gltfDirStr = mTile->mParentTileset->getCacheDirectory( mTile->mContentUrl.fileName() ) ;
  QDir gltfDir( gltfDirStr );
  /*if ( !gltfDir.exists() )
  {
    if ( !gltfDir.mkdir( gltfDirStr ) )
    {
      if ( gltfDir.exists() )
      {
        // created by another thread
      }
      else
      {
        qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader: Failed to create directory:" << gltfDir ;
        return;
      }
    }
  }*/

  QString gltfFilePath = gltfDir.absoluteFilePath( mTile->mContentUrl.fileName() + ".json" );
  setObjectName( mNode->tileId().text() + "Chunk " + mTile->mContentUrl.fileName() );
  QFile f( gltfFilePath );
  if ( !f.exists() )
  {
    B3dmHolder *h = ( B3dmHolder * )mTile->getContent();
    if ( h->mModel.getGltfModel() == NULL )
    {
      qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader: Failed to read gltf data:" << gltfFilePath ;
      gltfDir.removeRecursively();
      return;
    }
    tinygltf::TinyGLTF writer;
    bool ret = writer.WriteGltfSceneToFile( h->mModel.getGltfModel(), gltfFilePath.toStdString(), false, false, true, false ); //, true, true, true, false);
    if ( !ret )
    {
      qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader: Failed to write gltf file:" << gltfFilePath ;
      gltfDir.removeRecursively();
      return;
    }
    qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader: Created gltf file:" << gltfFilePath ;
  }
  else
  {
    qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader: Already created gltf file:" << gltfFilePath ;
  }

  mGltfUrl = QUrl::fromLocalFile( gltfFilePath );
  qDebug() << mNode->tileId().text() << "ChunkLoader::prepareSceneLoader mGltfUrl:" << mGltfUrl;
}

Qt3DCore::QEntity *Qgs3dTilesChunkLoader::createEntity( Qt3DCore::QEntity *chunkedEntity )
{
  if ( mFactory->mCreatedEntity.contains( mNode->tileId().text() ) )
  {
    if ( mTile == NULL )
      qDebug() << mNode->tileId().text() << "ChunkLoader::createEntity using cache for tile NULLLL";
    else
      qDebug() << mNode->tileId().text() << "ChunkLoader::createEntity using cache for tile url" << mTile->mContentUrl;
    return mFactory->mCreatedEntity[mNode->tileId().text()];
  }

  Qt3DCore::QEntity *sceneEntity = new Qt3DCore::QEntity( chunkedEntity );
  Tile *tile = mFactory->mRootTile->mParentTileset->findTile( mNode->tileId() );
  if ( tile == nullptr ) // when called for root node
  {
    sceneEntity->setObjectName( "=>fake root entity" );
    sceneEntity->setEnabled( false );
  }
  else
  {

    qDebug() << mNode->tileId().text() << "ChunkLoader::createEntity found tile gltf" << mGltfUrl;
    qDebug() << mNode->tileId().text() << "ChunkLoader::createEntity found tile url" << mTile->mContentUrl;
    qDebug() << mNode->tileId().text() << "ChunkLoader::createEntity found tile has" << mTile->children().length() << "children";
    qDebug() << mNode->tileId().text() << "ChunkLoader::createEntity found tile has a content of type" << mTile->getContent()->mType;

    sceneEntity->setObjectName( QString( "=>Scene for " ) + mTile->mContentUrl.fileName() );

    finalizeEntity( sceneEntity );
    sceneEntity->setEnabled( true );

    if ( mSceneLoader->status() == Qt3DRender::QSceneLoader::Status::Ready )
    {
      auto entName = *( mSceneLoader->entityNames().begin() );
      Qt3DCore::QEntity *ent = mSceneLoader->entity( entName );
      if ( ent != nullptr )
      {
        ent->setObjectName( "===>useful entity: " + entName );
        ent->setParent( sceneEntity );
      }
    }

    // TODO: WHY?
    /*
    if ( chunkedEntity->children().length() == 2 )
    {
      ( ( Qt3DCore::QEntity * )( chunkedEntity->children()[1] ) )->setEnabled( true );
    }
    */
  }

  mFactory->mCreatedEntity[mNode->tileId().text()] = sceneEntity;

  return sceneEntity;
}




void Qgs3dTilesChunkLoader::finalizeEntity( Qt3DCore::QEntity *sceneEntity )
{
  qDebug() << mNode->tileId().text() << "ChunkLoader::finalizeEntity: Will run scene loader... for:" << mGltfUrl;
  mSceneLoader = new Qt3DRender::QSceneLoader( sceneEntity );
  connect( mSceneLoader, &Qt3DRender::QSceneLoader::statusChanged, this, &Qgs3dTilesChunkLoader::sceneLoaderChanged );

  mSceneLoader->setObjectName( QString( "===>Loader for " ) + mTile->mContentUrl.fileName() );
  mSceneLoader->setSource( mGltfUrl );  // <== event should be procced! Must call sceneLoaderChanged

  // UGLY PART to force event to be procced
  for ( int i = 0 ; i < 200 && mSceneLoader->status() != Qt3DRender::QSceneLoader::Status::Ready; i++ )
  {
    QCoreApplication::processEvents();
    //    QThread::yieldCurrentThread();
    QThread::msleep( 50 );
  }

  if ( mSceneLoader->status() == Qt3DRender::QSceneLoader::Status::Ready )
  {
    qDebug() << mNode->tileId().text() << "ChunkLoader::finalizeEntity: added" << mSceneLoader->entityNames().size() << "elements:";
  }
  else
  {
    qWarning() << mNode->tileId().text() << "ChunkLoader::finalizeEntity: failed to finalize entity!";
  }

  qDebug() << mNode->tileId().text() << "ChunkLoader::finalizeEntity: Done for:" << mGltfUrl;
  mSceneLoader->setEnabled( false );
}




void Qgs3dTilesChunkLoader::sceneLoaderChanged( Qt3DRender::QSceneLoader::Status status )
{
  qDebug() << mNode->tileId().text() << "ChunkLoader::sceneLoaderChanged >>>>>>>" << mSceneLoader->objectName() << ">>>>>>";
  qDebug() << mNode->tileId().text() << "   STATUS:" << status;
  if ( mSceneLoader && status == Qt3DRender::QSceneLoader::Status::Ready )
  {
    Qt3DCore::QTransform *aTransform;
    Qt3DExtras::QPhongMaterial *aMaterial = new Qt3DExtras::QPhongMaterial();

    qDebug() << mNode->tileId().text() << "   ENTITIES" << mSceneLoader->entityNames().size() << "elements:";
    for ( auto entName : mSceneLoader->entityNames() )
    {
      Qt3DCore::QEntity *ent = mSceneLoader->entity( entName );
      qDebug() << mNode->tileId().text() << "      -" << entName << ":" << ent;

      bool hasGeom = false;
      bool hasMaterial = false;
      int idxMat = 0;
      int idxComp = 0;
      for ( Qt3DCore::QComponent *subent : ent->components() )
      {
        if ( dynamic_cast<Qt3DRender::QGeometryRenderer *>( subent ) )
        {
          hasGeom = true;
        }
        else if ( dynamic_cast<Qt3DRender::QMaterial *>( subent ) )
        {
          hasMaterial = true;
          idxMat = idxComp;
        }
        idxComp++;
      }

      if ( hasMaterial && mTile->mParentTileset->getUseFakeMaterial() )
      {
        ent->components().remove( idxMat );
        hasMaterial = false;
        qDebug() << mNode->tileId().text() << "        Removed found material at:" << idxMat;
      }

      for ( auto subent : ent->components() )
      {
        qDebug() << mNode->tileId().text() << "        *" << subent;
        Qt3DRender::QGeometryRenderer *geomR = dynamic_cast<Qt3DRender::QGeometryRenderer *>( subent );
        if ( geomR )
        {
          qDebug() << mNode->tileId().text() << "           t" << geomR->primitiveType();
          bool hasNormal = false;
          Qt3DRender::QAttribute *facesByVertexIdAtt = NULL;

          for ( auto att : geomR->geometry()->attributes() )
          {
            if ( att->name().compare( Qt3DRender::QAttribute::defaultNormalAttributeName() ) == 0 )
            {
              hasNormal = true;
            }
            else if ( att->name().compare( "" ) == 0 )
            {
              facesByVertexIdAtt = att;
            }
          }

          for ( auto att : geomR->geometry()->attributes() )
          {
            qDebug() << mNode->tileId().text() << "           a" << att->name() << "\tvs:" << att->vertexSize() << "\tc:" << att->count()  << "\tst:" << att->byteStride();

            // === POSITION
            if ( att->name().compare( Qt3DRender::QAttribute::defaultPositionAttributeName() ) == 0 )
            {
              float *ptrFloat = reinterpret_cast<float *>( att->buffer()->data().data() + att->byteOffset() );
              for ( int i = 0; i < 10 && i < ( int ) att->count(); i++ )
              {
                qDebug() << mNode->tileId().text() << "              pos (" << *( ptrFloat + 0 ) << "" << *( ptrFloat + 1 ) << "" << *( ptrFloat + 2 ) << ")";
                ptrFloat += 3;
              }

              if ( !hasNormal && facesByVertexIdAtt != NULL )
              {
                QByteArray normBuf( sizeof( Utils3d::VertexData ) * att->count(), 0 );
                //qDebug() <<  "           a ++++ normal. norm data:" << ( unsigned long ) normBuf->data() ;

                qDebug() << mNode->tileId().text() << "           a ++++ ADDING NORMALS!";
                qDebug() << mNode->tileId().text() << "           a ++++ face count:" << ( unsigned long ) facesByVertexIdAtt->count() ;
                Qgs3DUtils::createNormal( reinterpret_cast<Utils3d::VertexData *>( att->buffer()->data().data() + att->byteOffset() ),
                                          att->count(),
                                          reinterpret_cast<Utils3d::FaceByVertexId *>( facesByVertexIdAtt->buffer()->data().data() + facesByVertexIdAtt->byteOffset() ),
                                          facesByVertexIdAtt->count() / 3,
                                          reinterpret_cast<Utils3d::VertexData *>( normBuf.data() ) );

                Qt3DRender::QBuffer *b = new Qt3DRender::QBuffer();
                b->setData( normBuf );

                Qt3DRender::QAttribute *normAtt = new Qt3DRender::QAttribute( b, Qt3DRender::QAttribute::defaultNormalAttributeName(), att->vertexBaseType(),
                    att->vertexSize(), att->count(),
                    0, att->byteStride() );

                geomR->geometry()->addAttribute( normAtt );
              }
            }

            // === NORMAL
            else if ( att->name().compare( Qt3DRender::QAttribute::defaultNormalAttributeName() ) == 0 )
            {
              // debug done later
            }

            // === INDEX vertex/face index
            else if ( att->name().compare( "" ) == 0 )
            {
              quint16 *ptrUint16 = reinterpret_cast<quint16 *>( att->buffer()->data().data() + att->byteOffset() );
              for ( int i = 0; i < 10 && i < ( int ) att->count(); i++ )
              {
                qDebug() << mNode->tileId().text() << "              idx (" << *( ptrUint16 + 0 ) << ")";
                ptrUint16 += 1;
              }
            }
          }

          // CHECK AGAIN FOR NORMALS
          hasNormal = false;
          for ( auto att : geomR->geometry()->attributes() )
          {
            if ( att->name().compare( Qt3DRender::QAttribute::defaultNormalAttributeName() ) == 0 )
            {
              hasNormal = true;
              const float *ptrFloat = reinterpret_cast<const float *>( att->buffer()->data().constData() + att->byteOffset() );
              for ( int i = 0; i < 10 && i < ( int ) att->count(); i++ )
              {
                qDebug() << mNode->tileId().text() << "              norm (" << *( ptrFloat + 0 ) << "" << *( ptrFloat + 1 ) << "" << *( ptrFloat + 2 ) << ")";
                ptrFloat += 3;
              }

            }
          }
          if ( !hasNormal )
          {
            qDebug() << mNode->tileId().text() << "  ===============================================================  NO normal!" ;
          }

        }
      }

      if ( hasGeom )
      {
        aTransform = new Qt3DCore::QTransform();
        QgsMatrix4x4 fixRot;
        fixRot.setToIdentity();
        /*/tmp/3dtiles/dragon/dragon_high.b3dm/dragon_high.b3dm.json
        QgsCoordinateTransform coordTrans( mFactory->mCoordinateTransform.destinationCrs(),
                                           QgsCoordinateReferenceSystem::fromEpsgId( 4326 ),
                                           mFactory->mCoordinateTransform.context() );
        QgsVector3D transCenter = coordTrans.transform( flipCenter );
        qDebug() <<  "        * ++++ transCenter" << mTile->getBoundingVolumeAsAABB().center()
                 << ">>" << flipCenter
                 << ">>" << transCenter;
        fixRot.rotate( - transCenter.y(), QgsVector3D( 1.0, 0.0, 0.0 ) );
        fixRot.rotate( -( 90.0 + transCenter.x() ), QgsVector3D( 0.0, 0.0, 1.0 ) );
        */
        QVector3D center4978;
        QVector3D yUpCenter4978;
        if ( mFactory->mCoordinateTransform.destinationCrs() != QgsCoordinateReferenceSystem::fromEpsgId( 4978 ) )
        {
          /*if ( mTile->getContentConst()->mType == ThreeDTilesContent::b3dm && ( ( B3dmHolder * )( mTile->getContentConst() ) )->mModel.mFeatureTable.mRtcCenter.length() != 0.0 )
          {
            qDebug() << mNode->tileId().text() << "        * USING RTC center:" << ( ( B3dmHolder * )( mTile->getContentConst() ) )->mModel.mFeatureTable.mRtcCenter;
            QgsVector3D tempPt = (* mTile->getCombinedTransform()) * ( ( B3dmHolder * )( mTile->getContentConst() ) )->mModel.mFeatureTable.mRtcCenter;
            center4978 = QVector3D(tempPt.x(), tempPt.y(), tempPt.z());
          }
          else
          {*/
          qDebug() << mNode->tileId().text() << "        * USING Tile bbox center!";
          QgsAABB bb4978 = mTile->getBoundingVolumeAsAABB();
          center4978 = QVector3D( bb4978.xCenter(), bb4978.zCenter(), -bb4978.yCenter() );
          //}
          yUpCenter4978 = QVector3D( center4978.x(), -center4978.z(), center4978.y() );

          qDebug() << mNode->tileId().text() << "        * ++++ center4978" <<  center4978;
          qDebug() << mNode->tileId().text() << "        * ++++ yUpCenter4978" <<  yUpCenter4978;

          double psi, theta, phi;
          psi = 180.0 * atan( yUpCenter4978.y() / yUpCenter4978.z() ) / M_PI;
          theta = 180.0 * atan( yUpCenter4978.x() / yUpCenter4978.z() ) / M_PI;
          phi = 180.0 * atan( yUpCenter4978.y() / yUpCenter4978.x() ) / M_PI;
          if ( mTile->getCombinedTransform()->isIdentity() )
          {
            qDebug() << mNode->tileId().text() << "        * ++++ FOR Lille!";
            fixRot.rotate( -( 90.0 + phi ), QgsVector3D( 0.0, 0.0, 1.0 ) );
            fixRot.rotate( theta, QgsVector3D( 0.0, 1.0, 0.0 ) );
            fixRot.rotate( -( 90.0 + psi ), QgsVector3D( 1.0, 0.0, 0.0 ) );
          }
          else
          {
            qDebug() << mNode->tileId().text() << "        * ++++ FOR DRAGON!";
            fixRot.rotate( -( 90.0 + phi )*M_PI_2, QgsVector3D( 0.0, 0.0, 1.0 ) );
            fixRot.rotate( -theta, QgsVector3D( 0.0, 1.0, 0.0 ) );
            fixRot.rotate( psi, QgsVector3D( 1.0, 0.0, 0.0 ) );
          }

          qDebug() << mNode->tileId().text() << "        * ++++ transform psi:" << psi << "phi:" << phi << "theta:" << theta;
          qDebug() << mNode->tileId().text() << "        * ++++ transform fixRot:" << fixRot;
          //aTransform->setScale( 100.0f );
        }

        QMatrix4x4 transformMatrix( ( *mTile->getCombinedTransform() * fixRot ).toQMatrix4x4( & mFactory->mCoordinateTransform ) );
        aTransform->setMatrix( transformMatrix );

        if ( mTile->mParentTileset->getCorrectTranslation() )
        {
          // mFactory->mCoordinateTransform.transform( yUpCenter4978 );
          QgsVector3D c = mFactory->mCoordinateTransform.transform( ( *mTile->getCombinedTransform() ) * center4978 );
          QVector3D zUpCenter( c.x(), -c.z(), c.y() );
          /*          if (mTile->mParentTileset->getFlipY() )
                      aTransform->setTranslation( QVector3D( c.x(), c.z(), -c.y() ) );
                    else*/
          qDebug() << mNode->tileId().text() << "        * ++++ will use translation" << zUpCenter << "instead of" << QVector3D( mBbox.xCenter(), mBbox.yCenter(), mBbox.zCenter() );
          aTransform->setTranslation( zUpCenter );
          /*
          QgsAABB bbDest = mTile->getBoundingVolumeAsAABB( &mFactory->mCoordinateTransform);
          aTransform->setTranslation( QVector3D( bbDest.xCenter(), bbDest.yCenter(), bbDest.zCenter() ) );*/
        }

        ent->addComponent( aTransform );
        qDebug() << mNode->tileId().text() << "        * ++++ transform" << aTransform->matrix();

        if ( ! hasMaterial )
        {
          aMaterial = new Qt3DExtras::QPhongMaterial();
          //aMaterial->setDiffuse( QColor( ( int )( 25.6 * (mTile->mDepth % 10) ) , ( int )( 25.6 * (mTile->mDepth % 100) / 10 ), ( int )( 25.6 * mTile->mDepth /100 ) ) );
          aMaterial->setDiffuse( QColor( ( int )( 50.0 + 25.6 * ( 1 + mTile->mDepth / 100 ) ),
                                         ( int )( 50.0 + 25.6 * ( 1 + ( mTile->mDepth % 100 ) / 10 ) ),
                                         ( int )( 50.0 + 25.6 * ( mTile->mDepth % 10 ) ) ) );
          //aMaterial->set
          ent->addComponent( aMaterial );
          qDebug() <<  "        * ++++ material";
        }
      }
    }
  }
  qDebug() << mNode->tileId().text() << "ChunkLoader::sceneLoaderChanged: <<<<<<";
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


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
  qDebug() << "================================ Qgs3dTilesChunkLoaderFactory::~Qgs3dTilesChunkLoaderFactory";
  // noop
}

QgsChunkLoader *Qgs3dTilesChunkLoaderFactory::createChunkLoader( QgsChunkNode *node ) const
{
  return new Qgs3dTilesChunkLoader( ( Qgs3dTilesChunkLoaderFactory * )this, node );
}

int Qgs3dTilesChunkLoaderFactory::primitivesCount( QgsChunkNode *node ) const
{
  Tile *tile = mRootTile->mParentTileset->findTile( node->tileId() );
  if ( tile == nullptr ) // when called for root node,
  {
    return 1;
  }
  else
  {
    return tile->children().length();
  }
}


QgsChunkNode *Qgs3dTilesChunkLoaderFactory::createRootNode() const
{
  qDebug() << "Qgs3dTilesChunkLoaderFactory: create ROOT NODE\n";
  QgsAABB bb = mRootTile->getBoundingVolumeAsAABB();
  QgsChunkNodeId tileId = mRootTile->mParentTileset->encodeTileId( -1, bb );
  qDebug() << "Qgs3dTilesChunkLoaderFactory::createRootNode will use geometricError: " << 50000000;
  //qDebug() << "Qgs3dTilesChunkLoaderFactory::createRootNode bb: " << bb.minimum() << "/" << bb.maximum();
  return new QgsChunkNode( tileId, mRootTile->getBoundingVolumeAsAABB( &mCoordinateTransform ), 50000000 );
}

void Qgs3dTilesChunkLoaderFactory::createChildrenRec( QVector<QgsChunkNode *> &out, Tile *tile, QgsChunkNode *node, int level ) const
{
  for ( std::shared_ptr<Tile> childTile : tile->children() )
  {
    QgsAABB bb = childTile->getBoundingVolumeAsAABB();
    //qDebug() << "Qgs3dTilesChunkLoaderFactory::createChildrenRec bb: " << bb.minimum() << "/" << bb.maximum();
    QgsChunkNodeId childId = mRootTile->mParentTileset->encodeTileId( level, bb );
    QgsChunkNode *childNode = new QgsChunkNode( childId, childTile->getBoundingVolumeAsAABB( &mCoordinateTransform ), tile->getGeometricError(), node );
    qDebug() << "Qgs3dTilesChunkLoaderFactory::createChildrenRec " << childId.text() << "will use geometricError: " << tile->getGeometricError();
    out << childNode;
    //createChildrenRec (out, childTile.get(), childNode, level + 1);
  }
}


QVector<QgsChunkNode *> Qgs3dTilesChunkLoaderFactory::createChildren( QgsChunkNode *node ) const
{
  QVector<QgsChunkNode *> children;
  QgsChunkNodeId tileId = node->tileId();

  qDebug() << "Qgs3dTilesChunkLoaderFactory::createChildren from tileId" << tileId.text();
  Tile *tile = mRootTile->mParentTileset->findTile( tileId );

  if ( tile == nullptr ) // when called for root node
  {
    QgsAABB bb = mRootTile->getBoundingVolumeAsAABB();
    QgsChunkNodeId tileId = mRootTile->mParentTileset->encodeTileId( mRootTile->mDepth, bb );
    children << new QgsChunkNode( tileId, mRootTile->getBoundingVolumeAsAABB( &mCoordinateTransform ), mRootTile->mParentTileset->getGeometricError() );
    qDebug() << "Qgs3dTilesChunkLoaderFactory::createChildren ROOT child node: " << tileId.text() << "will use geometricError: " << mRootTile->mParentTileset->getGeometricError();

  }
  else
  {
    if ( tile->children().isEmpty() )
    {
      if ( tile->getContent()->mType == ThreeDTilesContent::tileset )
      {
        Tile *subTilesetContent = ( ( Tileset * )tile->getContent() )->mRootTile.get();
        createChildrenRec( children, subTilesetContent, node, tileId.d + 1 );
      }

    }
    else // only children without direct sub tileset
    {
      QgsAABB parentBb = tile->getBoundingVolumeAsAABB();
      for ( auto c : tile->children() )
      {
        QgsAABB cBb = c->getBoundingVolumeAsAABB();
        if ( !( parentBb.minimum().x() <= cBb.minimum().x() &&
                parentBb.minimum().y() <= cBb.minimum().y() &&
                parentBb.minimum().z() <= cBb.minimum().z() &&
                parentBb.maximum().x() >= cBb.maximum().x() &&
                parentBb.maximum().y() >= cBb.maximum().y() &&
                parentBb.maximum().z() >= cBb.maximum().z() ) )
          qDebug() << "=============================== BAD BB: child outside parent bbox! ";
      }
      createChildrenRec( children, tile, node, tileId.d + 1 );
    }
  }

  return children;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


Qgs3dTilesChunkedEntity::Qgs3dTilesChunkedEntity( Qgs3dTilesChunkLoaderFactory *factory, Tile *tile, bool showBoundingBoxes )
  : QgsChunkedEntity( tile->getGeometricError(), factory, false )
{
  qDebug() << "================================ Qgs3dTilesChunkedEntity::Qgs3dTilesChunkedEntity construction !!";
  setObjectName( "==>3dTilesChunkedEntity<==" );
  setUsingAdditiveStrategy( tile->mRefine !=  Refinement::ADD );
  setShowBoundingBoxes( showBoundingBoxes );
}

Qgs3dTilesChunkedEntity::~Qgs3dTilesChunkedEntity()
{
  qDebug() << "================================ Qgs3dTilesChunkedEntity::~Qgs3dTilesChunkedEntity destruction";
  // cancel / wait for jobs
  cancelActiveJobs();
}

/// @endcond
