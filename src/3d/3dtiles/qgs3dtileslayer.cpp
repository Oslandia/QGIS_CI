/***************************************************************************
  qgs3dtileslayer.cpp
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

#include "qgs3dtileslayer.h"

Qgs3dTilesLayer::Qgs3dTilesLayer( Tileset *tileset ) :
  QgsMeshLayer(), mTileset( tileset )
{

}

QgsMapLayerRenderer *Qgs3dTilesLayer::createMapRenderer( QgsRenderContext & )
{
  return createMapRenderer( );
}

QgsMapLayerRenderer *Qgs3dTilesLayer::createMapRenderer( )
{
  return ( QgsMapLayerRenderer * )new Qgs3dTilesLayer3DRenderer( this );
}

