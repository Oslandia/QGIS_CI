/***************************************************************************
  qgsdepthentity.h
  --------------------------------------
  Date                 : December 2022
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSDEPTHENTITY_H
#define QGSDEPTHENTITY_H

#include "qgsrenderpassquad.h"

#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief An entity that is responsible for capturing depth.
 *
 * \note Not available in Python bindings
 *
 * \since QGIS 3.30
 */
class QgsDepthEntity : public QgsRenderPassQuad
{
    Q_OBJECT

  public:
    //! Constructor
    QgsDepthEntity( Qt3DRender::QTexture2D *texture, QNode *parent = nullptr );

//    //! Sets the parts of the scene where objects cast shadows
//    void setupShadowRenderingExtent( float minX, float maxX, float minY, float maxY, float minZ, float maxZ );
//    //! Sets up a directional light that is used to render shadows
//    void setupDirectionalLight( QVector3D position, QVector3D direction );
//    //! Sets whether shadow rendering is enabled
//    void setShadowRenderingEnabled( bool enabled );
//    //! Sets the shadow bias value
//    void setShadowBias( float shadowBias );
//    //! Sets whether eye dome lighting is enabled
//    void setEyeDomeLightingEnabled( bool enabled );
//    //! Sets the eye dome lighting strength
//    void setEyeDomeLightingStrength( double strength );
//    //! Sets the eye dome lighting distance (contributes to the contrast of the image)
//    void setEyeDomeLightingDistance( int distance );

//    /**
//     * Sets whether screen space ambient occlusion is enabled
//     * \since QGIS 3.28
//     */
//    void setAmbientOcclusionEnabled( bool enabled );

  private:
    Qt3DRender::QParameter *mTextureParameter = nullptr;
    Qt3DRender::QParameter *mTextureTransformParameter = nullptr;
};

#endif // QGSDEPTHENTITY_H
