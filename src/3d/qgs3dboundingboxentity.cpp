/***************************************************************************
  qgs3dboundingbox.cpp
  --------------------------------------
  Date                 : April 2022
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
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QText2DEntity>
#include <Qt3DRender/QCamera>

#include "qgs3dboundingboxentity.h"
#include "qgs3dboundingboxlabels.h"
#include "qgs3dboundingboxsettings.h"
#include "qgs3dmapscene.h"
#include "qgs3dmapsettings.h"
#include "qgs3dwiredmesh.h"
#include "qgsaabb.h"
#include "qgscameracontroller.h"
#include "qgs3dbillboardlabel.h"

Qgs3DBoundingBoxEntity::Qgs3DBoundingBoxEntity( Qt3DCore::QEntity *parent, Qgs3DMapSettings *mapSettings, QgsCameraController *cameraCtrl )
  : Qt3DCore::QEntity( parent )
  , mMapSettings( mapSettings )
  , mSettings( new Qgs3DBoundingBoxSettings() )
  , mCameraCtrl( cameraCtrl )
{
  mBBMesh = new Qgs3DWiredMesh( this );
  addComponent( mBBMesh );

  Qt3DExtras::QPhongMaterial *bboxMaterial = new Qt3DExtras::QPhongMaterial;
  bboxMaterial->setAmbient( mColor );
  addComponent( bboxMaterial );

  QgsRectangle extent = qobject_cast<Qgs3DMapScene *>( parent )->sceneExtent();
  int fontSize = static_cast< int >( std::ceil( 0.01 * std::min( extent.width(), extent.height() ) ) );
  qDebug() << "la extent" << extent.width() << "x" << extent.height() << ", donne font size" << fontSize;

  mLabelsFont = QFont( "monospace", fontSize );
  mLabelsFont.setWeight( QFont::Weight::Black );
  mLabelsFont.setStyleStrategy( QFont::StyleStrategy::ForceOutline );

  // connect( mCameraCtrl, &QgsCameraController::cameraChanged, this, &Qgs3DBoundingBoxEntity::onCameraChanged );
}

Qgs3DBoundingBoxEntity::~Qgs3DBoundingBoxEntity()
{
  delete mSettings;
}

void Qgs3DBoundingBoxEntity::createLabels( Qt::Axis axis, const QgsVector3D &bboxMin, const QgsVector3D &bboxMax, float maxExtent, QList<QVector3D> &vertices )
{
  int nrIncrD;
  float rangeIncr;
  float labelValue;
  float rangeMin;
  float rangeMax;
  float coordTmp;
  QVector3D delta1;
  QVector3D delta2;
  QVector3D incr;
  QVector3D pt1;
  QVector3D pt2;
  QVector3D pt3;
  QVector3D pt4;

  float fontSize = mLabelsFont.pointSizeF();
  QgsAABB bbox = mSettings->coords();
  int nrTicks = mSettings->nrTicks();

  switch ( axis )
  {
    case Qt::Axis::XAxis:
      nrIncrD = std::max( ( int ) std::lround( nrTicks * bbox.xExtent() / maxExtent ), 3 );
      QgsBoundingBoxLabels::extendedWilkinsonAlgorithm( bboxMin.x(), bboxMax.x(), nrIncrD, false, rangeMin, rangeMax, rangeIncr );

      coordTmp = rangeMin - mMapSettings->origin().x();
      incr = QVector3D( rangeIncr, 0.0f, 0.0f );
      pt1 = QVector3D( coordTmp, bbox.yMin, bbox.zMin );
      pt2 = QVector3D( coordTmp, bbox.yMax, bbox.zMin );
      pt3 = QVector3D( coordTmp, bbox.yMin, bbox.zMax );
      pt4 = QVector3D( coordTmp, bbox.yMax, bbox.zMax );

      delta1 = QVector3D( 0.0f, 0.0f, fontSize );
      delta2 = QVector3D( 0.0f, fontSize, 0.0f );
      break;

    case Qt::Axis::YAxis:
      nrIncrD = std::max( ( int ) std::lround( nrTicks * bbox.yExtent() / maxExtent ), 3 );
      // QgsBoundingBoxLabels::extendedWilkinsonAlgorithm( bboxMin.y(), bboxMax.y(), nrIncrD, false, rangeMin, rangeMax, rangeIncr );
      QgsBoundingBoxLabels::extendedWilkinsonAlgorithm( bboxMin.z(), bboxMax.z(), nrIncrD, false, rangeMin, rangeMax, rangeIncr );

      coordTmp = rangeMin - mMapSettings->origin().z();
      incr = QVector3D( 0.0f, rangeIncr, 0.0f );
      pt1 = QVector3D( bbox.xMin, coordTmp, bbox.zMin );
      pt2 = QVector3D( bbox.xMin, coordTmp, bbox.zMax );
      pt3 = QVector3D( bbox.xMax, coordTmp, bbox.zMin );
      pt4 = QVector3D( bbox.xMax, coordTmp, bbox.zMax );

      delta1 = QVector3D( fontSize, 0.0f, 0.0f );
      delta2 = QVector3D( 0.0f, 0.0f, fontSize );
      break;

    case Qt::Axis::ZAxis:
      nrIncrD = std::max( ( int ) std::lround( nrTicks * bbox.zExtent() / maxExtent ), 3 );
      // QgsBoundingBoxLabels::extendedWilkinsonAlgorithm( bboxMin.z(), bboxMax.z(), nrIncrD, false, rangeMin, rangeMax, rangeIncr );
      QgsBoundingBoxLabels::extendedWilkinsonAlgorithm( bboxMin.y(), bboxMax.y(), nrIncrD, false, rangeMin, rangeMax, rangeIncr );

      coordTmp = - ( rangeMin - mMapSettings->origin().y() );
      incr = QVector3D( 0.0f, 0.0f, -rangeIncr );
      pt1 = QVector3D( bbox.xMin, bbox.yMin, coordTmp );
      pt2 = QVector3D( bbox.xMin, bbox.yMax, coordTmp );
      pt3 = QVector3D( bbox.xMax, bbox.yMin, coordTmp );
      pt4 = QVector3D( bbox.xMax, bbox.yMax, coordTmp );

      delta1 = QVector3D( fontSize, 0.0f, 0.0f );
      delta2 = QVector3D( 0.0f, fontSize, 0.0f );
      break;

    default:
      return;
  }

  labelValue = rangeMin;
  nrIncrD = ( rangeMax - rangeMin ) / rangeIncr + 1;
  for ( int i = 0; i < nrIncrD; ++i )
  {
    vertices.append( pt1 );
    vertices.append( pt1 - delta1 );
    vertices.append( pt1 );
    vertices.append( pt1 - delta2 );

    vertices.append( pt2 );
    vertices.append( pt2 - delta1 );
    vertices.append( pt2 );
    vertices.append( pt2 + delta2 );

    vertices.append( pt3 );
    vertices.append( pt3 + delta1 );
    vertices.append( pt3 );
    vertices.append( pt3 - delta2 );

    vertices.append( pt4 );
    vertices.append( pt4 + delta1 );
    vertices.append( pt4 );
    vertices.append( pt4 + delta2 );

    QString text = QString::number( int( labelValue ) );

    // Qt3DExtras::QText2DEntity *textX1 = new Qt3DExtras::QText2DEntity;
    // textX1->setParent( this );
    Qgs3DBillboardLabel *textX1 = new Qgs3DBillboardLabel( mCameraCtrl, this );
    textX1->setFont( mLabelsFont );
    textX1->setHeight( 1.5 * mLabelsFont.pointSize() );
    textX1->setWidth( text.length() * mLabelsFont.pointSize() );
    textX1->setText( text );
    textX1->setColor( mColor );

    // Qt3DCore::QTransform *textTransform1 = new Qt3DCore::QTransform();
    // textTransform1->setTranslation( pt1 );
    // textX1->addComponent( textTransform1 );
    textX1->setTranslation( pt1 - delta1 );
    mLabels.append( textX1 );

    // Qt3DExtras::QText2DEntity *textX2 = new Qt3DExtras::QText2DEntity;
    // textX2->setParent( this );
    Qgs3DBillboardLabel *textX2 = new Qgs3DBillboardLabel( mCameraCtrl, this );
    textX2->setFont( mLabelsFont );
    textX2->setHeight( 1.5 * mLabelsFont.pointSize() );
    textX2->setWidth( text.length() * mLabelsFont.pointSize() );
    textX2->setText( text );
    textX2->setColor( mColor );

    // Qt3DCore::QTransform *textTransform2 = new Qt3DCore::QTransform();
    // textTransform2->setTranslation( pt2 );
    // textX2->addComponent( textTransform2 );
    textX2->setTranslation( pt2 - delta1 );
    mLabels.append( textX2 );

    // Qt3DExtras::QText2DEntity *textX3 = new Qt3DExtras::QText2DEntity;
    // textX3->setParent( this );
    Qgs3DBillboardLabel *textX3 = new Qgs3DBillboardLabel( mCameraCtrl, this );
    textX3->setFont( mLabelsFont );
    textX3->setHeight( 1.5 * mLabelsFont.pointSize() );
    textX3->setWidth( text.length() * mLabelsFont.pointSize() );
    textX3->setText( text );
    textX3->setColor( mColor );

    // Qt3DCore::QTransform *textTransform3 = new Qt3DCore::QTransform();
    // textTransform3->setTranslation( pt3 );
    // textX3->addComponent( textTransform3 );
    textX3->setTranslation( pt3 + delta1 );
    mLabels.append( textX3 );

    // Qt3DExtras::QText2DEntity *textX4 = new Qt3DExtras::QText2DEntity;
    // textX4->setParent( this );
    Qgs3DBillboardLabel *textX4 = new Qgs3DBillboardLabel( mCameraCtrl, this );
    textX4->setFont( mLabelsFont );
    textX4->setHeight( 1.5 * mLabelsFont.pointSize() );
    textX4->setWidth( text.length() * mLabelsFont.pointSize() );
    textX4->setText( text );
    textX4->setColor( mColor );

    // Qt3DCore::QTransform *textTransform4 = new Qt3DCore::QTransform();
    // textTransform4->setTranslation( pt4 );
    // textX4->addComponent( textTransform4 );
    textX4->setTranslation( pt4 + delta1 );
    mLabels.append( textX4 );

    pt1 += incr;
    pt2 += incr;
    pt3 += incr;
    pt4 += incr;
    labelValue += rangeIncr;
  }
}

void Qgs3DBoundingBoxEntity::setParameters( Qgs3DBoundingBoxSettings const &settings )
{
  if ( settings != *mSettings )
  {
    delete mSettings;
    mSettings = new Qgs3DBoundingBoxSettings( settings );

    QgsAABB bbox = settings.coords();
    qDebug() << "set bounding box parameters";
    QList<QVector3D> ticksVertices;

    for ( auto *label : mLabels )
      label->setParent( ( QEntity * ) nullptr );

    mLabels.clear();

    QgsVector3D bboxWorldMin( bbox.xMin, bbox.yMin, bbox.zMin );
    QgsVector3D bboxMapMin = mMapSettings->worldToMapCoordinates( bboxWorldMin );
    QgsVector3D bboxWorldMax( bbox.xMax, bbox.yMax, bbox.zMax );
    QgsVector3D bboxMapMax = mMapSettings->worldToMapCoordinates( bboxWorldMax );
    float swapY = bboxMapMin.y();
    bboxMapMin.set( bboxMapMin.x(), bboxMapMax.y(), bboxMapMin.z() );
    bboxMapMax.set( bboxMapMax.x(), swapY, bboxMapMax.z() );
    float maxExtent = std::max( std::max( bboxMapMax.x() - bboxMapMin.x(), bboxMapMax.y() - bboxMapMin.y() ), bboxMapMax.z() - bboxMapMin.z() );

    createLabels( Qt::Axis::XAxis, bboxMapMin, bboxMapMax, maxExtent, ticksVertices );
    createLabels( Qt::Axis::YAxis, bboxMapMin, bboxMapMax, maxExtent, ticksVertices );
    createLabels( Qt::Axis::ZAxis, bboxMapMin, bboxMapMax, maxExtent, ticksVertices );

    mBBMesh->setVertices( bbox.verticesForLines() + ticksVertices );
  }
}

void Qgs3DBoundingBoxEntity::onCameraChanged()
{
  QVector3D cameraPosition = mCameraCtrl->camera()->position();

  // camera position has not been set yet
  if ( cameraPosition.x() == 0.0f && cameraPosition.z() == 0.0f )
  {
    qDebug() << "not set";
    return;
  }

  if ( mInitialCameraPosition ==  QVector3D( 0, 0, 0 ) )
    mInitialCameraPosition = mCameraCtrl->camera()->position();

  qDebug() << "on camera changed";
  qDebug() << "camera position" << cameraPosition;

  float fov_tan = 2.0f * std::tan( mCameraCtrl->camera()->fieldOfView() * M_PI / ( 180.0f * 2.0f ) );

  for ( auto *label : mLabels )
  {
    Qt3DCore::QTransform *transform = label->componentsOfType<Qt3DCore::QTransform>()[0];
    float distance = cameraPosition.distanceToPoint( transform->translation() );
    float size = fov_tan * distance;

    float distanceInit = mInitialCameraPosition.distanceToPoint( transform->translation() );
    float sizeInit = fov_tan * distanceInit;
    transform->setScale( size / sizeInit );
  }
}
