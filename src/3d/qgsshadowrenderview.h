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
  class QRenderTargetOutput;
}

namespace Qt3DExtras
{
  class Qt3DWindow;
}

class QgsShadowRenderingFrameGraph;
class Qgs3DMapSettings;
class QgsDirectionalLightSettings;

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
    QgsShadowRenderView( QObject *parent );

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

    //! Returns the shadow bias value
    float shadowBias() const { return mShadowBias; }
    //! Sets the shadow bias value
    void setShadowBias( float shadowBias );

    //! Returns the light camera
    Qt3DRender::QCamera *lightCamera() { return mLightCamera; }

    //! Sets shadow rendering to use a directional light
    void setupDirectionalLight( const QgsDirectionalLightSettings &light, float maximumShadowRenderingDistance,
                                const Qt3DRender::QCamera *mainCamera );

  signals:

    void shadowDirectionLightUpdated( const QVector3D &lightPosition, const QVector3D &lightDirection );
    void shadowExtentChanged( float minX, float maxX, float minY, float maxY, float minZ, float maxZ );
    void shadowBiasChanged( float bias );
    void shadowRenderingEnabled( bool isEnabled );

  private:
    float mShadowBias = 0.00001f;

    Qt3DRender::QSubtreeEnabler *mShadowRendererEnabler = nullptr;
    Qt3DRender::QLayer *mCastShadowsLayer = nullptr;
    // Shadow rendering pass branch nodes:
    Qt3DRender::QCameraSelector *mLightCameraSelectorShadowPass = nullptr;
    Qt3DRender::QLayerFilter *mShadowSceneEntitiesFilter = nullptr;
    Qt3DRender::QRenderTargetSelector *mShadowRenderTargetSelector = nullptr;
    Qt3DRender::QClearBuffers *mShadowClearBuffers = nullptr;
    Qt3DRender::QRenderStateSet *mShadowRenderStateSet = nullptr;

    Qt3DRender::QCamera *mLightCamera = nullptr;

    Qt3DRender::QFrameGraphNode *constructShadowRenderPass();

    //! Handles target outputs changes
    virtual void onTargetOutputUpdate();

    static void calculateViewExtent( const Qt3DRender::QCamera *camera, float shadowRenderingDistance, float y, float &minX, float &maxX, float &minY, float &maxY, float &minZ, float &maxZ );

};

#endif // QGSSHADOWRENDERVIEW_H
