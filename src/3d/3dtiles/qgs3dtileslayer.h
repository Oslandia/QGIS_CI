/***************************************************************************
  qgs3dtileslayer.h
  --------------------------------------
  Date                 : Mars 2021
  Copyright            : (C) 2021 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DTILESLAYER_H
#define QGS3DTILESLAYER_H

#include "qgsmeshlayer.h"
#include "qgs3dtileslayer3drenderer.h"
#include "3dtiles.h"

class _3D_EXPORT Qgs3dTilesLayer : public QgsMeshLayer
{
    Q_OBJECT
  public:
    Tileset *mTileset;

  public:
    Qgs3dTilesLayer( Tileset *tileset );

    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsMapLayerRenderer *createMapRenderer( ) ;

};

#endif // QGS3DTILESLAYER_H
