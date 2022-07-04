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
#include <qvector3d.h>

#include "qgs3dboundingboxentity.h"
#include "qgs3dboundingboxlabels.h"
#include "qgs3dboundingboxsettings.h"
#include "qgs3dmapscene.h"
#include "qgs3dmapsettings.h"
#include "qgs3dwiredmesh.h"
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
  bboxMaterial->setAmbient( mSettings->color() );
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

QList<QVector3D> Qgs3DBoundingBoxEntity::computeBoundingBoxVertices()
{
  QList<QVector3D> vertices;

  QgsAABB bbox = mSettings->coords();

  if ( mSettings->isFull() )
  {
    vertices = bbox.verticesForLines();
  }
  else
  {
    for ( const auto line : bbox.lines() )
    {
      for ( const auto face : mVisibleFaces )
      {
        if ( std::find( std::begin( line.faces ), std::end( line.faces ), face ) != std::end( line.faces ) )
        {
          vertices.append( line.points[0] );
          vertices.append( line.points[1] );
          break;
        }
      }
    }
  }

  return vertices;
}

void Qgs3DBoundingBoxEntity::updateVisibleFaces()
{
  mVisibleFaces.clear();

  if ( mSettings->isFull() )
  {
    mVisibleFaces =
    {
      QgsAABB::Face::Xmin, QgsAABB::Face::Xmax,
      QgsAABB::Face::Ymin, QgsAABB::Face::Ymax,
      QgsAABB::Face::Zmin, QgsAABB::Face::Zmax,
    };
  }
  else
  {
    // ajouter un commentaire pour expliquer
    QgsAABB bbox = mSettings->coords();
    QVector3D cameraPosition = mCameraCtrl->camera()->position();
    QVector3D bbCenter = QVector3D( ( bbox.xMin + bbox.xMax ) / 2.0f, ( bbox.yMin + bbox.yMax ) / 2.0f, ( bbox.zMin + bbox.zMax ) / 2.0f );
    QVector3D direction = ( bbCenter - cameraPosition ).normalized();

    for ( const auto face : QgsAABB::normals )
    {
      if ( QgsVector3D::dotProduct( direction, face.second ) > 0 )
      {
        mVisibleFaces.append( face.first );
      }
    }

  }
}

void Qgs3DBoundingBoxEntity::computeLabelsRanges( QVector3D &rangeMin, QVector3D &rangeMax, QVector3D &rangeIncr )
{
  QgsAABB bbox = mSettings->coords();

  QgsVector3D bboxWorldMin( bbox.xMin, bbox.yMin, bbox.zMin );
  QgsVector3D bboxWorldMax( bbox.xMax, bbox.yMax, bbox.zMax );

  QgsVector3D bboxMapMin = mMapSettings->worldToMapCoordinates( bboxWorldMin );
  QgsVector3D bboxMapMax = mMapSettings->worldToMapCoordinates( bboxWorldMax );

  // the conversion invert the Y coords sign.
  // Min and Max y coords need to be swapped.
  float swapMinY = bboxMapMin.y();
  bboxMapMin.set( bboxMapMin.x(), bboxMapMax.y(), bboxMapMin.z() );
  bboxMapMax.set( bboxMapMax.x(), swapMinY, bboxMapMax.z() );

  float maxExtent = std::max( std::max( bboxMapMax.x() - bboxMapMin.x(), bboxMapMax.y() - bboxMapMin.y() ), bboxMapMax.z() - bboxMapMin.z() );
  int nrTicks = mSettings->nrTicks();

  int nrIncr = std::max( ( int ) std::lround( nrTicks * ( bboxMapMax.x() - bboxMapMin.x() ) / maxExtent ), 3 );
  QgsBoundingBoxLabels::extendedWilkinsonAlgorithm( bboxMapMin.x(), bboxMapMax.x(), nrIncr, false, rangeMin[0], rangeMax[0], rangeIncr[0] );

  nrIncr = std::max( ( int ) std::lround( nrTicks * bbox.yExtent() / maxExtent ), 3 );
  QgsBoundingBoxLabels::extendedWilkinsonAlgorithm( bboxMapMin.z(), bboxMapMax.z(), nrIncr, false, rangeMin[1], rangeMax[1], rangeIncr[1] );

  nrIncr = std::max( ( int ) std::lround( nrTicks * bbox.zExtent() / maxExtent ), 3 );
  QgsBoundingBoxLabels::extendedWilkinsonAlgorithm( bboxMapMin.y(), bboxMapMax.y(), nrIncr, false, rangeMin[2], rangeMax[2], rangeIncr[2] );
}

void Qgs3DBoundingBoxEntity::clearLabels()
{
  for ( auto *label : mLabels )
    label->setParent( ( QEntity * ) nullptr );

  mLabels.clear();
}

void Qgs3DBoundingBoxEntity::createLabelsForFullBoundingBox( Qt::Axis axis, const QVector3D &rangeMin, const QVector3D &rangeMax, const QVector3D &rangeIncr, QList<QVector3D> &vertices )
{
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

  switch ( axis )
  {
    case Qt::Axis::XAxis:
      coordTmp = rangeMin[0] - mMapSettings->origin().x();
      incr = QVector3D( rangeIncr[0], 0.0f, 0.0f );
      pt1 = QVector3D( coordTmp, bbox.yMin, bbox.zMin );
      pt2 = QVector3D( coordTmp, bbox.yMax, bbox.zMin );
      pt3 = QVector3D( coordTmp, bbox.yMin, bbox.zMax );
      pt4 = QVector3D( coordTmp, bbox.yMax, bbox.zMax );

      delta1 = QVector3D( 0.0f, 0.0f, fontSize );
      delta2 = QVector3D( 0.0f, fontSize, 0.0f );
      break;

    case Qt::Axis::YAxis:
      coordTmp = rangeMin[1] - mMapSettings->origin().z();
      incr = QVector3D( 0.0f, rangeIncr[1], 0.0f );
      pt1 = QVector3D( bbox.xMin, coordTmp, bbox.zMin );
      pt2 = QVector3D( bbox.xMin, coordTmp, bbox.zMax );
      pt3 = QVector3D( bbox.xMax, coordTmp, bbox.zMin );
      pt4 = QVector3D( bbox.xMax, coordTmp, bbox.zMax );

      delta1 = QVector3D( fontSize, 0.0f, 0.0f );
      delta2 = QVector3D( 0.0f, 0.0f, fontSize );
      break;

    case Qt::Axis::ZAxis:
      coordTmp = - ( rangeMin[2] - mMapSettings->origin().y() );
      incr = QVector3D( 0.0f, 0.0f, -rangeIncr[2] );
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

  float labelValue = rangeMin[axis];
  int nrIncrD = ( rangeMax[axis] - rangeMin[axis] ) / rangeIncr[axis] + 1;
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
    textX1->setColor( mSettings->color() );

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
    textX2->setColor( mSettings->color() );

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
    textX3->setColor( mSettings->color() );

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
    textX4->setColor( mSettings->color() );

    // Qt3DCore::QTransform *textTransform4 = new Qt3DCore::QTransform();
    // textTransform4->setTranslation( pt4 );
    // textX4->addComponent( textTransform4 );
    textX4->setTranslation( pt4 + delta1 );
    mLabels.append( textX4 );

    pt1 += incr;
    pt2 += incr;
    pt3 += incr;
    pt4 += incr;
    labelValue += rangeIncr[axis];
  }
}

void Qgs3DBoundingBoxEntity::createLabelsForPartialBoundingBox( const QVector3D &rangeMin, const QVector3D &rangeMax, const QVector3D &rangeIncr, QList<QVector3D> &vertices )
{
  QgsAABB bbox = mSettings->coords();
  int fontSize = mLabelsFont.pointSize();

  for ( const auto line : bbox.lines() )
  {
    for ( const auto face : mVisibleFaces )
    {
      if ( std::find( std::begin( line.faces ), std::end( line.faces ), face ) != std::end( line.faces ) )
      {
        QVector3D pt;
        QVector3D incr;

        switch ( line.axis )
        {
          case Qt::Axis::XAxis:
            incr = QVector3D( rangeIncr[0], 0.0f, 0.0f );
            pt = line.points[0];
            pt.setX( rangeMin[0] - mMapSettings->origin().x() );
            break;

          case Qt::Axis::YAxis:
            incr = QVector3D( 0.0f, rangeIncr[1], 0.0f );
            pt = line.points[0];
            pt.setY( rangeMin[1] - mMapSettings->origin().z() );
            break;

          case Qt::Axis::ZAxis:
            incr = QVector3D( 0.0f, 0.0f, -rangeIncr[2] );
            pt = line.points[0];
            pt.setZ( - ( rangeMin[2] - mMapSettings->origin().y() ) );
            break;

          default:
            break;
        }

        qDebug() << line.axis << " : " << ( int )line.faces[0] << "x" << ( int )line.faces[1];
        const QVector3D delta1 = fontSize * QgsAABB::normals.at( line.faces[0] );
        const QVector3D delta2 = fontSize * QgsAABB::normals.at( line.faces[1] );
        qDebug() << delta1 << "x" << delta2;

        float labelValue = rangeMin[line.axis];
        int nrIncrD = ( rangeMax[line.axis] - rangeMin[line.axis] ) / rangeIncr[line.axis] + 1;
        for ( int i = 0; i < nrIncrD; ++i )
        {
          vertices.append( pt );
          vertices.append( pt + delta1 );
          vertices.append( pt );
          vertices.append( pt + delta2 );

          QString text = QString::number( int( labelValue ) );
          Qgs3DBillboardLabel *textLabel = new Qgs3DBillboardLabel( mCameraCtrl, this );
          textLabel->setFont( mLabelsFont );
          textLabel->setHeight( 1.5 * mLabelsFont.pointSize() );
          textLabel->setWidth( text.length() * mLabelsFont.pointSize() );
          textLabel->setText( text );
          textLabel->setColor( mSettings->color() );
          textLabel->setTranslation( pt + delta1 );
          mLabels.append( textLabel );

          // qDebug() << "on ajoute le texte" << text << "en " << pt.x() << "x" << pt.y() << "x" << pt.z();

          pt += incr;
          labelValue += rangeIncr[line.axis];
        }
        break;
      }
    }
  }
}

void Qgs3DBoundingBoxEntity::setParameters( Qgs3DBoundingBoxSettings const &settings )
{
  if ( settings != *mSettings )
  {
    bool changedColor = mSettings->color() != settings.color();
    bool changedCoords = ( mSettings->coords() != settings.coords() ) || ( mBBMesh->vertexCount() == 0 ) || ( mSettings->isFull() != settings.isFull() );
    bool changedTicks =  mSettings->nrTicks() != settings.nrTicks();

    delete mSettings;
    mSettings = new Qgs3DBoundingBoxSettings( settings );

    QList<QVector3D> vertices;

    if ( changedCoords )
    {
      updateVisibleFaces();
      vertices = computeBoundingBoxVertices();

      if ( !mSettings->isFull() and !mCameraConnection )
      {
        mCameraConnection = connect( mCameraCtrl, &QgsCameraController::cameraChanged, this, &Qgs3DBoundingBoxEntity::onCameraChanged );
      }
      else if ( mSettings->isFull() and mCameraConnection )
      {
        disconnect( mCameraConnection );
      }
    }

    if ( changedCoords || changedTicks )
    {
      clearLabels();
      QVector3D rangeMin;
      QVector3D rangeMax;
      QVector3D rangeIncr;
      computeLabelsRanges( rangeMin, rangeMax, rangeIncr );

      QgsAABB bbox = mSettings->coords();
      int fontSize = static_cast< int >( std::ceil( 0.03 * std::min( bbox.xExtent(), bbox.zExtent() ) ) );
      mLabelsFont.setPointSize( fontSize );
      // qDebug() << "la extent" << extent.width() << "x" << extent.height() << ", donne font size" << fontSize;

      if ( mSettings->isFull() )
      {
        createLabelsForFullBoundingBox( Qt::Axis::XAxis, rangeMin, rangeMax, rangeIncr, vertices );
        createLabelsForFullBoundingBox( Qt::Axis::YAxis, rangeMin, rangeMax, rangeIncr, vertices );
        createLabelsForFullBoundingBox( Qt::Axis::ZAxis, rangeMin, rangeMax, rangeIncr, vertices );
      }
      else
      {
        createLabelsForPartialBoundingBox( rangeMin, rangeMax, rangeIncr, vertices );
      }

      mBBMesh->setVertices( vertices );
    }

    if ( changedColor )
    {
      Qt3DExtras::QPhongMaterial *material = this->componentsOfType<Qt3DExtras::QPhongMaterial>()[0];
      material->setAmbient( mSettings->color() );

      // If the ticks number has also changed, the labels have already been redrawn
      // with the correct color.
      if ( !changedTicks )
      {
        for ( auto *label : mLabels )
        {
          label->setColor( mSettings->color() );
        }
      }
    }
  }
}

void Qgs3DBoundingBoxEntity::onCameraChanged()
{
  QList<QgsAABB::Face> previousVisibleFaces = mVisibleFaces;
  updateVisibleFaces();

  if ( mVisibleFaces != previousVisibleFaces )
  {
    clearLabels();
    QList<QVector3D> vertices = computeBoundingBoxVertices();
    QVector3D rangeMin;
    QVector3D rangeMax;
    QVector3D rangeIncr;
    computeLabelsRanges( rangeMin, rangeMax, rangeIncr );

    createLabelsForPartialBoundingBox( rangeMin, rangeMax, rangeIncr, vertices );
    mBBMesh->setVertices( vertices );
  }

  // QVector3D cameraPosition = mCameraCtrl->camera()->position();

  // // camera position has not been set yet
  // if ( cameraPosition.x() == 0.0f && cameraPosition.z() == 0.0f )
  // {
  //   qDebug() << "not set";
  //   return;
  // }

  // if ( mInitialCameraPosition ==  QVector3D( 0, 0, 0 ) )
  //   mInitialCameraPosition = mCameraCtrl->camera()->position();

  // qDebug() << "on camera changed";
  // qDebug() << "camera position" << cameraPosition;

  // float fov_tan = 2.0f * std::tan( mCameraCtrl->camera()->fieldOfView() * M_PI / ( 180.0f * 2.0f ) );

  // for ( auto *label : mLabels )
  // {
  //   Qt3DCore::QTransform *transform = label->componentsOfType<Qt3DCore::QTransform>()[0];
  //   float distance = cameraPosition.distanceToPoint( transform->translation() );
  //   float size = fov_tan * distance;

  //   float distanceInit = mInitialCameraPosition.distanceToPoint( transform->translation() );
  //   float sizeInit = fov_tan * distanceInit;
  //   transform->setScale( size / sizeInit );
  // }
}
