/***************************************************************************
  qgs3dtileslayer3drendered.h
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

#include "qgs3dtileslayer3drenderer.h"

Qgs3dTilesLayer3DRenderer::Qgs3dTilesLayer3DRenderer( Qgs3dTilesLayer *layer ):
  QgsMeshLayer3DRenderer()
{
  setLayer( layer );
}


Qgs3dTilesLayer3DRenderer *Qgs3dTilesLayer3DRenderer::clone() const
{
  Qgs3dTilesLayer3DRenderer *r = new Qgs3dTilesLayer3DRenderer( qobject_cast<Qgs3dTilesLayer *>( mLayerRef.layer ) );
  return r;
}

void Qgs3dTilesLayer3DRenderer::setLayer( Qgs3dTilesLayer *layer )
{
  mLayerRef = QgsMapLayerRef( ( QgsMapLayer * )layer );
}

Qgs3dTilesLayer *Qgs3dTilesLayer3DRenderer::layer() const
{
  return qobject_cast<Qgs3dTilesLayer *>( mLayerRef.layer );
}

Qt3DCore::QEntity *Qgs3dTilesLayer3DRenderer::createEntity( const Qgs3DMapSettings &map ) const
{
  Qgs3dTilesLayer *meshLayer = layer();

  Tile *t = meshLayer->mTileset->mRoot.get();
  QgsCoordinateTransform coordTrans( t->mBv->mEpsg, map.crs(), map.transformContext() );

  Qgs3dTilesChunkLoaderFactory *facto = new Qgs3dTilesChunkLoaderFactory( map, coordTrans, meshLayer->mTileset->mRoot.get() );
  //  Qgs3dTilesChunkedEntity *entity = new Qgs3dTilesChunkedEntity( facto, t, true );
  QgsChunkedEntity *entity = new QgsChunkedEntity( t->mGeometricError, facto, false );
  entity->setUsingAdditiveStrategy( t->mRefine ==  Refinement::ADD );
  entity->setShowBoundingBoxes( true );

  return ( Qt3DCore::QEntity * )entity;
}
