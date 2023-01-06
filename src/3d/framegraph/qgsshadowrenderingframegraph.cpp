/***************************************************************************
  qgsshadowrenderingframegraph.cpp
  --------------------------------------
  Date                 : August 2020
  Copyright            : (C) 2020 by Belgacem Nedjima
  Email                : gb underscore nedjima at esi dot dz
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsshadowrenderingframegraph.h"
#include "qgsdirectionallightsettings.h"
#include "qgscameracontroller.h"
#include "qgsrectangle.h"
#include "qgspostprocessingentity.h"
#include "qgsdepthentity.h"
#include "qgs3dutils.h"
#include "qgsabstractrenderview.h"

#include "qgsambientocclusionrenderentity.h"
#include "qgsambientocclusionblurentity.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometry>

typedef Qt3DRender::QAttribute Qt3DQAttribute;
typedef Qt3DRender::QBuffer Qt3DQBuffer;
typedef Qt3DRender::QGeometry Qt3DQGeometry;
#else
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QBuffer>
#include <Qt3DCore/QGeometry>

typedef Qt3DCore::QAttribute Qt3DQAttribute;
typedef Qt3DCore::QBuffer Qt3DQBuffer;
typedef Qt3DCore::QGeometry Qt3DQGeometry;
#endif

#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QBlendEquation>
#include <Qt3DRender/QSortPolicy>
#include <Qt3DRender/QNoDepthMask>
#include <Qt3DRender/QBlendEquationArguments>
#include "qgsshadowrenderview.h"
#include "qgsforwardrenderview.h"
#include "qgsdepthrenderview.h"
#include "qgsdebugtexturerenderview.h"
#include "qgsdebugtextureentity.h"

const QString QgsShadowRenderingFrameGraph::FORWARD_RENDERVIEW = "forward";
const QString QgsShadowRenderingFrameGraph::SHADOW_RENDERVIEW = "shadow";
const QString QgsShadowRenderingFrameGraph::AXIS3D_RENDERVIEW = "3daxis";
const QString QgsShadowRenderingFrameGraph::DEPTH_RENDERVIEW = "depth";
const QString QgsShadowRenderingFrameGraph::DEBUG_RENDERVIEW = "debug";

void QgsShadowRenderingFrameGraph::constructDebugTexturePass()
{
  QgsDebugTextureRenderView *drv = new QgsDebugTextureRenderView( this, mMainCamera );
  registerRenderView( drv, DEBUG_RENDERVIEW );
}

void QgsShadowRenderingFrameGraph::constructForwardRenderPass()
{
  Qt3DRender::QTexture2D *forwardColorTexture = new Qt3DRender::QTexture2D;
  forwardColorTexture->setWidth( mSize.width() );
  forwardColorTexture->setHeight( mSize.height() );
  forwardColorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  forwardColorTexture->setGenerateMipMaps( false );
  forwardColorTexture->setMagnificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardColorTexture->setMinificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardColorTexture->wrapMode()->setX( Qt3DRender::QTextureWrapMode::ClampToEdge );
  forwardColorTexture->wrapMode()->setY( Qt3DRender::QTextureWrapMode::ClampToEdge );

  Qt3DRender::QTexture2D *forwardDepthTexture = new Qt3DRender::QTexture2D;
  forwardDepthTexture->setWidth( mSize.width() );
  forwardDepthTexture->setHeight( mSize.height() );
  forwardDepthTexture->setFormat( Qt3DRender::QTexture2D::TextureFormat::DepthFormat );
  forwardDepthTexture->setGenerateMipMaps( false );
  forwardDepthTexture->setMagnificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardDepthTexture->setMinificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardDepthTexture->wrapMode()->setX( Qt3DRender::QTextureWrapMode::ClampToEdge );
  forwardDepthTexture->wrapMode()->setY( Qt3DRender::QTextureWrapMode::ClampToEdge );

  Qt3DRender::QRenderTargetOutput *forwardRenderTargetDepthOutput = new Qt3DRender::QRenderTargetOutput;
  forwardRenderTargetDepthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  forwardRenderTargetDepthOutput->setTexture( forwardDepthTexture );
  Qt3DRender::QRenderTargetOutput *forwardRenderTargetColorOutput = new Qt3DRender::QRenderTargetOutput;
  forwardRenderTargetColorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );
  forwardRenderTargetColorOutput->setTexture( forwardColorTexture );

  QgsForwardRenderView *frv = new QgsForwardRenderView( this, mMainCamera );
  frv->setTargetOutputs( { forwardRenderTargetDepthOutput, forwardRenderTargetColorOutput } );
  registerRenderView( frv, FORWARD_RENDERVIEW );
}

Qt3DRender::QLayer *QgsShadowRenderingFrameGraph::transparentObjectLayer()
{
  QgsForwardRenderView *frv = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( frv )
    return frv->transparentObjectLayer();
  return nullptr;
}

void QgsShadowRenderingFrameGraph::constructShadowRenderPass()
{
  Qt3DRender::QTexture2D *shadowMapTexture = new Qt3DRender::QTexture2D;
  shadowMapTexture->setWidth( QgsShadowRenderingFrameGraph::mDefaultShadowMapResolution );
  shadowMapTexture->setHeight( QgsShadowRenderingFrameGraph::mDefaultShadowMapResolution );
  shadowMapTexture->setFormat( Qt3DRender::QTexture2D::TextureFormat::DepthFormat );
  shadowMapTexture->setGenerateMipMaps( false );
  shadowMapTexture->setMagnificationFilter( Qt3DRender::QTexture2D::Linear );
  shadowMapTexture->setMinificationFilter( Qt3DRender::QTexture2D::Linear );
  shadowMapTexture->wrapMode()->setX( Qt3DRender::QTextureWrapMode::ClampToEdge );
  shadowMapTexture->wrapMode()->setY( Qt3DRender::QTextureWrapMode::ClampToEdge );

  Qt3DRender::QRenderTargetOutput *renderTargetOutput = new Qt3DRender::QRenderTargetOutput;
  renderTargetOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  renderTargetOutput->setTexture( shadowMapTexture );

  QgsShadowRenderView *srv = new QgsShadowRenderView( this );
  srv->setTargetOutputs( { renderTargetOutput } );
  registerRenderView( srv, SHADOW_RENDERVIEW );
}

Qt3DRender::QFrameGraphNode *QgsShadowRenderingFrameGraph::constructPostprocessingPass()
{
  mPostProcessingCameraSelector = new Qt3DRender::QCameraSelector;
  // TODO ugly
  QgsShadowRenderView *srv = dynamic_cast<QgsShadowRenderView *>( renderView( SHADOW_RENDERVIEW ) );
  mPostProcessingCameraSelector->setCamera( srv->lightCamera() );

  mPostprocessPassLayerFilter = new Qt3DRender::QLayerFilter( mPostProcessingCameraSelector );

  mPostprocessClearBuffers = new Qt3DRender::QClearBuffers( mPostprocessPassLayerFilter );

  mRenderCaptureTargetSelector = new Qt3DRender::QRenderTargetSelector( mPostprocessClearBuffers );

  Qt3DRender::QRenderTarget *renderTarget = new Qt3DRender::QRenderTarget( mRenderCaptureTargetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput( renderTarget );
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mRenderCaptureColorTexture = new Qt3DRender::QTexture2D( colorOutput );
  mRenderCaptureColorTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureColorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  mRenderCaptureColorTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureColorTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( mRenderCaptureColorTexture );
  renderTarget->addOutput( colorOutput );

  Qt3DRender::QRenderTargetOutput *depthOutput = new Qt3DRender::QRenderTargetOutput( renderTarget );

  depthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  mRenderCaptureDepthTexture = new Qt3DRender::QTexture2D( depthOutput );
  mRenderCaptureDepthTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureDepthTexture->setFormat( Qt3DRender::QAbstractTexture::DepthFormat );
  mRenderCaptureDepthTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureDepthTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureDepthTexture->setComparisonFunction( Qt3DRender::QAbstractTexture::CompareLessEqual );
  mRenderCaptureDepthTexture->setComparisonMode( Qt3DRender::QAbstractTexture::CompareRefToTexture );

  depthOutput->setTexture( mRenderCaptureDepthTexture );
  renderTarget->addOutput( depthOutput );

  mRenderCaptureTargetSelector->setTarget( renderTarget );

  mRenderCapture = new Qt3DRender::QRenderCapture( mRenderCaptureTargetSelector );

  mPostprocessingEntity = new QgsPostprocessingEntity( this, mRootEntity );
  mPostprocessPassLayerFilter->addLayer( mPostprocessingEntity->layer() );

  return mPostProcessingCameraSelector;
}

Qt3DRender::QFrameGraphNode *QgsShadowRenderingFrameGraph::constructAmbientOcclusionRenderPass()
{
  Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector;
  cameraSelector->setCamera( mMainCamera );

  Qt3DRender::QRenderStateSet *renderStateSet = new Qt3DRender::QRenderStateSet( cameraSelector );

  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );;
  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  renderStateSet->addRenderState( depthRenderDepthTest );
  renderStateSet->addRenderState( depthRenderCullFace );

  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter( renderStateSet );

  Qt3DRender::QRenderTargetSelector *targetSelector = new Qt3DRender::QRenderTargetSelector( layerFilter );
  Qt3DRender::QRenderTarget *depthRenderTarget = new Qt3DRender::QRenderTarget( targetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorTargetOutput = new Qt3DRender::QRenderTargetOutput( depthRenderTarget );
  colorTargetOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mAmbientOcclusionRenderTexture = new Qt3DRender::QTexture2D( colorTargetOutput );
  mAmbientOcclusionRenderTexture->setSize( mSize.width(), mSize.height() );
  mAmbientOcclusionRenderTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
  mAmbientOcclusionRenderTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mAmbientOcclusionRenderTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorTargetOutput->setTexture( mAmbientOcclusionRenderTexture );
  depthRenderTarget->addOutput( colorTargetOutput );

  targetSelector->setTarget( depthRenderTarget );

  QgsAbstractRenderView *frv = renderView( QgsShadowRenderingFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = frv->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  mAmbientOcclusionRenderEntity = new QgsAmbientOcclusionRenderEntity( forwardDepthTexture, mMainCamera, mRootEntity );
  layerFilter->addLayer( mAmbientOcclusionRenderEntity->layer() );

  return cameraSelector;
}

Qt3DRender::QFrameGraphNode *QgsShadowRenderingFrameGraph::constructAmbientOcclusionBlurPass()
{
  Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector;
  cameraSelector->setCamera( mMainCamera );

  Qt3DRender::QRenderStateSet *renderStateSet = new Qt3DRender::QRenderStateSet( cameraSelector );

  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );;
  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  renderStateSet->addRenderState( depthRenderDepthTest );
  renderStateSet->addRenderState( depthRenderCullFace );

  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter( renderStateSet );

  Qt3DRender::QRenderTargetSelector *targetSelector = new Qt3DRender::QRenderTargetSelector( layerFilter );
  Qt3DRender::QRenderTarget *depthRenderTarget = new Qt3DRender::QRenderTarget( targetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorTargetOutput = new Qt3DRender::QRenderTargetOutput( depthRenderTarget );
  colorTargetOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mAmbientOcclusionBlurTexture = new Qt3DRender::QTexture2D( colorTargetOutput );
  mAmbientOcclusionBlurTexture->setSize( mSize.width(), mSize.height() );
  mAmbientOcclusionBlurTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
  mAmbientOcclusionBlurTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mAmbientOcclusionBlurTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorTargetOutput->setTexture( mAmbientOcclusionBlurTexture );
  depthRenderTarget->addOutput( colorTargetOutput );

  targetSelector->setTarget( depthRenderTarget );

  QgsAmbientOcclusionBlurEntity *blurEntity = new QgsAmbientOcclusionBlurEntity( mAmbientOcclusionRenderTexture, mRootEntity );
  layerFilter->addLayer( blurEntity->layer() );

  return cameraSelector;
}



void QgsShadowRenderingFrameGraph::constructDepthRenderPass()
{
  // depth buffer render to copy pass
  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput;
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  Qt3DRender::QTexture2D *colorTexture = new Qt3DRender::QTexture2D( colorOutput );
  colorTexture->setSize( mSize.width(), mSize.height() );
  colorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  colorTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  colorTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( colorTexture );

  Qt3DRender::QRenderTargetOutput *depthOutput = new Qt3DRender::QRenderTargetOutput;

  depthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  Qt3DRender::QTexture2D *depthTexture = new Qt3DRender::QTexture2D( depthOutput );
  depthTexture->setSize( mSize.width(), mSize.height() );
  depthTexture->setFormat( Qt3DRender::QAbstractTexture::DepthFormat );
  depthTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  depthTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  depthTexture->setComparisonFunction( Qt3DRender::QAbstractTexture::CompareLessEqual );
  depthTexture->setComparisonMode( Qt3DRender::QAbstractTexture::CompareRefToTexture );

  depthOutput->setTexture( depthTexture );

  QgsDepthRenderView *drv = new QgsDepthRenderView( this, mMainCamera );
  drv->setTargetOutputs( { colorOutput, depthOutput } );
  registerRenderView( drv, DEPTH_RENDERVIEW );

  // entity used to draw the depth texture and convert it to rgb image
  QgsAbstractRenderView *frv = renderView( QgsShadowRenderingFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = frv->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  Qt3DCore::QEntity *quad = new QgsDepthEntity( forwardDepthTexture, mRootEntity );
  quad->addComponent( drv->layerToFilter() );
}

Qt3DRender::QRenderCapture *QgsShadowRenderingFrameGraph::depthRenderCapture()
{
  QgsDepthRenderView *drv = dynamic_cast<QgsDepthRenderView *>( renderView( DEPTH_RENDERVIEW ) );
  if ( drv )
    return drv->renderCapture();

  return nullptr;
}

QgsShadowRenderingFrameGraph::QgsShadowRenderingFrameGraph( QSurface *surface, QSize s, Qt3DRender::QCamera *mainCamera, Qt3DCore::QEntity *root )
  : Qt3DCore::QEntity( root )
  , mSize( s )
{
  mRootEntity = root;
  mMainCamera = mainCamera;

  mRenderSurfaceSelector = new Qt3DRender::QRenderSurfaceSelector;

  QObject *surfaceObj = dynamic_cast< QObject *  >( surface );
  Q_ASSERT( surfaceObj );

  mRenderSurfaceSelector->setSurface( surfaceObj );
  mRenderSurfaceSelector->setExternalRenderTargetSize( mSize );

  mMainViewPort = new Qt3DRender::QViewport( mRenderSurfaceSelector );
  mMainViewPort->setNormalizedRect( QRectF( 0.0f, 0.0f, 1.0f, 1.0f ) );

  // Forward render
  constructForwardRenderPass();

  // depth buffer processing
  constructDepthRenderPass();

  // Ambient occlusion factor render pass
  Qt3DRender::QFrameGraphNode *ambientOcclusionFactorRender = constructAmbientOcclusionRenderPass();
  ambientOcclusionFactorRender->setParent( mMainViewPort );

  Qt3DRender::QFrameGraphNode *ambientOcclusionBlurPass = constructAmbientOcclusionBlurPass();
  ambientOcclusionBlurPass->setParent( mMainViewPort );

  // shadow rendering pass
  constructShadowRenderPass();

  // post process
  Qt3DRender::QFrameGraphNode *postprocessingPass = constructPostprocessingPass();
  postprocessingPass->setParent( mMainViewPort );

  // textures preview pass
  constructDebugTexturePass();
}

void QgsShadowRenderingFrameGraph::unregisterRenderView( const QString &name )
{
  QgsAbstractRenderView *renderView = mRenderViewMap [name];
  if ( renderView )
  {
    renderView->topGraphNode()->setParent( ( QNode * )nullptr );
    mRenderViewMap.remove( name );
  }
}

bool QgsShadowRenderingFrameGraph::registerRenderView( QgsAbstractRenderView *renderView, const QString &name )
{
  bool out;
  if ( mRenderViewMap [name] == nullptr )
  {
    mRenderViewMap [name] = renderView;
    renderView->topGraphNode()->setParent( mMainViewPort );
    out = true;
  }
  else
    out = false;

  return out;
}

void QgsShadowRenderingFrameGraph::setEnableRenderView( const QString &name, bool enable )
{
  if ( mRenderViewMap [name] )
  {
    mRenderViewMap [name]->enableSubTree( enable );
  }
}

QgsAbstractRenderView *QgsShadowRenderingFrameGraph::renderView( const QString &name )
{
  return mRenderViewMap [name];
}

Qt3DRender::QLayer *QgsShadowRenderingFrameGraph::filterLayer( const QString &name )
{
  return mRenderViewMap [name]->layerToFilter();
}

bool QgsShadowRenderingFrameGraph::isRenderViewEnabled( const QString &name )
{
  return mRenderViewMap [name] != nullptr && mRenderViewMap [name]->isSubTreeEnabled();
}

void QgsShadowRenderingFrameGraph::setClearColor( const QColor &clearColor )
{
  QgsForwardRenderView *frv = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( frv )
    frv->setClearColor( clearColor );
}

void QgsShadowRenderingFrameGraph::setAmbientOcclusionEnabled( bool enabled )
{
  mAmbientOcclusionRenderEntity->setEnabled( enabled );
  mPostprocessingEntity->setAmbientOcclusionEnabled( enabled );
}

void QgsShadowRenderingFrameGraph::setAmbientOcclusionIntensity( float intensity )
{
  mAmbientOcclusionRenderEntity->setIntensity( intensity );
}

void QgsShadowRenderingFrameGraph::setAmbientOcclusionRadius( float radius )
{
  mAmbientOcclusionRenderEntity->setRadius( radius );
}

void QgsShadowRenderingFrameGraph::setAmbientOcclusionThreshold( float threshold )
{
  mAmbientOcclusionRenderEntity->setThreshold( threshold );
}

void QgsShadowRenderingFrameGraph::setFrustumCullingEnabled( bool enabled )
{
  QgsForwardRenderView *frv = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( frv )
    frv->enableFrustumCulling( enabled );
}

void QgsShadowRenderingFrameGraph::setupEyeDomeLighting( bool enabled, double strength, int distance )
{
  mPostprocessingEntity->setEyeDomeLightingEnabled( enabled );
  mPostprocessingEntity->setEyeDomeLightingStrength( strength );
  mPostprocessingEntity->setEyeDomeLightingDistance( distance );
}

void QgsShadowRenderingFrameGraph::setSize( QSize s )
{
  mSize = s;

  for ( QgsAbstractRenderView *rv : qAsConst( mRenderViewMap ) )
  {
    rv->updateTargetOutputSize( mSize.width(), mSize.height() );
  }

  mRenderCaptureColorTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureDepthTexture->setSize( mSize.width(), mSize.height() );
  mRenderSurfaceSelector->setExternalRenderTargetSize( mSize );

  mAmbientOcclusionRenderTexture->setSize( mSize.width(), mSize.height() );
  mAmbientOcclusionBlurTexture->setSize( mSize.width(), mSize.height() );
}

void QgsShadowRenderingFrameGraph::setRenderCaptureEnabled( bool enabled )
{
  if ( enabled == mRenderCaptureEnabled )
    return;
  mRenderCaptureEnabled = enabled;
  mRenderCaptureTargetSelector->setEnabled( mRenderCaptureEnabled );
}

void QgsShadowRenderingFrameGraph::setDebugOverlayEnabled( bool enabled )
{
  QgsForwardRenderView *frv = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( frv )
    frv->enableDebugOverlay( enabled );
}
