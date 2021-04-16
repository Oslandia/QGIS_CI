#ifndef QGS3DTILESLAYER3DRENDERER_H
#define QGS3DTILESLAYER3DRENDERER_H

#include "qgsmeshlayer3drenderer.h"
#include "qgs3dmapsettings.h"
#include "qgs3dtileschunkloader.h"

class Qgs3dTilesLayer;

class _3D_EXPORT Qgs3dTilesLayer3DRenderer : public QgsMeshLayer3DRenderer
{
public:
    Qgs3dTilesLayer3DRenderer(Qgs3dTilesLayer *layer);

    virtual Qgs3dTilesLayer3DRenderer *clone() const override;
    virtual void setLayer( Qgs3dTilesLayer *layer );
    virtual Qgs3dTilesLayer *layer() const;

    virtual Qt3DCore::QEntity *createEntity( const Qgs3DMapSettings &map ) const override;

    virtual QString type() const override { return "3dtiles"; }


private:
  QgsMapLayerRef mLayerRef; //!< Layer used to extract mesh data from
};

#include "qgs3dtileslayer.h"

#endif // QGS3DTILESLAYER3DRENDERER_H
