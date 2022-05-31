#ifndef QGSFRAMEGRAPHDEBUG_H
#define QGSFRAMEGRAPHDEBUG_H

#include <Qt3DRender/QTexture>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QLayer>

class QgsPreviewQuad;

class QgsFrameGraphDebug
{
  public:
    QgsFrameGraphDebug( Qt3DCore::QEntity *rootEntity, Qt3DRender::QTexture2D *forwardDepthTexture, Qt3DRender::QTexture2D *shadowMapTexture );

    //! Adds an preview entity that shows a texture in real time for debugging purposes
    QgsPreviewQuad *addTexturePreviewOverlay( Qt3DRender::QTexture2D *texture, const QPointF &centerNDC, const QSizeF &size, QVector<Qt3DRender::QParameter *> additionalShaderParameters = QVector<Qt3DRender::QParameter *>() );
    //! Sets the shadow map debugging view port
    void setupShadowMapDebugging( bool enabled, Qt::Corner corner, double size );
    //! Sets the depth map debugging view port
    void setupDepthMapDebugging( bool enabled, Qt::Corner corner, double size );

  private:
    QgsPreviewQuad *mDebugShadowMapPreviewQuad = nullptr;
    QgsPreviewQuad *mDebugDepthMapPreviewQuad = nullptr;
    QVector<QgsPreviewQuad *> mPreviewQuads;
    Qt3DCore::QEntity *mRootEntity = nullptr;

    // TODO
    Qt3DRender::QLayer *mPreviewLayer = nullptr;

};

#endif // QGSFRAMEGRAPHDEBUG_H
