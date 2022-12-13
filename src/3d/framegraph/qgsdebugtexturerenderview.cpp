/***************************************************************************
  qgsdebugtexturerenderview.cpp
  --------------------------------------
  Date                 : December 2022
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

#include "qgsdebugtexturerenderview.h"
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


QgsDebugTextureRenderView::QgsDebugTextureRenderView( QObject *parent, Qt3DRender::QCamera *mainCamera )
  : QgsAbstractRenderView( parent )
  , mMainCamera( mainCamera )
{
  mLayer = new Qt3DRender::QLayer;
  mLayer->setRecursive( true );

  mRendererEnabler = new Qt3DRender::QSubtreeEnabler;
  mRendererEnabler->setEnablement( Qt3DRender::QSubtreeEnabler::Persistent );

  // debug rendering pass
  constructRenderPass();
}

void QgsDebugTextureRenderView::onTargetOutputUpdate()
{
// nope
}

Qt3DRender::QLayer *QgsDebugTextureRenderView::layerToFilter()
{
  return mLayer;
}

Qt3DRender::QViewport *QgsDebugTextureRenderView::viewport()
{
  return nullptr;
}

Qt3DRender::QFrameGraphNode *QgsDebugTextureRenderView::topGraphNode()
{
  return mRendererEnabler;
}

void QgsDebugTextureRenderView::enableSubTree( bool enable )
{
  if ( mRendererEnabler != nullptr )
  {
    mRendererEnabler->setEnabled( enable );
  }
}

bool QgsDebugTextureRenderView::isSubTreeEnabled()
{
  return mRendererEnabler != nullptr && mRendererEnabler->isEnabled();
}

Qt3DRender::QFrameGraphNode *QgsDebugTextureRenderView::constructRenderPass()
{
  Qt3DRender::QLayerFilter *mPreviewLayerFilter = new Qt3DRender::QLayerFilter( mRendererEnabler );
  mPreviewLayerFilter->addLayer( mLayer );

  Qt3DRender::QRenderStateSet *mPreviewRenderStateSet = new Qt3DRender::QRenderStateSet( mPreviewLayerFilter );
  Qt3DRender::QDepthTest *mPreviewDepthTest = new Qt3DRender::QDepthTest;
  mPreviewDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );
  mPreviewRenderStateSet->addRenderState( mPreviewDepthTest );
  Qt3DRender::QCullFace *mPreviewCullFace = new Qt3DRender::QCullFace;
  mPreviewCullFace->setMode( Qt3DRender::QCullFace::NoCulling );
  mPreviewRenderStateSet->addRenderState( mPreviewCullFace );

  return mPreviewLayerFilter;
}
