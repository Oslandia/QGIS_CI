/***************************************************************************
  qgs3daxisrenderview.cpp
  --------------------------------------
  Date                 : May 2022
  Copyright            : (C) 2022 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3daxisrenderview.h"

#include "qgsshadowrenderingframegraph.h"
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QText2DEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QViewport>
#include <Qt3DRender/QPickEvent>
#include <Qt3DRender/QScreenRayCaster>
#include <Qt3DRender/QSubtreeEnabler>

#include <QVector3D>
#include <QVector2D>

#include <Qt3DRender/QLayer>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometryRenderer>

#include "qgs3dmapsettings.h"


Qgs3DAxisRenderView::Qgs3DAxisRenderView( QObject *parent, Qt3DExtras::Qt3DWindow *parentWindow,
    Qt3DRender::QCamera *axisCamera, Qgs3DMapSettings &settings )
  : QgsAbstractRenderView( parent )
  , mParentWindow( parentWindow )
  , mAxisCamera( axisCamera )
  , mMapSettings( settings )
{
  mShadowRendererEnabler = new Qt3DRender::QSubtreeEnabler;
  mShadowRendererEnabler->setEnablement( Qt3DRender::QSubtreeEnabler::Persistent );

  mAxisViewport = new Qt3DRender::QViewport( mShadowRendererEnabler );

  mAxisSceneLayer = new Qt3DRender::QLayer;
  mAxisSceneLayer->setRecursive( true );

  auto axisLayerFilter = new Qt3DRender::QLayerFilter( mAxisViewport );
  axisLayerFilter->addLayer( mAxisSceneLayer );

  auto axisCameraSelector = new Qt3DRender::QCameraSelector;
  axisCameraSelector->setParent( axisLayerFilter );
  axisCameraSelector->setCamera( mAxisCamera );

  auto clearBuffers = new Qt3DRender::QClearBuffers( axisCameraSelector );
  clearBuffers->setBuffers( Qt3DRender::QClearBuffers::DepthBuffer );

  Qgs3DAxisSettings axisSettings  = mMapSettings.get3dAxisSettings();
  setAxisViewportPosition( mAxisViewportSize, axisSettings.verticalPosition(), axisSettings.horizontalPosition() );

  connect( mParentWindow, &Qt3DExtras::Qt3DWindow::widthChanged, this, &Qgs3DAxisRenderView::onAxisViewportSizeUpdate );
  connect( mParentWindow, &Qt3DExtras::Qt3DWindow::heightChanged, this, &Qgs3DAxisRenderView::onAxisViewportSizeUpdate );
}

//! Sets root entity of the 3D scene
void Qgs3DAxisRenderView::setRootEntity( Qt3DCore::QEntity *root )
{
}

void Qgs3DAxisRenderView::enableSubTree( bool enable )
{
  if ( mShadowRendererEnabler != nullptr )
  {
    mShadowRendererEnabler->setEnabled( enable );
  }
}

Qt3DRender::QFrameGraphNode *Qgs3DAxisRenderView::topGraphNode()
{
  return mShadowRendererEnabler;
}

Qt3DRender::QViewport *Qgs3DAxisRenderView::viewport()
{
  return mAxisViewport;
}

Qt3DRender::QLayer *Qgs3DAxisRenderView::layerToFilter()
{
  return mAxisSceneLayer;
}

void Qgs3DAxisRenderView::setAxisViewportPosition( int axisViewportSize, AxisViewportPosition axisViewportVertPos, AxisViewportPosition axisViewportHorizPos )
{
  mAxisViewportSize = axisViewportSize;
  mAxisViewportVertPos = axisViewportVertPos;
  mAxisViewportHorizPos = axisViewportHorizPos;
  onAxisViewportSizeUpdate();
  mParentWindow->requestUpdate();
}

void Qgs3DAxisRenderView::onAxisViewportSizeUpdate( int )
{
  float widthRatio = ( float )mAxisViewportSize / mParentWindow->width();
  float heightRatio = ( float )mAxisViewportSize / mParentWindow->height();

  float xRatio;
  float yRatio;
  if ( mAxisViewportHorizPos == AxisViewportPosition::Begin )
    xRatio = 0.0f;
  else if ( mAxisViewportHorizPos == AxisViewportPosition::Middle )
    xRatio = 0.5 - widthRatio / 2.0;
  else
    xRatio = 1.0 - widthRatio;

  if ( mAxisViewportVertPos == AxisViewportPosition::Begin )
    yRatio = 0.0f;
  else if ( mAxisViewportVertPos == AxisViewportPosition::Middle )
    yRatio = 0.5 - heightRatio / 2.0;
  else
    yRatio = 1.0 - heightRatio;

#ifdef DEBUG
  qDebug() << "Axis, update viewport" << xRatio << yRatio << widthRatio << heightRatio;
#endif
  mAxisViewport->setNormalizedRect( QRectF( xRatio, yRatio, widthRatio, heightRatio ) );
}



void Qgs3DAxisRenderView::onAxisHorizPositionChanged( Qgs3DAxisRenderView::AxisViewportPosition pos )
{
  setAxisViewportPosition( mAxisViewportSize, mAxisViewportVertPos, pos );
  Qgs3DAxisSettings s = mMapSettings.get3dAxisSettings();
  s.setHorizontalPosition( pos );
  mMapSettings.set3dAxisSettings( s );
}

void Qgs3DAxisRenderView::onAxisVertPositionChanged( Qgs3DAxisRenderView::AxisViewportPosition pos )
{
  setAxisViewportPosition( mAxisViewportSize, pos, mAxisViewportHorizPos );
  Qgs3DAxisSettings s = mMapSettings.get3dAxisSettings();
  s.setVerticalPosition( pos );
  mMapSettings.set3dAxisSettings( s );
}
