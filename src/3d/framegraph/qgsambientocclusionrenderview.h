/***************************************************************************
  qgsambientocclusionrenderview.h
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

#ifndef QGSAMBIENTOCCLUSIONRENDERVIEW_H
#define QGSAMBIENTOCCLUSIONRENDERVIEW_H

#include "qgsabstractrenderview.h"

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
  class QRenderStateSet;
  class QRenderCapture;
}

/**
 * \ingroup 3d
 * \brief Container class that holds different objects related to depth rendering
 *
 * \note Not available in Python bindings
 *
 * The depth buffer render pass is made to copy the depth buffer into
 * an RGB texture that can be captured into a QImage and sent to the CPU for
 * calculating real 3D points from mouse coordinates (for zoom, rotation, drag..)
 *
 * \since QGIS 3.28
 */
class QgsAmbientOcclusionRenderView : public QgsAbstractRenderView
{
    Q_OBJECT
  public:
    QgsAmbientOcclusionRenderView( QObject *parent, Qt3DRender::QCamera *mainCamera );

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

    //! Returns the render capture object used to take an image of the depth buffer of the scene
    Qt3DRender::QRenderCapture *renderCapture() { return mDepthRenderCapture; }

  private:
    Qt3DRender::QCamera *mMainCamera = nullptr;

    Qt3DRender::QSubtreeEnabler *mRendererEnabler = nullptr;
    Qt3DRender::QLayer *mLayer = nullptr;
    Qt3DRender::QRenderTargetSelector *mRenderTargetSelector = nullptr;
    Qt3DRender::QRenderCapture *mDepthRenderCapture = nullptr;

    Qt3DRender::QFrameGraphNode *constructRenderPass();

    //! Handles target outputs changes
    virtual void onTargetOutputUpdate();
};

#endif // QGSAMBIENTOCCLUSIONRENDERVIEW_H
