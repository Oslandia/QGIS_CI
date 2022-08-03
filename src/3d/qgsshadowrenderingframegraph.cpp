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
#include "qgspreviewquad.h"
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

const QString QgsShadowRenderingFrameGraph::FORWARD_RENDERVIEW = "forward";
const QString QgsShadowRenderingFrameGraph::SHADOW_RENDERVIEW = "shadow";
const QString QgsShadowRenderingFrameGraph::AXIS3D_RENDERVIEW = "3daxis";

Qt3DRender::QFrameGraphNode *QgsShadowRenderingFrameGraph::constructTexturesPreviewPass()
{
  mPreviewLayerFilter = new Qt3DRender::QLayerFilter;
  mPreviewLayerFilter->addLayer( mPreviewLayer );

  mPreviewRenderStateSet = new Qt3DRender::QRenderStateSet( mPreviewLayerFilter );
  mPreviewDepthTest = new Qt3DRender::QDepthTest;
  mPreviewDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );
  mPreviewRenderStateSet->addRenderState( mPreviewDepthTest );
  mPreviewCullFace = new Qt3DRender::QCullFace;
  mPreviewCullFace->setMode( Qt3DRender::QCullFace::NoCulling );
  mPreviewRenderStateSet->addRenderState( mPreviewCullFace );

  return mPreviewLayerFilter;
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
  mAmbientOcclusionRenderCameraSelector = new Qt3DRender::QCameraSelector;
  mAmbientOcclusionRenderCameraSelector->setCamera( mMainCamera );

  mAmbientOcclusionRenderStateSet = new Qt3DRender::QRenderStateSet( mAmbientOcclusionRenderCameraSelector );

  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );;
  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  mAmbientOcclusionRenderStateSet->addRenderState( depthRenderDepthTest );
  mAmbientOcclusionRenderStateSet->addRenderState( depthRenderCullFace );

  mAmbientOcclusionRenderLayerFilter = new Qt3DRender::QLayerFilter( mAmbientOcclusionRenderStateSet );

  mAmbientOcclusionRenderCaptureTargetSelector = new Qt3DRender::QRenderTargetSelector( mAmbientOcclusionRenderLayerFilter );
  Qt3DRender::QRenderTarget *depthRenderTarget = new Qt3DRender::QRenderTarget( mAmbientOcclusionRenderCaptureTargetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput( depthRenderTarget );
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mAmbientOcclusionRenderTexture = new Qt3DRender::QTexture2D( colorOutput );
  mAmbientOcclusionRenderTexture->setSize( mSize.width(), mSize.height() );
  mAmbientOcclusionRenderTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
  mAmbientOcclusionRenderTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mAmbientOcclusionRenderTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( mAmbientOcclusionRenderTexture );
  depthRenderTarget->addOutput( colorOutput );

  mAmbientOcclusionRenderCaptureTargetSelector->setTarget( depthRenderTarget );

  QgsAbstractRenderView *frv = renderView( QgsShadowRenderingFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = frv->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  mAmbientOcclusionRenderEntity = new QgsAmbientOcclusionRenderEntity( forwardDepthTexture, mMainCamera, mRootEntity );
  mAmbientOcclusionRenderLayerFilter->addLayer( mAmbientOcclusionRenderEntity->layer() );

  return mAmbientOcclusionRenderCameraSelector;
}

Qt3DRender::QFrameGraphNode *QgsShadowRenderingFrameGraph::constructAmbientOcclusionBlurPass()
{
  mAmbientOcclusionBlurCameraSelector = new Qt3DRender::QCameraSelector;
  mAmbientOcclusionBlurCameraSelector->setCamera( mMainCamera );

  mAmbientOcclusionBlurStateSet = new Qt3DRender::QRenderStateSet( mAmbientOcclusionBlurCameraSelector );

  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );;
  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  mAmbientOcclusionBlurStateSet->addRenderState( depthRenderDepthTest );
  mAmbientOcclusionBlurStateSet->addRenderState( depthRenderCullFace );

  mAmbientOcclusionBlurLayerFilter = new Qt3DRender::QLayerFilter( mAmbientOcclusionBlurStateSet );

  mAmbientOcclusionBlurRenderCaptureTargetSelector = new Qt3DRender::QRenderTargetSelector( mAmbientOcclusionBlurLayerFilter );
  Qt3DRender::QRenderTarget *depthRenderTarget = new Qt3DRender::QRenderTarget( mAmbientOcclusionBlurRenderCaptureTargetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput( depthRenderTarget );
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mAmbientOcclusionBlurTexture = new Qt3DRender::QTexture2D( colorOutput );
  mAmbientOcclusionBlurTexture->setSize( mSize.width(), mSize.height() );
  mAmbientOcclusionBlurTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
  mAmbientOcclusionBlurTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mAmbientOcclusionBlurTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( mAmbientOcclusionBlurTexture );
  depthRenderTarget->addOutput( colorOutput );

  mAmbientOcclusionBlurRenderCaptureTargetSelector->setTarget( depthRenderTarget );

  mAmbientOcclusionBlurEntity = new QgsAmbientOcclusionBlurEntity( mAmbientOcclusionRenderTexture, mRootEntity );
  mAmbientOcclusionBlurLayerFilter->addLayer( mAmbientOcclusionBlurEntity->layer() );

  return mAmbientOcclusionBlurCameraSelector;
}



Qt3DRender::QFrameGraphNode *QgsShadowRenderingFrameGraph::constructDepthRenderPass()
{
  // depth buffer render to copy pass

  mDepthRenderCameraSelector = new Qt3DRender::QCameraSelector;
  mDepthRenderCameraSelector->setCamera( mMainCamera );

  mDepthRenderStateSet = new Qt3DRender::QRenderStateSet( mDepthRenderCameraSelector );

  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );;
  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  mDepthRenderStateSet->addRenderState( depthRenderDepthTest );
  mDepthRenderStateSet->addRenderState( depthRenderCullFace );

  mDepthRenderLayerFilter = new Qt3DRender::QLayerFilter( mDepthRenderStateSet );
  mDepthRenderLayerFilter->addLayer( mDepthRenderPassLayer );

  mDepthRenderCaptureTargetSelector = new Qt3DRender::QRenderTargetSelector( mDepthRenderLayerFilter );
  Qt3DRender::QRenderTarget *depthRenderTarget = new Qt3DRender::QRenderTarget( mDepthRenderCaptureTargetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput( depthRenderTarget );
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mDepthRenderCaptureColorTexture = new Qt3DRender::QTexture2D( colorOutput );
  mDepthRenderCaptureColorTexture->setSize( mSize.width(), mSize.height() );
  mDepthRenderCaptureColorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  mDepthRenderCaptureColorTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mDepthRenderCaptureColorTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( mDepthRenderCaptureColorTexture );
  depthRenderTarget->addOutput( colorOutput );

  Qt3DRender::QRenderTargetOutput *depthOutput = new Qt3DRender::QRenderTargetOutput( depthRenderTarget );

  depthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  mDepthRenderCaptureDepthTexture = new Qt3DRender::QTexture2D( depthOutput );
  mDepthRenderCaptureDepthTexture->setSize( mSize.width(), mSize.height() );
  mDepthRenderCaptureDepthTexture->setFormat( Qt3DRender::QAbstractTexture::DepthFormat );
  mDepthRenderCaptureDepthTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mDepthRenderCaptureDepthTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mDepthRenderCaptureDepthTexture->setComparisonFunction( Qt3DRender::QAbstractTexture::CompareLessEqual );
  mDepthRenderCaptureDepthTexture->setComparisonMode( Qt3DRender::QAbstractTexture::CompareRefToTexture );

  depthOutput->setTexture( mDepthRenderCaptureDepthTexture );
  depthRenderTarget->addOutput( depthOutput );

  mDepthRenderCaptureTargetSelector->setTarget( depthRenderTarget );

  // Note: We do not a clear buffers node since we are drawing a quad that will override the buffer's content anyway
  mDepthRenderCapture = new Qt3DRender::QRenderCapture( mDepthRenderCaptureTargetSelector );

  return mDepthRenderCameraSelector;
}


Qt3DCore::QEntity *QgsShadowRenderingFrameGraph::constructDepthRenderQuad()
{
  Qt3DCore::QEntity *quad = new Qt3DCore::QEntity;
  quad->setObjectName( "depthRenderQuad" );

  Qt3DQGeometry *geom = new Qt3DQGeometry;
  Qt3DQAttribute *positionAttribute = new Qt3DQAttribute;
  const QVector<float> vert = { -1.0f, -1.0f, 1.0f, /**/ 1.0f, -1.0f, 1.0f, /**/ -1.0f,  1.0f, 1.0f, /**/ -1.0f,  1.0f, 1.0f, /**/ 1.0f, -1.0f, 1.0f, /**/ 1.0f,  1.0f, 1.0f };

  const QByteArray vertexArr( ( const char * ) vert.constData(), vert.size() * sizeof( float ) );
  Qt3DQBuffer *vertexBuffer = nullptr;
  vertexBuffer = new Qt3DQBuffer( this );
  vertexBuffer->setData( vertexArr );

  positionAttribute->setName( Qt3DQAttribute::defaultPositionAttributeName() );
  positionAttribute->setVertexBaseType( Qt3DQAttribute::Float );
  positionAttribute->setVertexSize( 3 );
  positionAttribute->setAttributeType( Qt3DQAttribute::VertexAttribute );
  positionAttribute->setBuffer( vertexBuffer );
  positionAttribute->setByteOffset( 0 );
  positionAttribute->setByteStride( 3 * sizeof( float ) );
  positionAttribute->setCount( 6 );

  geom->addAttribute( positionAttribute );

  Qt3DRender::QGeometryRenderer *renderer = new Qt3DRender::QGeometryRenderer;
  renderer->setPrimitiveType( Qt3DRender::QGeometryRenderer::PrimitiveType::Triangles );
  renderer->setGeometry( geom );

  quad->addComponent( renderer );

  QMatrix4x4 modelMatrix;
  modelMatrix.setToIdentity();

  // construct material

  Qt3DRender::QMaterial *material = new Qt3DRender::QMaterial;
  QgsAbstractRenderView *frv = renderView( FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = frv->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  Qt3DRender::QParameter *textureParameter = new Qt3DRender::QParameter( "depthTexture", forwardDepthTexture );
  Qt3DRender::QParameter *textureTransformParameter = new Qt3DRender::QParameter( "modelMatrix", QVariant::fromValue( modelMatrix ) );
  material->addParameter( textureParameter );
  material->addParameter( textureTransformParameter );

  Qt3DRender::QEffect *effect = new Qt3DRender::QEffect;

  Qt3DRender::QTechnique *technique = new Qt3DRender::QTechnique;

  Qt3DRender::QGraphicsApiFilter *graphicsApiFilter = technique->graphicsApiFilter();
  graphicsApiFilter->setApi( Qt3DRender::QGraphicsApiFilter::Api::OpenGL );
  graphicsApiFilter->setProfile( Qt3DRender::QGraphicsApiFilter::OpenGLProfile::CoreProfile );
  graphicsApiFilter->setMajorVersion( 1 );
  graphicsApiFilter->setMinorVersion( 5 );

  Qt3DRender::QRenderPass *renderPass = new Qt3DRender::QRenderPass;

  Qt3DRender::QShaderProgram *shader = new Qt3DRender::QShaderProgram;
  shader->setVertexShaderCode( Qt3DRender::QShaderProgram::loadSource( QUrl( "qrc:/shaders/depth_render.vert" ) ) );
  shader->setFragmentShaderCode( Qt3DRender::QShaderProgram::loadSource( QUrl( "qrc:/shaders/depth_render.frag" ) ) );
  renderPass->setShaderProgram( shader );

  technique->addRenderPass( renderPass );

  effect->addTechnique( technique );
  material->setEffect( effect );

  quad->addComponent( material );

  return quad;
}

QgsShadowRenderingFrameGraph::QgsShadowRenderingFrameGraph( QSurface *surface, QSize s, Qt3DRender::QCamera *mainCamera, Qt3DCore::QEntity *root )
  : Qt3DCore::QEntity( root )
  , mSize( s )
{
  mRootEntity = root;
  mMainCamera = mainCamera;

  mPreviewLayer = new Qt3DRender::QLayer;
  mDepthRenderPassLayer = new Qt3DRender::QLayer;

  mPreviewLayer->setRecursive( true );
  mDepthRenderPassLayer->setRecursive( true );

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
  Qt3DRender::QFrameGraphNode *depthBufferProcessingPass = constructDepthRenderPass();
  depthBufferProcessingPass->setParent( mMainViewPort );

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
  Qt3DRender::QFrameGraphNode *previewPass = constructTexturesPreviewPass();
  previewPass->setParent( mMainViewPort );

  Qt3DRender::QParameter *depthMapIsDepthParam = new Qt3DRender::QParameter( "isDepth", true );
  Qt3DRender::QParameter *shadowMapIsDepthParam = new Qt3DRender::QParameter( "isDepth", true );

  QgsAbstractRenderView *frv = renderView( QgsShadowRenderingFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = frv->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  mDebugDepthMapPreviewQuad = this->addTexturePreviewOverlay( forwardDepthTexture, QPointF( 0.9f, 0.9f ), QSizeF( 0.1, 0.1 ), QVector<Qt3DRender::QParameter *> { depthMapIsDepthParam } );

  QgsAbstractRenderView *srv = renderView( QgsShadowRenderingFrameGraph::SHADOW_RENDERVIEW );
  Qt3DRender::QTexture2D *shadowDepthTexture = srv->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  mDebugShadowMapPreviewQuad = this->addTexturePreviewOverlay( shadowDepthTexture, QPointF( 0.9f, 0.9f ), QSizeF( 0.1, 0.1 ), QVector<Qt3DRender::QParameter *> { shadowMapIsDepthParam } );

  mDebugDepthMapPreviewQuad->setEnabled( false );
  mDebugShadowMapPreviewQuad->setEnabled( false );

  mDepthRenderQuad = constructDepthRenderQuad();
  mDepthRenderQuad->addComponent( mDepthRenderPassLayer );
  mDepthRenderQuad->setParent( mRootEntity );
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

bool QgsShadowRenderingFrameGraph::isRenderViewEnabled( const QString &name )
{
  return mRenderViewMap [name] != nullptr && mRenderViewMap [name]->isSubTreeEnabled();
}

QgsPreviewQuad *QgsShadowRenderingFrameGraph::addTexturePreviewOverlay( Qt3DRender::QTexture2D *texture, const QPointF &centerTexCoords, const QSizeF &sizeTexCoords, QVector<Qt3DRender::QParameter *> additionalShaderParameters )
{
  QgsPreviewQuad *previewQuad = new QgsPreviewQuad( texture, centerTexCoords, sizeTexCoords, additionalShaderParameters );
  previewQuad->addComponent( mPreviewLayer );
  previewQuad->setParent( mRootEntity );
  mPreviewQuads.push_back( previewQuad );
  return previewQuad;
}


void QgsShadowRenderingFrameGraph::setClearColor( const QColor &clearColor )
{
  QgsForwardRenderView *frv = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( frv )
    frv->setClearColor( clearColor );
}

void QgsShadowRenderingFrameGraph::setAmbientOcclusionEnabled( bool enabled )
{
  mAmbientOcclusionEnabled = enabled;
  mAmbientOcclusionRenderEntity->setEnabled( enabled );
  mPostprocessingEntity->setAmbientOcclusionEnabled( enabled );
}

void QgsShadowRenderingFrameGraph::setAmbientOcclusionIntensity( float intensity )
{
  mAmbientOcclusionIntensity = intensity;
  mAmbientOcclusionRenderEntity->setIntensity( intensity );
}

void QgsShadowRenderingFrameGraph::setAmbientOcclusionRadius( float radius )
{
  mAmbientOcclusionRadius = radius;
  mAmbientOcclusionRenderEntity->setRadius( radius );
}

void QgsShadowRenderingFrameGraph::setAmbientOcclusionThreshold( float threshold )
{
  mAmbientOcclusionThreshold = threshold;
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
  mEyeDomeLightingEnabled = enabled;
  mEyeDomeLightingStrength = strength;
  mEyeDomeLightingDistance = distance;
  mPostprocessingEntity->setEyeDomeLightingEnabled( enabled );
  mPostprocessingEntity->setEyeDomeLightingStrength( strength );
  mPostprocessingEntity->setEyeDomeLightingDistance( distance );
}

void QgsShadowRenderingFrameGraph::setupShadowMapDebugging( bool enabled, Qt::Corner corner, double size )
{
  mDebugShadowMapPreviewQuad->setEnabled( enabled );
  if ( enabled )
  {
    switch ( corner )
    {
      case Qt::Corner::TopRightCorner:
        mDebugShadowMapPreviewQuad->setViewPort( QPointF( 1.0f - size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::TopLeftCorner:
        mDebugShadowMapPreviewQuad->setViewPort( QPointF( 0.0f + size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomRightCorner:
        mDebugShadowMapPreviewQuad->setViewPort( QPointF( 1.0f - size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomLeftCorner:
        mDebugShadowMapPreviewQuad->setViewPort( QPointF( 0.0f + size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
    }
  }
}

void QgsShadowRenderingFrameGraph::setupDepthMapDebugging( bool enabled, Qt::Corner corner, double size )
{
  mDebugDepthMapPreviewQuad->setEnabled( enabled );

  if ( enabled )
  {
    switch ( corner )
    {
      case Qt::Corner::TopRightCorner:
        mDebugDepthMapPreviewQuad->setViewPort( QPointF( 1.0f - size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::TopLeftCorner:
        mDebugDepthMapPreviewQuad->setViewPort( QPointF( 0.0f + size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomRightCorner:
        mDebugDepthMapPreviewQuad->setViewPort( QPointF( 1.0f - size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomLeftCorner:
        mDebugDepthMapPreviewQuad->setViewPort( QPointF( 0.0f + size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
    }
  }
}

void QgsShadowRenderingFrameGraph::setSize( QSize s )
{
  mSize = s;
  QgsForwardRenderView *frv = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( frv )
    frv->updateTargetOutputSize( mSize.width(), mSize.height() );

  mRenderCaptureColorTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureDepthTexture->setSize( mSize.width(), mSize.height() );
  mDepthRenderCaptureDepthTexture->setSize( mSize.width(), mSize.height() );
  mDepthRenderCaptureColorTexture->setSize( mSize.width(), mSize.height() );
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
