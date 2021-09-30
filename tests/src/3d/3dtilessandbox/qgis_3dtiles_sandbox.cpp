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
#include <QCommandLineParser>

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

  QgsAABB ext = ts->mRootTile->getBoundingVolumeAsAABB( &coordTrans );

  QList<QgsPointLightSettings> lights;

  QgsPointLightSettings defaultPointLight;
  {
    defaultPointLight.setPosition( QgsVector3D( ext.xMin, ext.yMax * 2.0, ext.zMin ) );
    defaultPointLight.setConstantAttenuation( 0 );
    defaultPointLight.setLinearAttenuation( 0.0f );
    defaultPointLight.setQuadraticAttenuation( 0.0f );
    lights << defaultPointLight;
  }
  {
    defaultPointLight.setPosition( QgsVector3D( ext.xMax, ext.yMax * 2.0, ext.zMin ) );
    defaultPointLight.setConstantAttenuation( 0 );
    defaultPointLight.setLinearAttenuation( 0.0f );
    defaultPointLight.setQuadraticAttenuation( 0.0f );
    lights << defaultPointLight;
  }
  {
    defaultPointLight.setPosition( QgsVector3D( ext.xMin, ext.yMax * 2.0, ext.zMax ) );
    defaultPointLight.setConstantAttenuation( 0 );
    defaultPointLight.setLinearAttenuation( 0.0f );
    defaultPointLight.setQuadraticAttenuation( 0.0f );
    lights << defaultPointLight;
  }
  {
    defaultPointLight.setPosition( QgsVector3D( ext.xMax, ext.yMax * 2.0, ext.zMax ) );
    defaultPointLight.setConstantAttenuation( 0 );
    defaultPointLight.setLinearAttenuation( 0.0f );
    defaultPointLight.setQuadraticAttenuation( 0.0f );
    lights << defaultPointLight;
  }

  mapSet->setPointLights( lights );
  mapSet->setOutputDpi( QgsApplication::desktop()->logicalDpiX() );

  canvas->setMap( mapSet );

  canvas->scene()->cameraController()->setViewFromTop( ext.xCenter(), ext.zCenter(), ext.yMax * 10.0, 0.0 );
  canvas->scene()->cameraController()->camera()->setNearPlane( 1 );
  canvas->scene()->cameraController()->camera()->setFarPlane( 100000.0f );
  canvas->scene()->cameraController()->camera()->setFieldOfView( 90.0f );


  QObject::connect( canvas->scene(), &Qgs3DMapScene::totalPendingJobsCountChanged, [canvas]
  {
    qDebug() << "pending jobs:" << canvas->scene()->totalPendingJobsCount();
  } );

  /* QObject::connect( canvas->scene()->cameraController(), &QgsCameraController::cameraChanged, [canvas]
   {
     qDebug() << "Camera pos:" << canvas->scene()->cameraController()->camera()->position();
   } );*/

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

  // ================= manage option
  QCoreApplication::setApplicationName( "3dTiles sandbox" );
  QCoreApplication::setApplicationVersion( "1.0" );
  QCommandLineParser parser;
  parser.setApplicationDescription( "3dTiles sandbox" );
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption nameOption( QStringList() << "n" << "name",
                                 QStringLiteral( "3d Tiles tileset name (for cache)." ),
                                 QStringLiteral( "name" ) );
  parser.addOption( nameOption );

  QCommandLineOption tilesetOption( QStringList() << "j" << "json",
                                    QStringLiteral( "3d Tiles json tileset local path or remote url." ),
                                    QStringLiteral( "path" ) );
  parser.addOption( tilesetOption );

  QCommandLineOption qgsOption( QStringList() << "q" << "qgs",
                                QStringLiteral( "Qgis projet file to use." ),
                                QStringLiteral( "path" ) );
  parser.addOption( qgsOption );

  QCommandLineOption origGeomOption( QStringList() << "g" << "origGeom",
                                     QStringLiteral( "Use default geometric error. Default: false." ) );
  parser.addOption( origGeomOption );

  QCommandLineOption flipYOption( QStringList() << "y" << "yUp",
                                  QStringLiteral( "Flip to Y up. Default: false." ) );
  parser.addOption( flipYOption );

  QCommandLineOption fixTransOption( QStringList() << "f" << "fixTrans",
                                     QStringLiteral( "Try to fix translation. Default: false." ) );
  parser.addOption( fixTransOption );

  QCommandLineOption useMatOption( QStringList() << "m" << "fakeMat",
                                   QStringLiteral( "Use fake material. Default: false." ) );
  parser.addOption( useMatOption );

  // Process the actual command line arguments given by the user
  parser.process( app );

  QString jsonFile;
  QString name;
  QString projectFile;
  bool doFlipY = false;
  bool doFixTrans = false;
  bool doUseMat = false;
  bool doOrigGeom = false;

  if ( parser.isSet( nameOption ) )
    name = parser.value( nameOption );
  else
    parser.showHelp( 1 );

  if ( parser.isSet( tilesetOption ) )
    jsonFile = parser.value( tilesetOption );
  else
    parser.showHelp( 1 );

  if ( parser.isSet( qgsOption ) )
    projectFile = parser.value( qgsOption );
  else
    parser.showHelp( 1 );

  if ( parser.isSet( flipYOption ) )
    doFlipY = true;

  if ( parser.isSet( fixTransOption ) )
    doFixTrans = true;

  if ( parser.isSet( useMatOption ) )
    doUseMat = true;

  if ( parser.isSet( origGeomOption ) )
    doOrigGeom = true;

  Qgs3DMapSettings *map = new Qgs3DMapSettings;

  qDebug() << "Will read json:" << jsonFile;
  qDebug() << "Will read qgis:" << projectFile;
  qDebug() << "Will use doFlipY:" << doFlipY << "/ doFixTrans:" << doFixTrans << "/ doUseMat:" << doUseMat;

  std::unique_ptr<ThreeDTilesContent> ts = Tileset::fromUrl( QUrl( jsonFile ) );

  ( ( Tileset * )ts.get() )->setName( name );
  ( ( Tileset * )ts.get() )->setFlipToYUp( doFlipY );
  ( ( Tileset * )ts.get() )->setCorrectTranslation( doFixTrans );
  ( ( Tileset * )ts.get() )->setUseFakeMaterial( doUseMat );
  ( ( Tileset * )ts.get() )->setUseOriginalGeomError( doOrigGeom );

  bool res = QgsProject::instance()->read( projectFile );
  if ( !res )
  {
    qDebug() << "can't open project file" << projectFile;
    return 1;
  }

  QgsCoordinateTransform coordTrans( QgsCoordinateReferenceSystem::fromEpsgId( 4978 ),
                                     QgsProject::instance()->crs(),
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
