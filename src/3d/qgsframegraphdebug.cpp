#include "qgsframegraphdebug.h"
#include "qgspreviewquad.h"

QgsFrameGraphDebug::QgsFrameGraphDebug( Qt3DCore::QEntity *root, Qt3DRender::QTexture2D *forwardDepthTexture, Qt3DRender::QTexture2D *shadowMapTexture )
  : mRootEntity( root )
{

  Qt3DRender::QParameter *depthMapIsDepthParam = new Qt3DRender::QParameter( "isDepth", true );
  Qt3DRender::QParameter *shadowMapIsDepthParam = new Qt3DRender::QParameter( "isDepth", true );

  mDebugDepthMapPreviewQuad = this->addTexturePreviewOverlay( forwardDepthTexture, QPointF( 0.9f, 0.9f ), QSizeF( 0.1, 0.1 ), QVector<Qt3DRender::QParameter *> { depthMapIsDepthParam } );
  mDebugShadowMapPreviewQuad = this->addTexturePreviewOverlay( shadowMapTexture, QPointF( 0.9f, 0.9f ), QSizeF( 0.1, 0.1 ), QVector<Qt3DRender::QParameter *> { shadowMapIsDepthParam } );
  mDebugDepthMapPreviewQuad->setEnabled( false );
  mDebugShadowMapPreviewQuad->setEnabled( false );
}


QgsPreviewQuad *QgsFrameGraphDebug::addTexturePreviewOverlay( Qt3DRender::QTexture2D *texture, const QPointF &centerTexCoords, const QSizeF &sizeTexCoords, QVector<Qt3DRender::QParameter *> additionalShaderParameters )
{
  QgsPreviewQuad *previewQuad = new QgsPreviewQuad( texture, centerTexCoords, sizeTexCoords, additionalShaderParameters );
  previewQuad->addComponent( mPreviewLayer );
  previewQuad->setParent( mRootEntity );
  mPreviewQuads.push_back( previewQuad );
  return previewQuad;
}



void QgsFrameGraphDebug::setupShadowMapDebugging( bool enabled, Qt::Corner corner, double size )
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

void QgsFrameGraphDebug::setupDepthMapDebugging( bool enabled, Qt::Corner corner, double size )
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
