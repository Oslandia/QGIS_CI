#ifndef QGS3DTILESLAYER_H
#define QGS3DTILESLAYER_H

#include "qgsmeshlayer.h"
#include "qgs3dtileslayer3drenderer.h"
#include "3dtiles.h"

class _3D_EXPORT Qgs3dTilesLayer : public QgsMeshLayer
{
    Q_OBJECT
public:
    Tileset * mTileset;

public:
    Qgs3dTilesLayer(Tileset * tileset);

    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsMapLayerRenderer *createMapRenderer( ) ;

};

#endif // QGS3DTILESLAYER_H
