/***************************************************************************
  qgsshadowrenderview.h
  --------------------------------------
  Date                 : May 2022
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

#ifndef QGSSHADOWRENDERVIEW_H
#define QGSSHADOWRENDERVIEW_H

#include "qgsabstractrenderview.h"

class QColor;
class QRect;
class QSurface;

namespace Qt3DCore
{
  class QEntity;
}

namespace Qt3DRender
{
  class QRenderSettings;
  class QCamera;
  class QFrameGraphNode;
  class QLayer;
  class QViewport;
  class QSubtreeEnabler;
  class QTexture2D;
  class QCameraSelector;
  class QLayerFilter;
  class QRenderTargetSelector;
  class QClearBuffers;
  class QRenderStateSet;
}

namespace Qt3DExtras
{
  class Qt3DWindow;
}

class QgsShadowRenderingFrameGraph;
class Qgs3DMapSettings;

/**
 * \ingroup 3d
 * \brief Container class that holds different objects related to shadow rendering
 *
 * \note Not available in Python bindings
 *
 * \since QGIS 3.26
 */
class QgsShadowRenderView : public QgsAbstractRenderView
{
    Q_OBJECT
  public:
    QgsShadowRenderView();

    //! Sets root entity of the 3D scene
    virtual void setRootEntity( Qt3DCore::QEntity *root ) ;

    //! Returns the layer to be used by entities to be included in this renderview
    virtual Qt3DRender::QLayer *layerToFilter();

    //! Returns the viewport associated to this renderview
    virtual Qt3DRender::QViewport *viewport();

    //! Returns the top node of this renderview branch. Should be a QSubtreeEnabler
    virtual Qt3DRender::QFrameGraphNode *topGraphNode();

    //! Enable or disable via \a enable the renderview sub tree
    virtual void enableSubTree( bool enable );

    //! Returns the shadow map (a depth texture for the shadow rendering pass)
    Qt3DRender::QTexture2D *shadowMapTexture() { return mShadowMapTexture; }

    //! Returns a layer object used to indicate that an entity will cast shadows
    Qt3DRender::QLayer *castShadowsLayer() { return mCastShadowsLayer; }

    //! Returns whether shadow rendering is enabled
    bool shadowRenderingEnabled() const { return mShadowRenderingEnabled; }
    //! Sets whether the shadow rendering is enabled
    void setShadowRenderingEnabled( bool enabled );

    //! Returns the shadow bias value
    float shadowBias() const { return mShadowBias; }
    //! Sets the shadow bias value
    void setShadowBias( float shadowBias );

    //! Returns the shadow map resolution
    int shadowMapResolution() const { return mShadowMapResolution; }
    //! Sets the resolution of the shadow map
    void setShadowMapResolution( int resolution );

  private:
    bool mShadowRenderingEnabled = false;
    float mShadowBias = 0.00001f;
    int mShadowMapResolution = 2048;

    Qt3DRender::QLayer *mCastShadowsLayer = nullptr;
    // Shadow rendering pass branch nodes:
    Qt3DRender::QCameraSelector *mLightCameraSelectorShadowPass = nullptr;
    Qt3DRender::QLayerFilter *mShadowSceneEntitiesFilter = nullptr;
    Qt3DRender::QRenderTargetSelector *mShadowRenderTargetSelector = nullptr;
    Qt3DRender::QClearBuffers *mShadowClearBuffers = nullptr;
    Qt3DRender::QRenderStateSet *mShadowRenderStateSet = nullptr;
    // Shadow rendering pass texture related objects:
    Qt3DRender::QTexture2D *mShadowMapTexture = nullptr;


};

#endif // QGSSHADOWRENDERVIEW_H
