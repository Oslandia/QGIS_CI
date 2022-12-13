/***************************************************************************
  qgsdebugtextureentity.h
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

#ifndef QGSDEBUGTEXTUREENTITY_H
#define QGSDEBUGTEXTUREENTITY_H

#include "qgspreviewquad.h"

class QgsShadowRenderingFrameGraph;
namespace Qt3DRender
{
  class QTexture2D;
}

#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief An entity that is responsible for debugging texture.
 *
 * \note Not available in Python bindings
 *
 * \since QGIS 3.30
 */
class QgsDebugTextureEntity : public QgsPreviewQuad
{
    Q_OBJECT

  public:
    //! Constructor
    QgsDebugTextureEntity( QgsShadowRenderingFrameGraph *frameGraph, Qt3DRender::QTexture2D *texture );

    //! Sets the texture debugging parameters
    void onSettingsChanged( bool enabled, Qt::Corner corner, double size );
};

#endif // QGSDEBUGTEXTUREENTITY_H
