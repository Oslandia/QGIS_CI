/***************************************************************************
  qgsforwardrenderview.cpp
  --------------------------------------
  Date                 : August 2022
  Copyright            : (C) 2022 by Benoit De Mezzo and (C) 2020 by Belgacem Nedjima
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsforwardrenderview.h"
#include "qgsdirectionallightsettings.h"
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QViewport>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QRenderTargetSelector>
#include <Qt3DRender/QRenderTarget>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QFrustumCulling>
#include <Qt3DRender/QRenderStateSet>
#include <Qt3DRender/QDepthTest>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QPolygonOffset>
#include <Qt3DRender/QRenderCapture>
#include <Qt3DRender/QSubtreeEnabler>
#include <Qt3DRender/QBlendEquation>
#include <Qt3DRender/QSortPolicy>
#include <Qt3DRender/QNoDepthMask>
#include <Qt3DRender/QBlendEquationArguments>

QgsForwardRenderView::QgsForwardRenderView( QObject *parent, Qt3DRender::QCamera *mainCamera )
  : QgsAbstractRenderView( parent )
  , mMainCamera( mainCamera )
{
  mLayer = new Qt3DRender::QLayer;
  mLayer->setRecursive( true );

  mTransparentObjectsLayer = new Qt3DRender::QLayer;
  mTransparentObjectsLayer->setRecursive( true );

  mRendererEnabler = new Qt3DRender::QSubtreeEnabler;
  mRendererEnabler->setEnablement( Qt3DRender::QSubtreeEnabler::Persistent );

  // forward rendering pass
  constructRenderPass();
}

void QgsForwardRenderView::onTargetOutputUpdate()
{
  if ( mRenderTargetSelector )
  {
    if ( ! mTargetOutputs.isEmpty() )
    {
      Qt3DRender::QRenderTarget *renderTarget = new Qt3DRender::QRenderTarget;

      for ( Qt3DRender::QRenderTargetOutput *targetOutput : qAsConst( mTargetOutputs ) )
        renderTarget->addOutput( targetOutput );

      mRenderTargetSelector->setTarget( renderTarget );
    }
    else
    {
      mRenderTargetSelector->setTarget( nullptr );
    }
  }
}

Qt3DRender::QLayer *QgsForwardRenderView::layerToFilter()
{
  return mLayer;
}

Qt3DRender::QViewport *QgsForwardRenderView::viewport()
{
  return nullptr;
}

Qt3DRender::QFrameGraphNode *QgsForwardRenderView::topGraphNode()
{
  return mRendererEnabler;
}

void QgsForwardRenderView::enableSubTree( bool enable )
{
  if ( mRendererEnabler != nullptr )
  {
    mRendererEnabler->setEnabled( enable );
  }
}

bool QgsForwardRenderView::isSubTreeEnabled()
{
  return mRendererEnabler != nullptr && mRendererEnabler->isEnabled();
}

Qt3DRender::QFrameGraphNode *QgsForwardRenderView::constructRenderPass()
{
  // The branch structure:
  // mMainCameraSelector
  // mRenderLayerFilter
  // mRenderTargetSelector
  // opaqueObjectsFilter       | transparentObjectsLayerFilter
  // forwaredRenderStateSet    | sortPolicy
  // mFrustumCulling           | transparentObjectsRenderStateSet
  // mClearBuffers      |
  // mDebugOverlay             |
  mMainCameraSelector = new Qt3DRender::QCameraSelector( mRendererEnabler );
  mMainCameraSelector->setObjectName( "Forward render view CameraSelector" );
  mMainCameraSelector->setCamera( mMainCamera );

  mLayerFilter = new Qt3DRender::QLayerFilter( mMainCameraSelector );
  mLayerFilter->addLayer( mLayer );

  mRenderTargetSelector = new Qt3DRender::QRenderTargetSelector( mLayerFilter );
  // no target output for now, updateTargetOutput() will be called later

  Qt3DRender::QLayerFilter *opaqueObjectsFilter = new Qt3DRender::QLayerFilter( mRenderTargetSelector );
  opaqueObjectsFilter->addLayer( mTransparentObjectsLayer );
  opaqueObjectsFilter->setFilterMode( Qt3DRender::QLayerFilter::DiscardAnyMatchingLayers );

  Qt3DRender::QRenderStateSet *renderStateSet = new Qt3DRender::QRenderStateSet( opaqueObjectsFilter );

  Qt3DRender::QDepthTest *depthTest = new Qt3DRender::QDepthTest;
  depthTest->setDepthFunction( Qt3DRender::QDepthTest::Less );
  renderStateSet->addRenderState( depthTest );

  Qt3DRender::QCullFace *cullFace = new Qt3DRender::QCullFace;
  cullFace->setMode( Qt3DRender::QCullFace::CullingMode::Back );
  renderStateSet->addRenderState( cullFace );

  mFrustumCulling = new Qt3DRender::QFrustumCulling( renderStateSet );

  mClearBuffers = new Qt3DRender::QClearBuffers( mFrustumCulling );
  mClearBuffers->setClearColor( QColor::fromRgbF( 0.0, 0.0, 1.0, 1.0 ) );
  mClearBuffers->setBuffers( Qt3DRender::QClearBuffers::ColorDepthBuffer );
  mClearBuffers->setClearDepthValue( 1.0f );

  Qt3DRender::QLayerFilter *transparentObjectsLayerFilter = new Qt3DRender::QLayerFilter( mRenderTargetSelector );
  transparentObjectsLayerFilter->addLayer( mTransparentObjectsLayer );
  transparentObjectsLayerFilter->setFilterMode( Qt3DRender::QLayerFilter::AcceptAnyMatchingLayers );

  Qt3DRender::QSortPolicy *sortPolicy = new Qt3DRender::QSortPolicy( transparentObjectsLayerFilter );
  QVector<Qt3DRender::QSortPolicy::SortType> sortTypes;
  sortTypes.push_back( Qt3DRender::QSortPolicy::BackToFront );
  sortPolicy->setSortTypes( sortTypes );

  Qt3DRender::QRenderStateSet *transparentObjectsRenderStateSet = new Qt3DRender::QRenderStateSet( sortPolicy );
  {
    Qt3DRender::QDepthTest *depthTest = new Qt3DRender::QDepthTest;
    depthTest->setDepthFunction( Qt3DRender::QDepthTest::Less );
    transparentObjectsRenderStateSet->addRenderState( depthTest );

    Qt3DRender::QNoDepthMask *noDepthMask = new Qt3DRender::QNoDepthMask;
    transparentObjectsRenderStateSet->addRenderState( noDepthMask );

    Qt3DRender::QCullFace *cullFace = new Qt3DRender::QCullFace;
    cullFace->setMode( Qt3DRender::QCullFace::CullingMode::NoCulling );
    transparentObjectsRenderStateSet->addRenderState( cullFace );

    Qt3DRender::QBlendEquation *blendEquation = new Qt3DRender::QBlendEquation;
    blendEquation->setBlendFunction( Qt3DRender::QBlendEquation::Add );
    transparentObjectsRenderStateSet->addRenderState( blendEquation );

    Qt3DRender::QBlendEquationArguments *blenEquationArgs = new Qt3DRender::QBlendEquationArguments;
    blenEquationArgs->setSourceRgb( Qt3DRender::QBlendEquationArguments::Blending::One );
    blenEquationArgs->setDestinationRgb( Qt3DRender::QBlendEquationArguments::Blending::OneMinusSource1Alpha );
    blenEquationArgs->setSourceAlpha( Qt3DRender::QBlendEquationArguments::Blending::One );
    blenEquationArgs->setDestinationAlpha( Qt3DRender::QBlendEquationArguments::Blending::OneMinusSource1Alpha );
    transparentObjectsRenderStateSet->addRenderState( blenEquationArgs );
  }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  mDebugOverlay = new Qt3DRender::QDebugOverlay( mClearBuffers );
  mDebugOverlay->setEnabled( false );
#endif

  // cppcheck wrongly believes transparentObjectsRenderStateSet will leak
  // cppcheck-suppress memleak
  return mMainCameraSelector;
}

void QgsForwardRenderView::setClearColor( const QColor &clearColor )
{
  mClearBuffers->setClearColor( clearColor );
}


void QgsForwardRenderView::enableFrustumCulling( bool enabled )
{
  if ( enabled == mFrustumCullingEnabled )
    return;
  mFrustumCullingEnabled = enabled;
  if ( mFrustumCullingEnabled )
    mFrustumCulling->setParent( mClearBuffers );
  else
    mFrustumCulling->setParent( ( Qt3DCore::QNode * )nullptr );
}


void QgsForwardRenderView::enableDebugOverlay( bool enabled )
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  mDebugOverlay->setEnabled( enabled );
#else
  Q_UNUSED( enabled )
  qDebug() << "Cannot display debug overlay. Qt version 5.15 or above is needed.";
#endif
}
