/***************************************************************************
  testqgs4978rendering.cpp
  --------------------------------------
  Date                 : May 2022
  Copyright            : (C) 2022 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgstest.h"
#include "qgsmultirenderchecker.h"

#include "qgslinestring.h"
#include "qgsproject.h"
#include "qgsvectorlayer.h"
#include "qgsapplication.h"
#include "qgs3d.h"

#include "qgs3dmapscene.h"
#include "qgs3dmapsettings.h"
#include "qgs3dutils.h"
#include "qgscameracontroller.h"
#include "qgsflatterraingenerator.h"
#include "qgsline3dsymbol.h"
#include "qgsoffscreen3dengine.h"
#include "qgsrulebased3drenderer.h"
#include "qgsterrainentity_p.h"
#include "qgsvectorlayer3drenderer.h"
#include "qgs25drenderer.h"
#include "qgssimplelinematerialsettings.h"
#include "qgslinesymbol.h"
#include "qgssinglesymbolrenderer.h"

#include <QFileInfo>
#include <QDir>
#include <QDesktopWidget>

class TestQgs4978Rendering : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void test3dLineRendering();
    void test2dLineRendering();

  private:
    bool render3dCheck( const QString &testName, QImage &image, int mismatchCount = 0 );
    bool render2dCheck( const QString &testName, QgsVectorLayer *layer, QgsRectangle extent, QgsCoordinateReferenceSystem *crs = nullptr );

    QString mReport;

    std::unique_ptr<QgsProject> mProject;

};

//runs before all tests
void TestQgs4978Rendering::initTestCase()
{
  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  Qgs3D::initialize();

  mReport = QStringLiteral( "<h1>3D Rendering Tests</h1>\n" );

  mProject.reset( new QgsProject );
  QgsCoordinateReferenceSystem newCrs;
  newCrs.createFromString( "EPSG:4978" );
  mProject->setCrs( newCrs );
}

//runs after all tests
void TestQgs4978Rendering::cleanupTestCase()
{
  mProject.reset();

  const QString myReportFile = QDir::tempPath() + "/qgistest.html";
  QFile myFile( myReportFile );
  if ( myFile.open( QIODevice::WriteOnly | QIODevice::Append ) )
  {
    QTextStream myQTextStream( &myFile );
    myQTextStream << mReport;
    myFile.close();
  }

  QgsApplication::exitQgis();
}


bool TestQgs4978Rendering::render3dCheck( const QString &testName, QImage &image, int mismatchCount )
{
  mReport += "<h2>" + testName + "</h2>\n";
  const QString myTmpDir = QDir::tempPath() + '/';
  const QString myFileName = myTmpDir + testName + ".png";
  image.save( myFileName, "PNG" );
  QgsMultiRenderChecker myChecker;
  myChecker.setControlPathPrefix( QStringLiteral( "3d" ) );
  myChecker.setControlName( "expected_" + testName );
  myChecker.setRenderedImage( myFileName );
  myChecker.setColorTolerance( 2 );  // color tolerance < 2 was failing polygon3d_extrusion test
  const bool myResultFlag = myChecker.runTest( testName, mismatchCount );
  mReport += myChecker.report();
  return myResultFlag;
}

bool TestQgs4978Rendering::render2dCheck( const QString &testName, QgsVectorLayer *layer, QgsRectangle extent, QgsCoordinateReferenceSystem *crs )
{
  mReport += "<h2>" + testName + "</h2>\n";

  const QString myTmpDir = QDir::tempPath() + '/';
  const QString myFileName = myTmpDir + testName + ".png";

  auto mMapSettings = new QgsMapSettings();
  mMapSettings->setLayers( QList<QgsMapLayer *>() << layer );
  mMapSettings->setExtent( extent );
  mMapSettings->setOutputDpi( 96 );
  if ( crs == nullptr )
  {
    QgsCoordinateReferenceSystem newCrs;
    newCrs.createFromString( "EPSG:3857" );
    mMapSettings->setDestinationCrs( newCrs );
  }
  else
  {
    mMapSettings->setDestinationCrs( *crs );
  }

  QgsRenderChecker myChecker;
  myChecker.setControlPathPrefix( QStringLiteral( "3d" ) );
  myChecker.setControlName( "expected_" + testName );
  myChecker.setMapSettings( *mMapSettings );
  myChecker.setRenderedImage( myFileName );
  myChecker.setColorTolerance( 15 );
  const bool myResultFlag = myChecker.runTest( testName, 0 );
  mReport += myChecker.report();
  return myResultFlag;
}

void TestQgs4978Rendering::test2dLineRendering()
{
  QgsVectorLayer *layerLines = new QgsVectorLayer( QString( TEST_DATA_DIR ) + "/3d/earth_size_sphere_4978.gpkg", "lines", "ogr" );

  QgsLineSymbol *fillSymbol = new QgsLineSymbol();
  fillSymbol->setWidth( 0.3 );
  fillSymbol->setColor( QColor( 255, 0, 0 ) );
  layerLines->setRenderer( new QgsSingleSymbolRenderer( fillSymbol ) );

  QgsCoordinateReferenceSystem newCrs;
  newCrs.createFromString( "EPSG:3857" );
  QVERIFY( render2dCheck( "4978_2D_line_rendering", layerLines, QgsRectangle( -2.5e7, -2.5e7, 2.5e7, 2.5e7 ), &newCrs ) );

  delete layerLines;
}

void TestQgs4978Rendering::test3dLineRendering()
{
  const QgsRectangle fullExtent( 0, 0, 1000, 1000 );

  QgsVectorLayer *layerLines = new QgsVectorLayer( QString( TEST_DATA_DIR ) + "/3d/earth_size_sphere_4978.gpkg", "lines", "ogr" );

  QgsLine3DSymbol *lineSymbol = new QgsLine3DSymbol;
  lineSymbol->setRenderAsSimpleLines( true );
  lineSymbol->setWidth( 1 );
  QgsSimpleLineMaterialSettings mat;
  mat.setAmbient( Qt::red );
  lineSymbol->setMaterial( mat.clone() );
  layerLines->setRenderer3D( new QgsVectorLayer3DRenderer( lineSymbol ) );

  Qgs3DMapSettings *map = new Qgs3DMapSettings;
  map->setCrs( mProject->crs() );
  map->setOrigin( QgsVector3D( fullExtent.center().x(), fullExtent.center().y(), 0 ) );
  map->setLayers( QList<QgsMapLayer *>() << layerLines );

  QgsFlatTerrainGenerator *flatTerrain = new QgsFlatTerrainGenerator;
  flatTerrain->setCrs( map->crs() );
  flatTerrain->setExtent( fullExtent );
  map->setTerrainGenerator( flatTerrain );

  QgsOffscreen3DEngine engine;
  Qgs3DMapScene *scene = new Qgs3DMapScene( *map, &engine );
  engine.setRootEntity( scene );

  // look from the top
  scene->cameraController()->setLookingAtPoint( QgsVector3D( 0, 0, 0 ), 1.5e7, 0, 0 );

  // When running the test on Travis, it would initially return empty rendered image.
  // Capturing the initial image and throwing it away fixes that. Hopefully we will
  // find a better fix in the future.
  Qgs3DUtils::captureSceneImage( engine, scene );

  QImage img = Qgs3DUtils::captureSceneImage( engine, scene );
  QVERIFY( render3dCheck( "4978_line_rendering_1", img, 40 ) );

  // more perspective look
  scene->cameraController()->setLookingAtPoint( QgsVector3D( 0, 0, 0 ), 1.5e7, 45, 45 );

  QImage img2 = Qgs3DUtils::captureSceneImage( engine, scene );
  QVERIFY( render3dCheck( "4978_line_rendering_2", img2, 40 ) );

  delete layerLines;
}

QGSTEST_MAIN( TestQgs4978Rendering )
#include "testqgs4978rendering.moc"
