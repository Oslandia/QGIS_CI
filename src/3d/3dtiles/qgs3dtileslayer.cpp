#include "qgs3dtileslayer.h"

Qgs3dTilesLayer::Qgs3dTilesLayer(Tileset * tileset) :
    QgsMeshLayer(), mTileset (tileset)
{

}

QgsMapLayerRenderer *Qgs3dTilesLayer::createMapRenderer( QgsRenderContext & )
{
  return createMapRenderer( );
}

QgsMapLayerRenderer *Qgs3dTilesLayer::createMapRenderer(  )
{
  return (QgsMapLayerRenderer *)new Qgs3dTilesLayer3DRenderer( this );
}

