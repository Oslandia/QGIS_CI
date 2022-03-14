/***************************************************************************
  qgs3daxis.cpp
  --------------------------------------
  Date                 : March 2022
  Copyright            : (C) 2022 by Jean Felder
  Email                : jean dot felder at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QLayerFilter>
#include<ctime>

#include "qgs3daxis.h"

Qgs3DAxis::Qgs3DAxis( Qt3DExtras::Qt3DWindow *mainWindow, Qt3DCore::QEntity *main3DScene, QgsCameraController *cameraCtrl, const Qgs3DMapSettings *map )
  : QObject( mainWindow )
{
  mMainCamera = cameraCtrl->camera();
  mMainWindow = mainWindow;
  mCrs = map->crs();

  mSceneEntity = new Qt3DCore::QEntity( main3DScene );

  mCamera = new Qt3DRender::QCamera();
  mCamera->setParent( mSceneEntity );
  mCamera->setProjectionType( Qt3DRender::QCameraLens::ProjectionType::OrthographicProjection );
  mCamera->lens()->setOrthographicProjection(
    -7.0f, 7.0f,
    -7.0f, 7.0f,
    -10.0f, 25.0f );
  mCamera->setUpVector( -mMainCamera->upVector() );
  mCamera->setViewCenter( QVector3D( 0.0f, 0.0f, 0.0f ) );
  // position will be set later

  connect( cameraCtrl, &QgsCameraController::cameraChanged, this, &Qgs3DAxis::updateCamera );
  connect( mMainWindow, &Qt3DExtras::Qt3DWindow::widthChanged, this, &Qgs3DAxis::updateViewportSize );
  connect( mMainWindow, &Qt3DExtras::Qt3DWindow::heightChanged, this, &Qgs3DAxis::updateViewportSize );

  mViewport = new Qt3DRender::QViewport( mainWindow->activeFrameGraph() );

  Qt3DRender::QLayer *layer = new Qt3DRender::QLayer;
  mSceneEntity->addComponent( layer );
  layer->setRecursive( true );
  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter( mViewport );
  layerFilter->addLayer( layer );

  Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector( layerFilter );
  cameraSelector->setCamera( mCamera );
  Qt3DRender::QClearBuffers *clearBuffers = new Qt3DRender::QClearBuffers( cameraSelector );

  clearBuffers->setBuffers( Qt3DRender::QClearBuffers::DepthBuffer );

  createScene();
}

void Qgs3DAxis::createScene()
{
  if ( mAxisRoot == nullptr )
  {
    qDebug() << "Should recreate mAxisRoot" << mMode;
    mAxisRoot = new Qt3DCore::QEntity( mSceneEntity );
    mAxisRoot->setObjectName( "3DAxisRoot" );

    createAxis( Axis::X );
    createAxis( Axis::Y );
    createAxis( Axis::Z );
  }

  if ( mMode == OFF )
  {
    mAxisRoot->setEnabled( false );
  }
  else
  {
    mAxisRoot->setEnabled( true );
    if ( mMode == SRS )
    {
      if ( mCrs.isGeographic() )
      {
        mTextWhite_X->setText( "Long" );
        mTextWhite_Y->setText( "Lat" );
      }
      else
      {
        mTextWhite_X->setText( "X" );
        mTextWhite_Y->setText( "Y" );
      }
      mTextWhite_Z->setText( "Z" );
    }
    else if ( mMode == NEU )
    {
      mTextWhite_X->setText( "East" );
      mTextWhite_Y->setText( "North" );
      mTextWhite_Z->setText( "Up" );
    }
    else
    {
      mTextWhite_X->setText( "X?" );
      mTextWhite_Y->setText( "Y?" );
      mTextWhite_Z->setText( "Z?" );
    }

  }
}

void Qgs3DAxis::createAxis( const Qgs3DAxis::Axis &axis )
{
  float cylinderRadius = 0.05 * mCylinderLength;
  float coneLength = 0.3 * mCylinderLength;
  float coneBottomRadius = 0.1 * mCylinderLength;

  QQuaternion rotation;
  QColor color;
  QVector3D textTranslation;

  Qt3DExtras::QText2DEntity *textWhite;
  Qt3DCore::QTransform *textTransform;

  switch ( axis )
  {
    case Axis::X:
      mTextWhite_X = new Qt3DExtras::QText2DEntity( );  // object initialization in two step:
      mTextWhite_X->setParent( mAxisRoot ); // see https://bugreports.qt.io/browse/QTBUG-77139
      mTextTransform_X = new Qt3DCore::QTransform();

      rotation = QQuaternion::fromAxisAndAngle( QVector3D( 0.0f, 0.0f, 1.0f ), -90.0f );
      color = Qt::red;
      textWhite = mTextWhite_X;
      textTranslation = QVector3D( mCylinderLength, -mCylinderLength / 4.0f, 0.0f );
      textTransform = mTextTransform_X;
      break;

    case Axis::Y:
      mTextWhite_Y = new Qt3DExtras::QText2DEntity( );  // object initialization in two step:
      mTextWhite_Y->setParent( mAxisRoot ); // see https://bugreports.qt.io/browse/QTBUG-77139
      mTextTransform_Y = new Qt3DCore::QTransform();

      rotation = QQuaternion::fromAxisAndAngle( QVector3D( 0.0f, 0.0f, 0.0f ), 0.0f );
      color = Qt::green;
      textWhite = mTextWhite_Y;
      textTranslation = QVector3D( coneBottomRadius, ( mCylinderLength + coneLength ) / 2.0, 0.0f );
      textTransform = mTextTransform_Y;
      break;

    case Axis::Z:
      mTextWhite_Z = new Qt3DExtras::QText2DEntity( );  // object initialization in two step:
      mTextWhite_Z->setParent( mAxisRoot ); // see https://bugreports.qt.io/browse/QTBUG-77139
      mTextTransform_Z = new Qt3DCore::QTransform();

      rotation = QQuaternion::fromAxisAndAngle( QVector3D( 1.0f, 0.0f, 0.0f ), 90.0f );
      color = Qt::blue;
      textWhite = mTextWhite_Z;
      textTranslation = QVector3D( 0.0f, -mCylinderLength / 4.0f, mCylinderLength + coneLength / 2.0 );
      textTransform = mTextTransform_Z;
      break;

    default:
      return;
  }

  // cylinder
  Qt3DCore::QEntity *cylinder = new Qt3DCore::QEntity( mAxisRoot );
  Qt3DExtras::QCylinderMesh *cylinderMesh = new Qt3DExtras::QCylinderMesh;
  cylinderMesh->setRadius( cylinderRadius );
  cylinderMesh->setLength( mCylinderLength );
  cylinderMesh->setRings( 100 );
  cylinderMesh->setSlices( 20 );
  cylinder->addComponent( cylinderMesh );

  Qt3DExtras::QPhongMaterial *cylinderMaterial = new Qt3DExtras::QPhongMaterial( cylinder );
  cylinderMaterial->setAmbient( color );
  cylinderMaterial->setShininess( 0 );
  cylinder->addComponent( cylinderMaterial );

  Qt3DCore::QTransform *cylinderTransform = new Qt3DCore::QTransform;
  QMatrix4x4 transformMatrixCylinder;
  transformMatrixCylinder.rotate( rotation );
  transformMatrixCylinder.translate( QVector3D( 0.0f, mCylinderLength / 2.0f, 0.0f ) );
  cylinderTransform->setMatrix( transformMatrixCylinder );
  cylinder->addComponent( cylinderTransform );

  // cone
  Qt3DCore::QEntity *coneEntity = new Qt3DCore::QEntity( mAxisRoot );
  Qt3DExtras::QConeMesh *coneMesh = new Qt3DExtras::QConeMesh;
  coneMesh->setLength( coneLength );
  coneMesh->setBottomRadius( coneBottomRadius );
  coneMesh->setTopRadius( 0.0f );
  coneMesh->setRings( 100 );
  coneMesh->setSlices( 20 );
  coneEntity->addComponent( coneMesh );

  Qt3DExtras::QPhongMaterial *coneMaterial = new Qt3DExtras::QPhongMaterial( coneEntity );
  coneMaterial->setAmbient( color );
  coneMaterial->setShininess( 0 );
  coneEntity->addComponent( coneMaterial );

  Qt3DCore::QTransform *coneTransform = new Qt3DCore::QTransform;
  QMatrix4x4 transformMatrixCone;
  transformMatrixCone.rotate( rotation );
  transformMatrixCone.translate( QVector3D( 0.0f, mCylinderLength, 0.0f ) );
  coneTransform->setMatrix( transformMatrixCone );
  coneEntity->addComponent( coneTransform );

  // TODO must face the camera
  // axis name
  textTransform->setScale( 0.1f );
  textTransform->setTranslation( textTranslation );

  QFont f = QFont( "monospace", 12 );
  f.setWeight( QFont::Weight::Black );
  f.setStyleStrategy( QFont::StyleStrategy::ForceOutline );
  textWhite->setFont( f );
  textWhite->setHeight( 20 );
  textWhite->setWidth( 50 );
  textWhite->setColor( QColor( 192, 192, 192, 192 ) );
  textWhite->addComponent( textTransform );
}


void Qgs3DAxis::updateViewportSize( int )
{
  float widthRatio = 150.0f / mMainWindow->width();
  float heightRatio = 150.0f / mMainWindow->height();

  qDebug() << "Axis, update viewport" << widthRatio << heightRatio;
  mViewport->setNormalizedRect( QRectF( 1.0 - widthRatio, 1.0 - heightRatio, widthRatio, heightRatio ) );
}

void Qgs3DAxis::updateCamera( /* const QVector3D & viewVector*/ )
{
  if ( mMainCamera->viewVector() != mPreviousVector
       && mMainCamera->viewVector().x() == mMainCamera->viewVector().x()
       && mMainCamera->viewVector().y() == mMainCamera->viewVector().y()
       && mMainCamera->viewVector().z() == mMainCamera->viewVector().z() )
  {
    mPreviousVector = mMainCamera->viewVector();
    QVector3D mainCameraShift = mMainCamera->viewVector().normalized();
    float zy_swap = mainCameraShift.y();
    mainCameraShift.setY( mainCameraShift.z() );
    mainCameraShift.setZ( -zy_swap );
    mainCameraShift.setX( -mainCameraShift.x() );

    QQuaternion r = QQuaternion::rotationTo( QVector3D( 0.0, 0.0, -1.0 ), -mainCameraShift ).normalized();
    mTextTransform_X->setRotation( r );
    mTextTransform_Y->setRotation( r );
    mTextTransform_Z->setRotation( r );

    mCamera->setPosition( mainCameraShift );

#ifdef DEBUG
    qDebug() << std::time( nullptr ) << this << "update camera from" << mPreviousVector << "to" << mainCameraShift << " / r:" << r;
    mTextWhite_X->dumpObjectTree();
    mTextWhite_X->setFont( QFont( "monospace", 10 ) );
    mTextWhite_X->setText( "TEST" );
#endif
  }
}

void Qgs3DAxis::updateMode( Mode axisMode )
{
  if ( mMode != axisMode )
  {
    mMode = axisMode;
    createScene();
  }
}
