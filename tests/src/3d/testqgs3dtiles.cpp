/***************************************************************************
  TestQgs3dTiles.cpp
  ----------------------
  Date                 : July 2021
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
    void testSphereReprojection();
    void testSphere2Reprojection();

private:
    void checkBoudingVolume( BoundingVolume *bv, bool doComplexTransformTest=true );
};

//runs before all tests
void TestQgs3dTiles::initTestCase()
{
}

//runs after all tests
void TestQgs3dTiles::cleanupTestCase()
{
}

void TestQgs3dTiles::testCubeReprojection()
{
    QJsonArray bvValue = { 0, 0, 0, 2.0, 0, 0, 0, 2.0, 0, 0, 0, 2.0 };
    Box bv( bvValue );
    checkBoudingVolume( &bv );
}

void TestQgs3dTiles::testSphereReprojection()
{
    QJsonArray bvValue = { 0, 0, 0, 2.0 };
    Sphere bv( bvValue );
    checkBoudingVolume( &bv );
}

void TestQgs3dTiles::testSphere2Reprojection()
{
    QJsonArray bvValue = { 4046916.0160611099563539,
                           214447.2275126340100542,
                           4908777.7954695904627442,
                           24174.2948580211013905 };
    Sphere bv( bvValue );
    checkBoudingVolume( &bv, false ); // do not comply well with double translation...
}

void TestQgs3dTiles::checkBoudingVolume( BoundingVolume *bv, bool doComplexTransformTest )
{
    QgsCoordinateTransform coordTrans( QgsCoordinateReferenceSystem::fromEpsgId( 4978 ),
                                       QgsCoordinateReferenceSystem::fromEpsgId( 3857 ),
                                       QgsProject::instance() );

    QgsMatrix4x4 tileTrans( 1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1, 0,
                            1000000, -5000000, 4000000, 1 );
    tileTrans = tileTrans.transposed();

    double d02, d05, d07;
    double dr02, dr05, dr07;
    double f02, f05, f07;
    double fr02, fr05, fr07;

    {
        qDebug() << "== without rotation, without epsg reprojection";
        Q3dCube c = bv->asCube( tileTrans );
        d02 = c.mPoints[0].distance( c.mPoints[2] );
        d07 = c.mPoints[0].distance( c.mPoints[7] );
        d05 = c.mPoints[0].distance( c.mPoints[5] );
        qDebug() << "d02:" << d02
                 << ", d07:" << d07
                 << ", d05:" << d05;
        QCOMPARE( ( long )( d02 * 1.0e8 ), ( long )( d07 * 1.0e8 ) ); // check cube diagonals
        QCOMPARE( ( long )( d02 * 1.0e8 ), ( long )( d05 * 1.0e8 ) ); // check cube diagonals
    }

    {
        qDebug() << "== with rotation, without epsg reprojection";
        tileTrans.rotate( 30.0, QgsVector3D( 1.0, 1.0, 1.0 ) );
        tileTrans.rotate( -10.0, QgsVector3D( 1.0, -1.0, 0.5 ) );

        Q3dCube c = bv->asCube( tileTrans );
        dr02 = c.mPoints[0].distance( c.mPoints[2] );
        dr07 = c.mPoints[0].distance( c.mPoints[7] );
        dr05 = c.mPoints[0].distance( c.mPoints[5] );
        qDebug() << "dr02:" << dr02
                 << ", dr07:" << dr07
                 << ", dr05:" << dr05;
        QCOMPARE( ( long )( dr02 * 1.0e8 ), ( long )( dr07 * 1.0e8 ) ); // check cube diagonals
        QCOMPARE( ( long )( dr02 * 1.0e8 ), ( long )( dr05 * 1.0e8 ) ); // check cube diagonals
    }

    // reset transform
    tileTrans = QgsMatrix4x4( 1, 0, 0, 0,
                              0, 1, 0, 0,
                              0, 0, 1, 0,
                              1000000, -5000000, 4000000, 1 );
    tileTrans = tileTrans.transposed();

    {
        qDebug() << "== without rotation, with epsg reprojection";
        Q3dCube c = bv->asCube( tileTrans, &coordTrans );
        f02 = d02;
        f07 = d07;
        f05 = d05;
        d02 = c.mPoints[0].distance( c.mPoints[2] );
        d07 = c.mPoints[0].distance( c.mPoints[7] );
        d05 = c.mPoints[0].distance( c.mPoints[5] );
        qDebug() << "d02:" << d02
                 << ", d07:" << d07
                 << ", d05:" << d05;
        // compute ratio with/without proj
        f02 /= d02;
        f07 /= d07;
        f05 /= d05;
        qDebug() << "f02:" << f02
                 << ", f07:" << f07
                 << ", f05:" << f05;
        QVERIFY ( abs ( 1.0 - d02 / d07) < 0.11 ); // proportion differences are almost the same (still a cube)
        QVERIFY ( abs ( 1.0 - d02 / d05) < 0.11 ); // proportion differences are almost the same (still a cube)
        QVERIFY ( abs ( 1.0 - d07 / d05) < 0.11 ); // proportion differences are almost the same (still a cube)
        QVERIFY( abs ( 1.0 - f02 ) < 0.21 ); // the proportions are almost preserved between reprojection
        QVERIFY( abs ( 1.0 - f07 ) < 0.21 ); // the proportions are almost preserved between reprojection
        QVERIFY( abs ( 1.0 - f05 ) < 0.21 ); // the proportions are almost preserved between reprojection
    }

    {
        qDebug() << "== with rotation, with epsg reprojection";
        tileTrans.rotate( 30.0, QgsVector3D( 1.0, 1.0, 1.0 ) );
        tileTrans.rotate( -10.0, QgsVector3D( 1.0, -1.0, 0.5 ) );

        Q3dCube c = bv->asCube( tileTrans, &coordTrans );
        fr02 = dr02;
        fr07 = dr07;
        fr05 = dr05;
        dr02 = c.mPoints[0].distance( c.mPoints[2] );
        dr07 = c.mPoints[0].distance( c.mPoints[7] );
        dr05 = c.mPoints[0].distance( c.mPoints[5] );
        qDebug() << "dr02:" << dr02
                 << ", dr07:" << dr07
                 << ", dr05:" << dr05;
        // compute ratio with/without proj
        fr02 /= dr02;
        fr07 /= dr07;
        fr05 /= dr05;
        qDebug() << "fr02:" << fr02
                 << ", fr07:" << fr07
                 << ", fr05:" << fr05;
        QVERIFY ( abs ( 1.0 - dr02 / dr07) < 0.11 ); // proportion differences are almost the same (still a cube)
        QVERIFY ( abs ( 1.0 - dr02 / dr05) < 0.11 ); // proportion differences are almost the same (still a cube)
        QVERIFY ( abs ( 1.0 - dr07 / dr05) < 0.11 ); // proportion differences are almost the same (still a cube)
        QVERIFY( abs ( 1.0 - fr02 ) < 0.21 ); // the proportions are almost preserved between reprojection
        QVERIFY( abs ( 1.0 - fr07 ) < 0.21 ); // the proportions are almost preserved between reprojection
        QVERIFY( abs ( 1.0 - fr05 ) < 0.21 ); // the proportions are almost preserved between reprojection
        QVERIFY( abs ( 1.0 - f02/fr02 ) < 0.11 ); // proportion differences are almost the same
        QVERIFY( abs ( 1.0 - f07/fr07 ) < 0.11 ); // proportion differences are almost the same
        QVERIFY( abs ( 1.0 - f05/fr05 ) < 0.11 ); // proportion differences are almost the same
    }


    if (doComplexTransformTest)
    {
        {
            qDebug() << "== with transform matrix, without epsg reprojection";
            tileTrans = QgsMatrix4x4 (97, 25, 0, 0,
                                      -16, 62.5, 76.5, 0,
                                      19, -74, 64.5, 0,
                                      1000000, -5000000, 4000000, 1);
            tileTrans = tileTrans.transposed();

            Q3dCube c = bv->asCube( tileTrans );
            d02 = c.mPoints[0].distance( c.mPoints[2] );
            d07 = c.mPoints[0].distance( c.mPoints[7] );
            d05 = c.mPoints[0].distance( c.mPoints[5] );
            qDebug() << "d02:" << d02
                     << ", d07:" << d07
                     << ", d05:" << d05;
            QVERIFY ( abs ( 1.0 - d02 / d07) < 0.11 ); // proportion differences are almost the same (still a cube)
            QVERIFY ( abs ( 1.0 - d02 / d05) < 0.11 ); // proportion differences are almost the same (still a cube)
            QVERIFY ( abs ( 1.0 - d07 / d05) < 0.11 ); // proportion differences are almost the same (still a cube)
        }

        {
            qDebug() << "== with transform matrix, with epsg reprojection";
            Q3dCube c = bv->asCube( tileTrans, &coordTrans );
            f02 = d02;
            f07 = d07;
            f05 = d05;
            d02 = c.mPoints[0].distance( c.mPoints[2] );
            d07 = c.mPoints[0].distance( c.mPoints[7] );
            d05 = c.mPoints[0].distance( c.mPoints[5] );
            qDebug() << "d02:" << d02
                     << ", d07:" << d07
                     << ", d05:" << d05;
            // compute ratio with/without proj
            f02 /= d02;
            f07 /= d07;
            f05 /= d05;
            qDebug() << "f02:" << f02
                     << ", f07:" << f07
                     << ", f05:" << f05;
            qDebug() << "d d02:" << abs ( 1.0 - d02 / d07)
                     << ", d d07:" << abs ( 1.0 - d02 / d05)
                     << ", d d05:" << abs ( 1.0 - d07 / d05);
            QVERIFY ( abs ( 1.0 - d02 / d07) < 0.11 ); // proportion differences are almost the same (still a cube)
            QVERIFY ( abs ( 1.0 - d02 / d05) < 0.11 ); // proportion differences are almost the same (still a cube)
            QVERIFY ( abs ( 1.0 - d07 / d05) < 0.11 ); // proportion differences are almost the same (still a cube)
            QVERIFY( abs ( 1.0 - f02 ) < 0.21 ); // the proportions are almost preserved between reprojection
            QVERIFY( abs ( 1.0 - f07 ) < 0.21 ); // the proportions are almost preserved between reprojection
            QVERIFY( abs ( 1.0 - f05 ) < 0.21 ); // the proportions are almost preserved between reprojection
        }
    }
}




QGSTEST_MAIN( TestQgs3dTiles )
#include "testqgs3dtiles.moc"
