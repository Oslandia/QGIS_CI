/***************************************************************************
  qgsambientocclusionrenderview.cpp
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

#include "qgsambientocclusionrenderview.h"
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QRenderTargetSelector>
#include <Qt3DRender/QRenderTarget>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QSubtreeEnabler>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QRenderStateSet>
#include <Qt3DRender/QDepthTest>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QRenderCapture>


QgsAmbientOcclusionRenderView::QgsAmbientOcclusionRenderView( QObject *parent, Qt3DRender::QCamera *mainCamera )
  : QgsAbstractRenderView( parent )
  , mMainCamera( mainCamera )
{
  mLayer = new Qt3DRender::QLayer;
  mLayer->setRecursive( true );

  mRendererEnabler = new Qt3DRender::QSubtreeEnabler;
  mRendererEnabler->setEnablement( Qt3DRender::QSubtreeEnabler::Persistent );

  // ambient occlusion rendering pass
  constructRenderPass();
}

void QgsAmbientOcclusionRenderView::onTargetOutputUpdate()
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

Qt3DRender::QLayer *QgsAmbientOcclusionRenderView::layerToFilter()
{
  return mLayer;
}

Qt3DRender::QViewport *QgsAmbientOcclusionRenderView::viewport()
{
  return nullptr;
}

Qt3DRender::QFrameGraphNode *QgsAmbientOcclusionRenderView::topGraphNode()
{
  return mRendererEnabler;
}

void QgsAmbientOcclusionRenderView::enableSubTree( bool enable )
{
  if ( mRendererEnabler != nullptr )
  {
    mRendererEnabler->setEnabled( enable );
  }
}

bool QgsAmbientOcclusionRenderView::isSubTreeEnabled()
{
  return mRendererEnabler != nullptr && mRendererEnabler->isEnabled();
}

Qt3DRender::QFrameGraphNode *QgsAmbientOcclusionRenderView::constructRenderPass()
{
  Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector( mRendererEnabler );
  cameraSelector->setCamera( mMainCamera );

  Qt3DRender::QRenderStateSet *renderStateSet = new Qt3DRender::QRenderStateSet( cameraSelector );

  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );;
  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  renderStateSet->addRenderState( depthRenderDepthTest );
  renderStateSet->addRenderState( depthRenderCullFace );

  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter( renderStateSet );
  layerFilter->addLayer( mLayer );

  mRenderTargetSelector = new Qt3DRender::QRenderTargetSelector( layerFilter );

  return cameraSelector;
}
