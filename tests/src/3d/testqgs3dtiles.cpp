/***************************************************************************
  TestQgs3dTiles.cpp
  ----------------------
  Date                 : July 2XY1
  Copyright            : (C) 2XY1 by Benoit De Mezzo
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

#include "3dtiles/3dtiles.h"
#include "qgsproject.h"

/**
 * \ingroup UnitTests
 * This is a unit test for the vertex tool
 */
class TestQgs3dTiles : public QObject
{
    Q_OBJECT
  public:
    TestQgs3dTiles() = default;

  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.

    void testCubeReprojection();
    void testCube2Reprojection();
    void testSphereReprojection();
    void testSphere2Reprojection();

  private:
    void checkBoudingVolume( BoundingVolume *bv, QgsVector3D translation );
};

//runs before all tests
void TestQgs3dTiles::initTestCase()
{
  double radius = 6378200;
  unsigned int rings = 64;
  unsigned int sectors = 64;

  double const R = 1. / ( double )( rings - 1 );
  double const S = 1. / ( double )( sectors - 1 );
  QVector<QVector<QgsVector3D>> ringList;

  // Establish texture coordinates, vertex list, and normals
  for ( unsigned int r = 1; r < rings; r++ )
  {
    QVector<QgsVector3D> ringPoint;
    for ( unsigned int s = 0; s < sectors; s++ )
    {
      double const y = sin( -M_PI_2 + M_PI * r * R );
      double const x = cos( 2 * M_PI * s * S ) * sin( M_PI * r * R );
      double const z = sin( 2 * M_PI * s * S ) * sin( M_PI * r * R );
      ringPoint.append( QgsVector3D( y * radius, x * radius, z * radius ) );
    }
    ringList.append( ringPoint );
  }

  for ( unsigned int s = 0; s < sectors; s++ )
  {
    QVector<QgsVector3D> ringPoint;
    for ( unsigned int r = 0; r < rings - 1; r++ )
    {
      ringPoint.append( ringList[r][s] );
    }
    ringList.append( ringPoint );
  }

  for ( int r = 0; r < ringList.length(); r++ )
  {
    printf( "--- Ring %d\n", r );
    printf( "insert into test_3d (id, name, geom) values (%d, 'ring %d', ST_SetSRID(ST_MakeLine(ARRAY[\n", ( 100 + r ), r );
    for ( int p = 0; p < ringList[r].length(); p++ )
    {
      printf( "ST_MakePoint( %lf, %lf, %lf)", ringList[r][p].x(), ringList[r][p].y(), ringList[r][p].z() );
      if ( p < ringList[r].length() - 1 )
      {
        printf( ", " );
      }
    }
    printf( " ]), 4978));\n" );
  }
}

//runs after all tests
void TestQgs3dTiles::cleanupTestCase()
{
}

void TestQgs3dTiles::testCubeReprojection()
{
  QJsonArray bvValue = { 0, 0, 0, 2.0, 0, 0, 0, 2.0, 0, 0, 0, 2.0 }; // cartesian coordinates
  Box bv( bvValue );
  qDebug() << "===== cube near philadelphie";
  checkBoudingVolume( &bv, QgsVector3D( 1215107, -4736648, 4081966 ) ); // CRS 4978 ~ near philadelphie
}

void TestQgs3dTiles::testCube2Reprojection()
{
  QJsonArray bvValue = { 0, 0, 0, 2.0, 0, 0, 0, 2.0, 0, 0, 0, 2.0 }; // cartesian coordinates
  Box bv( bvValue );
  qDebug() << "===== cube near lille";
  checkBoudingVolume( &bv, QgsVector3D( 4046916, 214447, 4908778 ) ); // CRS 4978 ~ near lille
}

void TestQgs3dTiles::testSphereReprojection()
{
  QJsonArray bvValue = { 0, 0, 0, 2.0 }; // cartesian coordinates
  Sphere bv( bvValue );
  qDebug() << "===== sphere near philadelphie";
  checkBoudingVolume( &bv, QgsVector3D( 1215107, -4736648, 4081966 ) ); // CRS 4978 ~ near philadelphie
}

void TestQgs3dTiles::testSphere2Reprojection()
{
  QJsonArray bvValue = { 4046916, 214447, 4908778,
                         24174.3
                       }; // CRS 4978 ~ near lille ==> not transform/translation
  Sphere bv( bvValue );
  qDebug() << "===== sphere near lille";
  checkBoudingVolume( &bv, QgsVector3D( 0, 0, 0 ) ); // do not comply well with double translation...
}

void TestQgs3dTiles::checkBoudingVolume( BoundingVolume *bv, QgsVector3D translation )
{
  QgsCoordinateTransform coordTrans( QgsCoordinateReferenceSystem::fromEpsgId( 4978 ),
                                     QgsCoordinateReferenceSystem::fromEpsgId( 3857 ),
                                     QgsProject::instance() );
  // column-major order
  QgsMatrix4x4 tileTrans( 1, 0, 0, 0,
                          0, 1, 0, 0,
                          0, 0, 1, 0,
                          translation.x(), translation.y(), translation.z(), 1 );
  // to row-major order
  tileTrans = tileTrans.transposed();

  double dXY, dXZ, dYZ;
  double drXY, drXZ, drYZ;
  double fXY, fXZ, fYZ;
  double frXY, frXZ, frYZ;

  {
    qDebug() << "== without rotation, without epsg reprojection";
    Q3dCube c = bv->asCube( tileTrans );
    dXY = c.mPoints[0].distance( c.mPoints[2] );
    dYZ = c.mPoints[0].distance( c.mPoints[7] );
    dXZ = c.mPoints[0].distance( c.mPoints[5] );
    qDebug() << "dXY:" << dXY
             << ", dYZ:" << dYZ
             << ", dXZ:" << dXZ;
    QCOMPARE( ( long )( dXY * 1.0e8 ), ( long )( dYZ * 1.0e8 ) ); // check cube diagonals
    QCOMPARE( ( long )( dXY * 1.0e8 ), ( long )( dXZ * 1.0e8 ) ); // check cube diagonals
  }

  {
    qDebug() << "== with rotation, without epsg reprojection";
    tileTrans.rotate( 30.0, QgsVector3D( 1.0, 1.0, 1.0 ) );
    tileTrans.rotate( -10.0, QgsVector3D( 1.0, -1.0, 0.5 ) );

    Q3dCube c = bv->asCube( tileTrans );
    drXY = c.mPoints[0].distance( c.mPoints[2] );
    drYZ = c.mPoints[0].distance( c.mPoints[7] );
    drXZ = c.mPoints[0].distance( c.mPoints[5] );
    for ( QgsVector4D p : c.mPoints )
    {
      printf( "ST_MakePoint( %lf, %lf, %lf),\n", p.x(), p.y(), p.z() );
    }
    qDebug() << "drXY:" << drXY
             << ", drYZ:" << drYZ
             << ", drXZ:" << drXZ;
    QCOMPARE( ( long )( drXY * 1.0e8 ), ( long )( drYZ * 1.0e8 ) ); // check cube diagonals
    QCOMPARE( ( long )( drXY * 1.0e8 ), ( long )( drXZ * 1.0e8 ) ); // check cube diagonals
  }

  // reset transform
  tileTrans = QgsMatrix4x4( 1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1, 0,
                            translation.x(), translation.y(), translation.z(), 1 );
  tileTrans = tileTrans.transposed();

  {
    qDebug() << "== without rotation, with epsg reprojection";
    Q3dCube c = bv->asCube( tileTrans, &coordTrans );
    fXY = dXY;
    fYZ = dYZ;
    fXZ = dXZ;
    dXY = c.mPoints[0].distance( c.mPoints[2] );
    dYZ = c.mPoints[0].distance( c.mPoints[7] );
    dXZ = c.mPoints[0].distance( c.mPoints[5] );
    qDebug() << "dXY:" << dXY
             << ", dYZ:" << dYZ
             << ", dXZ:" << dXZ;
    // compute ratio with/without proj
    fXY /= dXY;
    fYZ /= dYZ;
    fXZ /= dXZ;
    qDebug() << "fXY:" << fXY
             << ", fYZ:" << fYZ
             << ", fXZ:" << fXZ;
    qDebug() << "d dXY:" << abs( 1.0 - dXY / dYZ )
             << ", d dYZ:" << abs( 1.0 - dXY / dXZ )
             << ", d dXZ:" << abs( 1.0 - dYZ / dXZ );
    QVERIFY( abs( 1.0 - dXY / dYZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - dXY / dXZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - dYZ / dXZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - fXY ) < 0.35 );  // the proportions are almost preserved between reprojection
    QVERIFY( abs( 1.0 - fYZ ) < 0.35 );  // the proportions are almost preserved between reprojection
    QVERIFY( abs( 1.0 - fXZ ) < 0.35 );  // the proportions are almost preserved between reprojection
  }

  {
    qDebug() << "== with rotation, with epsg reprojection";
    tileTrans.rotate( 30.0, QgsVector3D( 1.0, 1.0, 1.0 ) );
    tileTrans.rotate( -10.0, QgsVector3D( 1.0, -1.0, 0.5 ) );

    Q3dCube c = bv->asCube( tileTrans, &coordTrans );
    frXY = drXY;
    frYZ = drYZ;
    frXZ = drXZ;
    drXY = c.mPoints[0].distance( c.mPoints[2] );
    drYZ = c.mPoints[0].distance( c.mPoints[7] );
    drXZ = c.mPoints[0].distance( c.mPoints[5] );
    qDebug() << "drXY:" << drXY
             << ", drYZ:" << drYZ
             << ", drXZ:" << drXZ;
    // compute ratio with/without proj
    frXY /= drXY;
    frYZ /= drYZ;
    frXZ /= drXZ;
    qDebug() << "frXY:" << frXY
             << ", frYZ:" << frYZ
             << ", frXZ:" << frXZ;
    qDebug() << "d dXY:" << abs( 1.0 - dXY / dYZ )
             << ", d dYZ:" << abs( 1.0 - dXY / dXZ )
             << ", d dXZ:" << abs( 1.0 - dYZ / dXZ );
    /*QVERIFY( abs( 1.0 - drXY / drYZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - drXY / drXZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - drYZ / drXZ ) < 0.15 );  // proportion differences are almost the same (still a cube)*/
    QVERIFY( abs( 1.0 - frXY ) < 0.36 );  // the proportions are almost preserved between reprojection
    QVERIFY( abs( 1.0 - frYZ ) < 0.35 );  // the proportions are almost preserved between reprojection
    QVERIFY( abs( 1.0 - frXZ ) < 0.35 );  // the proportions are almost preserved between reprojection
    QVERIFY( abs( 1.0 - fXY / frXY ) < 0.15 ); // proportion differences are almost the same
    QVERIFY( abs( 1.0 - fYZ / frYZ ) < 0.15 ); // proportion differences are almost the same
    QVERIFY( abs( 1.0 - fXZ / frXZ ) < 0.15 ); // proportion differences are almost the same
  }


  {
    qDebug() << "== with transform matrix, without epsg reprojection";
    tileTrans = QgsMatrix4x4( 97, 25, 0, 0,
                              -16, 62.5, 76.5, 0,
                              19, -74, 64.5, 0,
                              translation.x(), translation.y(), translation.z(), 1 );
    tileTrans = tileTrans.transposed();

    Q3dCube c = bv->asCube( tileTrans );
    dXY = c.mPoints[0].distance( c.mPoints[2] );
    dYZ = c.mPoints[0].distance( c.mPoints[7] );
    dXZ = c.mPoints[0].distance( c.mPoints[5] );
    qDebug() << "dXY:" << dXY
             << ", dYZ:" << dYZ
             << ", dXZ:" << dXZ;
    QVERIFY( abs( 1.0 - dXY / dYZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - dXY / dXZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - dYZ / dXZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
  }

  {
    qDebug() << "== with transform matrix, with epsg reprojection";
    Q3dCube c = bv->asCube( tileTrans, &coordTrans );
    fXY = dXY;
    fYZ = dYZ;
    fXZ = dXZ;
    dXY = c.mPoints[0].distance( c.mPoints[2] );
    dYZ = c.mPoints[0].distance( c.mPoints[7] );
    dXZ = c.mPoints[0].distance( c.mPoints[5] );
    qDebug() << "dXY:" << dXY
             << ", dYZ:" << dYZ
             << ", dXZ:" << dXZ;
    // compute ratio with/without proj
    fXY /= dXY;
    fYZ /= dYZ;
    fXZ /= dXZ;
    qDebug() << "fXY:" << fXY
             << ", fYZ:" << fYZ
             << ", fXZ:" << fXZ;
    qDebug() << "d dXY:" << abs( 1.0 - dXY / dYZ )
             << ", d dYZ:" << abs( 1.0 - dXY / dXZ )
             << ", d dXZ:" << abs( 1.0 - dYZ / dXZ );
    QVERIFY( abs( 1.0 - dXY / dYZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - dXY / dXZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - dYZ / dXZ ) < 0.15 );  // proportion differences are almost the same (still a cube)
    QVERIFY( abs( 1.0 - fXY ) < 0.25 );  // the proportions are almost preserved between reprojection
    QVERIFY( abs( 1.0 - fYZ ) < 0.25 );  // the proportions are almost preserved between reprojection
    QVERIFY( abs( 1.0 - fXZ ) < 0.25 );  // the proportions are almost preserved between reprojection
  }
}




QGSTEST_MAIN( TestQgs3dTiles )
#include "testqgs3dtiles.moc"
