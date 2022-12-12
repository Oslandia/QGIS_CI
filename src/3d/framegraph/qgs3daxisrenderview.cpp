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
#include <QScreen>

#include <Qt3DRender/QLayer>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometryRenderer>

#include "qgsmapsettings.h"
#include "qgs3dmapsettings.h"


Qgs3DAxisRenderView::Qgs3DAxisRenderView( QObject *parent, Qt3DExtras::Qt3DWindow *parentWindow,
    Qt3DRender::QCamera *axisCamera, Qgs3DMapSettings *settings )
  : QgsAbstractRenderView( parent )
  , mParentWindow( parentWindow )
  , mCamera( axisCamera )
  , mMapSettings( settings )
{
  mRendererEnabler = new Qt3DRender::QSubtreeEnabler;
  mRendererEnabler->setEnablement( Qt3DRender::QSubtreeEnabler::Persistent );

  mViewport = new Qt3DRender::QViewport( mRendererEnabler );
  mViewport->setObjectName( "3D axis view Viewport" );

  mLayer = new Qt3DRender::QLayer;
  mLayer->setRecursive( true );

  // render pass
  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter( mViewport );
  layerFilter->addLayer( mLayer );

  Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector;
  cameraSelector->setParent( layerFilter );
  cameraSelector->setCamera( mCamera );

  mRenderTargetSelector = new Qt3DRender::QRenderTargetSelector( cameraSelector );
  // no target output for now, updateTargetOutput() will be called later

  Qt3DRender::QClearBuffers *clearBuffers = new Qt3DRender::QClearBuffers( mRenderTargetSelector );
  clearBuffers->setBuffers( Qt3DRender::QClearBuffers::DepthBuffer );

  // update viewport size
  onViewportSizeUpdate();

  connect( mParentWindow, &Qt3DExtras::Qt3DWindow::widthChanged, this, &Qgs3DAxisRenderView::onViewportSizeUpdate );
  connect( mParentWindow, &Qt3DExtras::Qt3DWindow::heightChanged, this, &Qgs3DAxisRenderView::onViewportSizeUpdate );
}

void Qgs3DAxisRenderView::onTargetOutputUpdate()
{
  if ( ! mTargetOutputs.isEmpty() && mRenderTargetSelector )
  {
    Qt3DRender::QRenderTarget *renderTarget = new Qt3DRender::QRenderTarget;

    for ( Qt3DRender::QRenderTargetOutput *targetOutput : qAsConst( mTargetOutputs ) )
      renderTarget->addOutput( targetOutput );

    mRenderTargetSelector->setTarget( renderTarget );
  }
}

void Qgs3DAxisRenderView::enableSubTree( bool enable )
{
  if ( mRendererEnabler != nullptr )
  {
    mRendererEnabler->setEnabled( enable );
  }
}

bool Qgs3DAxisRenderView::isSubTreeEnabled()
{
  return mRendererEnabler != nullptr && mRendererEnabler->isEnabled();
}


Qt3DRender::QFrameGraphNode *Qgs3DAxisRenderView::topGraphNode()
{
  return mRendererEnabler;
}

Qt3DRender::QViewport *Qgs3DAxisRenderView::viewport()
{
  return mViewport;
}

Qt3DRender::QLayer *Qgs3DAxisRenderView::layerToFilter()
{
  return mLayer;
}

void Qgs3DAxisRenderView::onViewportSizeUpdate( int )
{
  Qgs3DAxisSettings settings = mMapSettings->get3DAxisSettings();

  double windowWidth = ( double )mParentWindow->width();
  double windowHeight = ( double )mParentWindow->height();

  if ( 2 <= QgsLogger::debugLevel() )
  {
    QgsMapSettings set;
    QgsDebugMsgLevel( QString( "onAxisViewportSizeUpdate window w/h: %1px / %2px" )
                      .arg( windowWidth ).arg( windowHeight ), 2 );
    QgsDebugMsgLevel( QString( "onAxisViewportSizeUpdate window physicalDpi %1 (%2, %3)" )
                      .arg( mParentWindow->screen()->physicalDotsPerInch() )
                      .arg( mParentWindow->screen()->physicalDotsPerInchX() )
                      .arg( mParentWindow->screen()->physicalDotsPerInchY() ), 2 );
    QgsDebugMsgLevel( QString( "onAxisViewportSizeUpdate window logicalDotsPerInch %1 (%2, %3)" )
                      .arg( mParentWindow->screen()->logicalDotsPerInch() )
                      .arg( mParentWindow->screen()->logicalDotsPerInchX() )
                      .arg( mParentWindow->screen()->logicalDotsPerInchY() ), 2 );

    QgsDebugMsgLevel( QString( "onAxisViewportSizeUpdate window pixel ratio %1" )
                      .arg( mParentWindow->screen()->devicePixelRatio() ), 2 );

    QgsDebugMsgLevel( QString( "onAxisViewportSizeUpdate set pixel ratio %1" )
                      .arg( set.devicePixelRatio() ), 2 );
    QgsDebugMsgLevel( QString( "onAxisViewportSizeUpdate set outputDpi %1" )
                      .arg( set.outputDpi() ), 2 );
    QgsDebugMsgLevel( QString( "onAxisViewportSizeUpdate set dpiTarget %1" )
                      .arg( set.dpiTarget() ), 2 );
  }

  // default viewport size in pixel according to 92 dpi
  double defaultViewportPixelSize = ( ( double )settings.defaultViewportSize() / 25.4 ) * 92.0;

  // computes the viewport size according to screen dpi but as the viewport size growths too fast
  // then we limit the growth by using a factor on the dpi difference.
  double viewportPixelSize = defaultViewportPixelSize + ( ( double )settings.defaultViewportSize() / 25.4 )
                             * ( mParentWindow->screen()->physicalDotsPerInch() - 92.0 ) * 0.7;
  QgsDebugMsgLevel( QString( "onAxisViewportSizeUpdate viewportPixelSize %1" ).arg( viewportPixelSize ), 2 );
  double widthRatio = viewportPixelSize / windowWidth;
  double heightRatio = widthRatio * windowWidth / windowHeight;

  QgsDebugMsgLevel( QString( "3DAxis viewport ratios width: %1% / height: %2%" ).arg( widthRatio ).arg( heightRatio ), 2 );

  if ( heightRatio * windowHeight < viewportPixelSize )
  {
    heightRatio = viewportPixelSize / windowHeight;
    widthRatio = heightRatio * windowHeight / windowWidth;
    QgsDebugMsgLevel( QString( "3DAxis viewport, height too small, ratios adjusted to width: %1% / height: %2%" ).arg( widthRatio ).arg( heightRatio ), 2 );
  }

  if ( heightRatio > settings.maxViewportRatio() || widthRatio > settings.maxViewportRatio() )
  {
    QgsDebugMsgLevel( "viewport takes too much place into the 3d view, disabling it", 2 );
    // take too much place into the 3d view
    mViewport->setEnabled( false );
    emit viewportScaleFactorChanged( 0.0 );
  }
  else
  {
    if ( ! mViewport->isEnabled() )
    {
      // will be used to adjust the axis label translations/sizes
      emit viewportScaleFactorChanged( viewportPixelSize / defaultViewportPixelSize );
    }
    mViewport->setEnabled( true );

    float xRatio = 1.0f;
    float yRatio = 1.0f;
    if ( settings.horizontalPosition() == Qt::AnchorPoint::AnchorLeft )
      xRatio = 0.0f;
    else if ( settings.horizontalPosition() == Qt::AnchorPoint::AnchorHorizontalCenter )
      xRatio = 0.5f - widthRatio / 2.0f;
    else
      xRatio = 1.0f - widthRatio;

    if ( settings.verticalPosition() == Qt::AnchorPoint::AnchorTop )
      yRatio = 0.0f;
    else if ( settings.verticalPosition() == Qt::AnchorPoint::AnchorVerticalCenter )
      yRatio = 0.5f - heightRatio / 2.0f;
    else
      yRatio = 1.0f - heightRatio;

    QgsDebugMsgLevel( QString( "Qgs3DAxis: update viewport: %1 x %2 x %3 x %4" ).arg( xRatio ).arg( yRatio ).arg( widthRatio ).arg( heightRatio ), 2 );
    mViewport->setNormalizedRect( QRectF( xRatio, yRatio, widthRatio, heightRatio ) );
  }

}


void Qgs3DAxisRenderView::onHorizPositionChanged( Qt::AnchorPoint pos )
{
  Qgs3DAxisSettings s = mMapSettings->get3DAxisSettings();
  s.setHorizontalPosition( pos );
  mMapSettings->set3DAxisSettings( s );
  onViewportSizeUpdate();
}

void Qgs3DAxisRenderView::onVertPositionChanged( Qt::AnchorPoint pos )
{
  Qgs3DAxisSettings s = mMapSettings->get3DAxisSettings();
  s.setVerticalPosition( pos );
  mMapSettings->set3DAxisSettings( s );
  onViewportSizeUpdate();
}
