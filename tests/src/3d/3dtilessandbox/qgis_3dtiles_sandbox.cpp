/***************************************************************************
                         qgis_3d_sandbox.cpp
                         --------------------
    begin                : October 2020
    copyright            : (C) 2020 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QApplication>
#include <QDesktopWidget>

#include "qgsapplication.h"
#include "qgs3d.h"
#include "qgslayertree.h"
#include "qgsmapsettings.h"
#include "qgspointcloudlayer.h"
#include "qgspointcloudlayer3drenderer.h"
#include "qgsproject.h"
#include "qgsflatterraingenerator.h"
#include "qgs3dmapscene.h"
#include "qgs3dmapsettings.h"
#include "qgs3dmapcanvas.h"
#include <iostream>
#include <fstream>
#include <streambuf>
#include "qgslinestring.h"
#include "qgsvectorlayer.h"
#include "qgsline3dsymbol.h"
#include "qgssimplelinematerialsettings.h"
#include "qgsvectorlayer3drenderer.h"

#include "3dtiles/qgs3dtileschunkloader.h"
#include "3dtiles/qgs3dtileslayer.h"
#include <QLoggingCategory>

void initCanvas3D( Qgs3DMapCanvas *canvas, Qgs3DMapSettings *mapSet, /*QList<QgsGeometry> & glist,*/
                   const QgsCoordinateTransform &coordTrans, Tileset *ts )
{
  QgsProject::instance()->setCrs( coordTrans.destinationCrs() );

  QgsLayerTree *root = QgsProject::instance()->layerTreeRoot();
  QList< QgsMapLayer * > visibleLayers = root->checkedLayers();

  /*
  QgsVectorLayer *layerLines = new QgsVectorLayer( "LineString?crs=EPSG:3857", "lines", "memory" );

  QgsLine3DSymbol *lineSymbol = new QgsLine3DSymbol;
  lineSymbol->setRenderAsSimpleLines( true );
  lineSymbol->setWidth( 5 );
  //RandomAmbientLineMaterialSettings mat;
  QgsSimpleLineMaterialSettings mat;
  mat.setAmbient( Qt::red );
  lineSymbol->setMaterial( mat.clone() );
  layerLines->setRenderer3D( new QgsVectorLayer3DRenderer( lineSymbol ) );

  QgsFeatureList flist;

  for (QgsGeometry & g : glist) {
    QgsFeature f1( layerLines->fields() );
    f1.setGeometry( g );
    flist << f1;
  }
  qDebug() << "geom size: " << flist.length() << "\n";

  layerLines->dataProvider()->addFeatures( flist );
  visibleLayers << layerLines;*/

  qDebug() << "============= CREATING Qgs3dTilesLayer \n";
  Qgs3dTilesLayer *threedTilesLayer = new Qgs3dTilesLayer( ts );
  threedTilesLayer->setRenderer3D( new Qgs3dTilesLayer3DRenderer( threedTilesLayer ) );
  visibleLayers << threedTilesLayer;
  qDebug() << "============= DONE Qgs3dTilesLayer \n";

  QgsMapSettings ms;
  ms.setDestinationCrs( QgsProject::instance()->crs() );
  ms.setLayers( visibleLayers );
  QgsRectangle fullExtent = ms.fullExtent();
  fullExtent.scale( 1.3 );

  mapSet->setCrs( coordTrans.destinationCrs() );
  mapSet->setOrigin( QgsVector3D( 0, 0, 0 ) );
  mapSet->setLayers( visibleLayers );
  mapSet->setTerrainLayers( visibleLayers );

  mapSet->setTransformContext( QgsProject::instance()->transformContext() );
  mapSet->setPathResolver( QgsProject::instance()->pathResolver() );
  mapSet->setMapThemeCollection( QgsProject::instance()->mapThemeCollection() );
  QObject::connect( QgsProject::instance(), &QgsProject::transformContextChanged, mapSet, [mapSet]
  {
    mapSet->setTransformContext( QgsProject::instance()->transformContext() );
  } );

  QgsFlatTerrainGenerator *flatTerrain = new QgsFlatTerrainGenerator;
  flatTerrain->setCrs( mapSet->crs() );
  flatTerrain->setExtent( fullExtent );
  mapSet->setTerrainGenerator( flatTerrain );

  QgsPointLightSettings defaultPointLight;
  defaultPointLight.setPosition( QgsVector3D( 4e6, 0.25e6, 10e6 ) );
  defaultPointLight.setConstantAttenuation( 0 );
  QList<QgsPointLightSettings> lights;
  lights << defaultPointLight;
  QgsPointLightSettings defaultPointLight2;
  defaultPointLight2.setPosition( QgsVector3D( 354958, 36.2705, 6.56284e6 ) );
  defaultPointLight2.setConstantAttenuation( 0 );
  lights << defaultPointLight2;
  QgsPointLightSettings defaultPointLight3;
  defaultPointLight3.setPosition( QgsVector3D( 355789, 67.3203, 6.56197e6 ) );
  defaultPointLight3.setConstantAttenuation( 0 );
  lights << defaultPointLight3;
  mapSet->setPointLights( lights );
  mapSet->setOutputDpi( QgsApplication::desktop()->logicalDpiX() );

  canvas->setMap( mapSet );

  QgsAABB ext = ts->mRoot->getBoundingVolumeAsAABB( &coordTrans );
  canvas->scene()->cameraController()->setViewFromTop( ext.xCenter(), ext.zCenter(), ext.yMax * 10.0, 0.0 );
  canvas->scene()->cameraController()->camera()->setNearPlane( 1 );
  canvas->scene()->cameraController()->camera()->setFarPlane( 100000.0f );
  canvas->scene()->cameraController()->camera()->setFieldOfView( 90.0f );


  QObject::connect( canvas->scene(), &Qgs3DMapScene::totalPendingJobsCountChanged, [canvas]
  {
    qDebug() << "pending jobs:" << canvas->scene()->totalPendingJobsCount();
  } );

  QObject::connect( canvas->scene()->cameraController(), &QgsCameraController::cameraChanged, [canvas]
  {
    qDebug() << "Camera pos:" << canvas->scene()->cameraController()->camera()->position();
  } );

}

/*
void createBboxes(Tile * ts, const QgsCoordinateTransform & coordTrans, QList<QgsGeometry> & glist) {
    QgsGeometry cube = ts->getBoundingVolumeAsGeometry(&coordTrans);
    qDebug() << "createBboxes Tile BBox: " << cube.asWkt();
    glist << cube;
    for (std::shared_ptr<Tile> t : ts->mChildren) {
        createBboxes(t.get(), coordTrans, glist);
    }
    if (ts->mContent.mSubContent->mType == ThreeDTilesContent::tileset && ts->mContent.mSubContent->mDepth < 1) {
        createBboxes(((Tileset*)ts->mContent.mSubContent.get())->mRoot.get(), coordTrans, glist);
    }
}
*/

int main( int argc, char *argv[] )
{
  QApplication app( argc, argv );

  QLoggingCategory::setFilterRules( "Qt3D.GLTFIO=true" );
  QLoggingCategory::setFilterRules( "Qt3D.AssimpImporter=true" );
  QLoggingCategory::setFilterRules( "Qt3D.GLTFImport=true" );
  //QLoggingCategory::setFilterRules("Qt3D.Renderer.*=true");
  QLoggingCategory::setFilterRules( "Qt3D.Renderer.SceneLoaders=true" );
  QLoggingCategory::setFilterRules( "Qt3D.Renderer.OpenGL.SceneLoaders=true" );
  QLoggingCategory::setFilterRules( "Qt3D.Renderer.RHI.SceneLoaders=true" );
  //QLoggingCategory::setFilterRules("Qt3D.*=true");

  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  Qgs3D::initialize();

  if ( argc < 3 )
  {
    qDebug() << "need json_tileset_file QGIS_project_file";
    return 1;
  }

  QString jsonFile( argv[1] );
  QString projectFile( argv[2] );

  Qgs3DMapSettings *map = new Qgs3DMapSettings;

  qDebug() << "Will read json: " << jsonFile << "\n";
  qDebug() << "Will read qgis: " << projectFile << "\n";
  std::unique_ptr<ThreeDTilesContent> ts = Tileset::fromUrl( QUrl( jsonFile ) );

  //qDebug() << "Dump: \n" << *ts.get()<< "\n";

  bool res = QgsProject::instance()->read( projectFile );
  if ( !res )
  {
    qDebug() << "can't open project file" << projectFile;
    return 1;
  }

  QgsCoordinateTransform coordTrans( QgsCoordinateReferenceSystem::fromEpsgId( 4978 ),
                                     QgsCoordinateReferenceSystem::fromEpsgId( 3857 ),
                                     QgsProject::instance() );
  qDebug() << "2154: " << QgsCoordinateReferenceSystem::fromEpsgId( 2154 ).toProj();
  qDebug() << "3857: " << QgsCoordinateReferenceSystem::fromEpsgId( 3857 ).toProj();
  qDebug() << "4326: " << QgsCoordinateReferenceSystem::fromEpsgId( 4326 ).toProj();
  qDebug() << "4979: " << QgsCoordinateReferenceSystem::fromEpsgId( 4979 ).toProj();
  qDebug() << "4978: " << QgsCoordinateReferenceSystem::fromEpsgId( 4978 ).toProj();

  /*
  QList<QgsGeometry> glist;
  createBboxes(((Tileset*)ts.get())->mRoot.get(), coordTrans, glist);*/

  qDebug() << "============= STARTING initCanvas3D \n";
  Qgs3DMapCanvas *canvas = new Qgs3DMapCanvas;
  initCanvas3D( canvas, map, /*glist, */coordTrans, ( Tileset * )ts.get() );
  qDebug() << "============= DONE initCanvas3D \n";
  canvas->resize( 1200, 1000 );
  canvas->show();

  return app.exec();
}
