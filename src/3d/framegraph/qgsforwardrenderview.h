/***************************************************************************
  qgsforwardrenderview.h
  --------------------------------------
  Date                 : august 2022
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

#ifndef QGSFORWARDRENDERVIEW_H
#define QGSFORWARDRENDERVIEW_H

#include "qgsabstractrenderview.h"
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#include <Qt3DRender/QDebugOverlay>
#endif

namespace Qt3DRender
{
  class QRenderSettings;
  class QLayer;
  class QSubtreeEnabler;
  class QTexture2D;
  class QCamera;
  class QCameraSelector;
  class QLayerFilter;
  class QRenderTargetSelector;
  class QClearBuffers;
  class QFrustumCulling;
}

/**
 * \ingroup 3d
 * \brief Container class that holds different objects related to forward rendering
 *
 * \note Not available in Python bindings
 *
 * \since QGIS 3.28
 */
class QgsForwardRenderView : public QgsAbstractRenderView
{
    Q_OBJECT
  public:
    QgsForwardRenderView( QObject *parent, Qt3DRender::QCamera *mainCamera );

    //! Returns the layer to be used by entities to be included in this renderview
    virtual Qt3DRender::QLayer *layerToFilter();

    //! Returns the viewport associated to this renderview
    virtual Qt3DRender::QViewport *viewport();

    //! Returns the top node of this renderview branch. Should be a QSubtreeEnabler
    virtual Qt3DRender::QFrameGraphNode *topGraphNode();

    //! Enable or disable via \a enable the renderview sub tree
    virtual void enableSubTree( bool enable );

    //! Returns true if renderview is enabled
    virtual bool isSubTreeEnabled();

    //! Returns a layer object used to indicate that the object is transparent
    Qt3DRender::QLayer *transparentObjectLayer() { return mTransparentObjectsLayer; }

    //! Sets the clear color of the scene (background color)
    void setClearColor( const QColor &clearColor );

    //! Returns whether frustum culling is enabled
    bool isFrustumCullingEnabled() const { return mFrustumCullingEnabled; }
    //! Sets whether frustum culling is enabled
    void enableFrustumCulling( bool enabled );

    /**
     * Sets whether debug overlay is enabled
     * \since QGIS 3.26
     */
    void enableDebugOverlay( bool enabled );

  private:
    Qt3DRender::QCamera *mMainCamera = nullptr;

    Qt3DRender::QSubtreeEnabler *mRendererEnabler = nullptr;
    Qt3DRender::QCameraSelector *mMainCameraSelector = nullptr;
    Qt3DRender::QLayerFilter *mLayerFilter = nullptr;
    Qt3DRender::QLayer *mTransparentObjectsLayer = nullptr;
    Qt3DRender::QLayer *mLayer = nullptr;
    Qt3DRender::QRenderTargetSelector *mRenderTargetSelector = nullptr;
    Qt3DRender::QClearBuffers *mClearBuffers = nullptr;
    bool mFrustumCullingEnabled = true;
    Qt3DRender::QFrustumCulling *mFrustumCulling = nullptr;
    // Forward rendering pass texture related objects:
    Qt3DRender::QTexture2D *mColorTexture = nullptr;
    Qt3DRender::QTexture2D *mDepthTexture = nullptr;
    // QDebugOverlay added in the forward pass
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    Qt3DRender::QDebugOverlay *mDebugOverlay = nullptr;
#endif

    Qt3DRender::QFrameGraphNode *constructRenderPass();

    //! Handles target outputs changes
    virtual void onTargetOutputUpdate();
};

#endif // QGSFORWARDRENDERVIEW_H
