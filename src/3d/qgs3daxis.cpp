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

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QText2DEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QViewport>

#include "qgs3daxis.h"

Qgs3DAxis::Qgs3DAxis( Qt3DExtras::Qt3DWindow *mainWindow, Qt3DRender::QCamera *mainCamera, Qt3DCore::QEntity *root )
{
  mMainCamera = mainCamera;
  mMainWindow = mainWindow;

  mSceneEntity = new Qt3DCore::QEntity( root );

  mCamera = new Qt3DRender::QCamera();
  mCamera->setParent( mSceneEntity );
  mCamera->setProjectionType( Qt3DRender::QCameraLens::ProjectionType::OrthographicProjection );
  mCamera->lens()->setOrthographicProjection(
    -5.0f, 5.0f,
    -5.0f, 5.0f,
    0.1f, 25.0f );
  mCamera->setUpVector( mMainCamera->upVector() );
  mCamera->setViewCenter( QVector3D( 0.0f, 0.0f, 0.0f ) );

  mMainCamera->connect( mMainCamera, &Qt3DRender::QCamera::viewVectorChanged, this, &Qgs3DAxis::updateCamera );
  mMainCamera->connect( mMainWindow, &Qt3DExtras::Qt3DWindow::widthChanged, this, &Qgs3DAxis::updateViewportSize );
  mMainCamera->connect( mMainWindow, &Qt3DExtras::Qt3DWindow::heightChanged, this, &Qgs3DAxis::updateViewportSize );

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
  createAxis( Axis::X );
  createAxis( Axis::Y );
  createAxis( Axis::Z );
}

void Qgs3DAxis::createAxis( const Qgs3DAxis::Axis &axis )
{
  float cylinderRadius = 0.05 * mCylinderLength;
  float coneLength = 0.3 * mCylinderLength;
  float coneBottomRadius = 0.1 * mCylinderLength;

  QQuaternion rotation;
  QColor color;
  QString axisName;
  QVector3D textTranslation;

  switch ( axis )
  {
    case Axis::X:
      rotation = QQuaternion::fromAxisAndAngle( QVector3D( 0.0f, 0.0f, 1.0f ), -90.0f );
      color = Qt::red;
      axisName = QString( "X" );
      textTranslation = QVector3D( mCylinderLength, -mCylinderLength / 4.0f, 0.0f );
      break;
    case Axis::Y:
      rotation = QQuaternion::fromAxisAndAngle( QVector3D( 0.0f, 0.0f, 0.0f ), 0.0f );
      color = Qt::green;
      axisName = QString( "Y" );
      textTranslation = QVector3D( coneBottomRadius, ( mCylinderLength + coneLength ) / 2.0, 0.0f );
      break;
    case Axis::Z:
      rotation = QQuaternion::fromAxisAndAngle( QVector3D( 1.0f, 0.0f, 0.0f ), 90.0f );
      color = Qt::blue;
      axisName = QString( "Z" );
      textTranslation = QVector3D( 0.0f, -mCylinderLength / 4.0f, mCylinderLength + coneLength / 2.0 );
      break;
    default:
      return;
  }

  // cylinder
  Qt3DCore::QEntity *cylinder = new Qt3DCore::QEntity( mSceneEntity );
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
  Qt3DCore::QEntity *coneEntity = new Qt3DCore::QEntity( mSceneEntity );
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
  Qt3DExtras::QText2DEntity *textWhite = new Qt3DExtras::QText2DEntity( mSceneEntity );
  textWhite->setFont( QFont( "monospace", 8 ) );
  textWhite->setHeight( 25 );
  textWhite->setWidth( 125 );
  textWhite->setText( axisName );
  textWhite->setColor( Qt::white );

  Qt3DCore::QTransform *textWhiteTransform = new Qt3DCore::QTransform;
  textWhiteTransform->setScale( 0.1f );
  textWhiteTransform->setTranslation( textTranslation );
  textWhite->addComponent( textWhiteTransform );

  Qt3DExtras::QText2DEntity *textBlack = new Qt3DExtras::QText2DEntity( mSceneEntity );
  textBlack->setFont( QFont( "monospace", 8 ) );
  textBlack->setHeight( 20 );
  textBlack->setWidth( 100 );
  textBlack->setText( axisName );
  textBlack->setColor( Qt::black );

  Qt3DCore::QTransform *textTransform = new Qt3DCore::QTransform;
  textTransform->setScale( 0.1f );
  textTransform->setTranslation( textTranslation );
  textBlack->addComponent( textTransform );

}


void Qgs3DAxis::updateViewportSize( int )
{
  float widthRatio = 120.0f / mMainWindow->width();
  float heightRatio = 120.0f / mMainWindow->height();

  qDebug() << "Axis, update viewport" << widthRatio << heightRatio;
  mViewport->setNormalizedRect( QRectF( 1.0 - widthRatio, 1.0 - heightRatio, widthRatio, heightRatio ) );
}

void Qgs3DAxis::updateCamera( const QVector3D &viewVector )
{
  QVector3D mainCameraShift =  - viewVector.normalized() * 2.0 * mCylinderLength; // 2.0 * mCylinderLength ==> to be far enough to have no clipping
  qDebug() << "update camera, main Camera shift" << mainCameraShift;
  mCamera->setPosition( mainCameraShift );
}
