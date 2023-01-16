/***************************************************************************
  qgsshadowrenderingframegraph.h
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

#ifndef QGSSHADOWRENDERINGFRAMEGRAPH_H
#define QGSSHADOWRENDERINGFRAMEGRAPH_H

#include <QWindow>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QRenderSurfaceSelector>
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#include <Qt3DRender/QDebugOverlay>
#endif

#include "qgspointlightsettings.h"
#include "qgsambientocclusionrenderentity.h"

class QgsDirectionalLightSettings;
class QgsCameraController;
class QgsRectangle;
class QgsPostprocessingEntity;
class QgsAmbientOcclusionBlurEntity;
class QgsAbstractRenderView;
class QgsDebugTextureEntity;

#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief Container class that holds different objects related to shadow rendering
 *
 * \note Not available in Python bindings
 *
 * \since QGIS 3.16
 */
class QgsShadowRenderingFrameGraph : public Qt3DCore::QEntity
{
    Q_OBJECT

  public:
    //! Constructor
    QgsShadowRenderingFrameGraph( QSurface *surface, QSize s, Qt3DRender::QCamera *mainCamera, Qt3DCore::QEntity *root );

    virtual ~QgsShadowRenderingFrameGraph();

    //! Returns the root of the frame graph object
    Qt3DRender::QFrameGraphNode *frameGraphRoot() { return mRenderSurfaceSelector; }

    /**
     * Returns blurred ambient occlusion factor values texture
     * \since QGIS 3.28
     */
    Qt3DRender::QTexture2D *blurredAmbientOcclusionFactorMap() { return mAmbientOcclusionBlurTexture; }

    /**
     * Returns a layer object used to indicate that the object is transparent
     * \since QGIS 3.26
     * TODO should not be there
     */
    Qt3DRender::QLayer *transparentObjectLayer();

    //! Returns the main camera
    Qt3DRender::QCamera *mainCamera() { return mMainCamera; }

    //! Returns the root entity
    Qt3DCore::QEntity *rootEntity() { return mRootEntity; }

    //! Returns the postprocessing entity
    QgsPostprocessingEntity *postprocessingEntity() { return mPostprocessingEntity; }

    //! Returns the render capture object used to take an image of the scene
    Qt3DRender::QRenderCapture *renderCapture() { return mRenderCapture; }

    //! Returns the render capture object used to take an image of the depth buffer of the scene
    Qt3DRender::QRenderCapture *depthRenderCapture();

    //! Sets whether frustum culling is enabled
    void setFrustumCullingEnabled( bool enabled );

    //! Sets the clear color of the scene (background color)
    void setClearColor( const QColor &clearColor );

    //! Sets eye dome lighting shading related settings
    void setupEyeDomeLighting( bool enabled, double strength, int distance );
    //! Sets the size of the buffers used for rendering
    void setSize( QSize s );

    /**
     * Sets whether it will be possible to render to an image
     * \since QGIS 3.18
     */
    void setRenderCaptureEnabled( bool enabled );

    /**
     * Returns whether it will be possible to render to an image
     * \since QGIS 3.18
     */
    bool renderCaptureEnabled() const { return mRenderCaptureEnabled; }

    /**
     * Sets whether debug overlay is enabled
     * \since QGIS 3.26
     */
    void setDebugOverlayEnabled( bool enabled );

    //! Registers a new the render view \a renderView with name \a name
    bool registerRenderView( QgsAbstractRenderView *renderView, const QString &name );

    //! Unregisters the render view named \a name, if any
    void unregisterRenderView( const QString &name );

    //! Enables or disables the render view named \a name according to \a enable
    void setEnableRenderView( const QString &name, bool enable );

    //! Returns true if the render view named \a name is enabled
    bool isRenderViewEnabled( const QString &name );

    //! Returns the render view named \a name, if any
    QgsAbstractRenderView *renderView( const QString &name );

    //! Returns the layer used to assign entities to the render view named \a name, if any
    Qt3DRender::QLayer *filterLayer( const QString &name );

    static const QString FORWARD_RENDERVIEW;
    static const QString SHADOW_RENDERVIEW;
    static const QString AXIS3D_RENDERVIEW;
    static const QString DEPTH_RENDERVIEW;
    static const QString DEBUG_RENDERVIEW;
    //! Ambient occlusion render view name
    static const QString AO_RENDERVIEW;

  private:
    Qt3DRender::QRenderSurfaceSelector *mRenderSurfaceSelector = nullptr;
    Qt3DRender::QViewport *mMainViewPort = nullptr;

    Qt3DRender::QCamera *mMainCamera = nullptr;

    // Post processing pass branch nodes:
    Qt3DRender::QCameraSelector *mPostProcessingCameraSelector = nullptr;
    Qt3DRender::QLayerFilter *mPostprocessPassLayerFilter = nullptr;
    Qt3DRender::QClearBuffers *mPostprocessClearBuffers = nullptr;
    Qt3DRender::QRenderTargetSelector *mRenderCaptureTargetSelector = nullptr;
    Qt3DRender::QRenderCapture *mRenderCapture = nullptr;
    // Post processing pass texture related objects:
    Qt3DRender::QTexture2D *mRenderCaptureColorTexture = nullptr;
    Qt3DRender::QTexture2D *mRenderCaptureDepthTexture = nullptr;

    // Ambient occlusion factor blur pass texture related objects:
    Qt3DRender::QTexture2D *mAmbientOcclusionBlurTexture = nullptr;

    static constexpr int mDefaultShadowMapResolution = 2048;

    QSize mSize = QSize( 1024, 768 );

    QVector3D mLightDirection = QVector3D( 0.0, -1.0f, 0.0f );

    Qt3DCore::QEntity *mRootEntity = nullptr;

    QgsPostprocessingEntity *mPostprocessingEntity = nullptr;

    void constructShadowRenderPass();
    void constructForwardRenderPass();
    void constructDebugTexturePass();
    Qt3DRender::QFrameGraphNode *constructPostprocessingPass();
    void constructDepthRenderPass();
    void constructAmbientOcclusionRenderPass();
    Qt3DRender::QFrameGraphNode *constructAmbientOcclusionBlurPass();

    bool mRenderCaptureEnabled = true;

    QMap<QString, QgsAbstractRenderView *> mRenderViewMap;

    Q_DISABLE_COPY( QgsShadowRenderingFrameGraph )
};

#endif // QGSSHADOWRENDERINGFRAMEGRAPH_H
