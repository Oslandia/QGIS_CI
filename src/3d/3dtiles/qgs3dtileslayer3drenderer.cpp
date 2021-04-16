#include "qgs3dtileslayer3drenderer.h"

Qgs3dTilesLayer3DRenderer::Qgs3dTilesLayer3DRenderer(Qgs3dTilesLayer *layer):
    QgsMeshLayer3DRenderer()
{
    setLayer(layer);
}


Qgs3dTilesLayer3DRenderer *Qgs3dTilesLayer3DRenderer::clone() const
{
  Qgs3dTilesLayer3DRenderer *r = new Qgs3dTilesLayer3DRenderer( qobject_cast<Qgs3dTilesLayer *>( mLayerRef.layer ) );
  return r;
}

void Qgs3dTilesLayer3DRenderer::setLayer( Qgs3dTilesLayer *layer )
{
  mLayerRef = QgsMapLayerRef( (QgsMapLayer * )layer );
}

Qgs3dTilesLayer *Qgs3dTilesLayer3DRenderer::layer() const
{
  return qobject_cast<Qgs3dTilesLayer *>( mLayerRef.layer );
}

Qt3DCore::QEntity *Qgs3dTilesLayer3DRenderer::createEntity( const Qgs3DMapSettings &map ) const
{
  Qgs3dTilesLayer *meshLayer = layer();

  Tile * t = meshLayer->mTileset->mRoot.get();
  QgsCoordinateTransform coordTrans( t->mBv->mEpsg, map.crs(), map.transformContext() );

  Qgs3dTilesChunkLoaderFactory * facto = new Qgs3dTilesChunkLoaderFactory(map, coordTrans, meshLayer->mTileset->mRoot.get());
  Qgs3dTilesChunkedEntity *entity = new Qgs3dTilesChunkedEntity( NULL, t, facto, true );
//  entity->setGeometryError(10000.0);

  return (Qt3DCore::QEntity *)entity;
}
