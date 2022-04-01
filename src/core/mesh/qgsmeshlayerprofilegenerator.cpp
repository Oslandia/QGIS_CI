/***************************************************************************
                         qgsmeshlayerprofilegenerator.cpp
                         ---------------
    begin                : March 2022
    copyright            : (C) 2022 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgsmeshlayerprofilegenerator.h"
#include "qgsprofilerequest.h"
#include "qgscurve.h"
#include "qgsmeshlayer.h"
#include "qgscoordinatetransform.h"
#include "qgsgeos.h"
#include "qgsterrainprovider.h"
#include "qgsmeshlayerutils.h"

//
// QgsMeshLayerProfileGenerator
//

QString QgsMeshLayerProfileResults::type() const
{
  return QStringLiteral( "mesh" );
}

QMap<double, double> QgsMeshLayerProfileResults::distanceToHeightMap() const
{
  QMap<double, double> res;
  for ( const Result &r : results )
  {
    res.insert( r.distance, r.height );
  }
  return res;
}

QgsPointSequence QgsMeshLayerProfileResults::sampledPoints() const
{
  return rawPoints;
}

QVector<QgsGeometry> QgsMeshLayerProfileResults::asGeometries() const
{
  QVector<QgsGeometry> res;
  res.reserve( rawPoints.size() );
  for ( const QgsPoint &point : rawPoints )
    res.append( QgsGeometry( point.clone() ) );

  return res;
}

void QgsMeshLayerProfileResults::renderResults( QgsProfileRenderContext &context )
{

}

//
// QgsMeshLayerProfileGenerator
//

QgsMeshLayerProfileGenerator::QgsMeshLayerProfileGenerator( QgsMeshLayer *layer, const QgsProfileRequest &request )
  : mProfileCurve( request.profileCurve() ? request.profileCurve()->clone() : nullptr )
  , mSourceCrs( layer->crs() )
  , mTargetCrs( request.crs() )
  , mTransformContext( request.transformContext() )
  , mStepDistance( request.stepDistance() )
{
  layer->updateTriangularMesh();
  mTriangularMesh = *layer->triangularMesh();
}

QgsMeshLayerProfileGenerator::~QgsMeshLayerProfileGenerator() = default;

bool QgsMeshLayerProfileGenerator::generateProfile()
{
  if ( !mProfileCurve )
    return false;

  // we need to transform the profile curve to the mesh's CRS
  QgsGeometry transformedCurve( mProfileCurve->clone() );
  mLayerToTargetTransform = QgsCoordinateTransform( mSourceCrs, mTargetCrs, mTransformContext );

  try
  {
    transformedCurve.transform( mLayerToTargetTransform, Qgis::TransformDirection::Reverse );
  }
  catch ( QgsCsException & )
  {
    QgsDebugMsg( QStringLiteral( "Error transforming profile line to mesh CRS" ) );
    return false;
  }

  mResults = std::make_unique< QgsMeshLayerProfileResults >();

  // we don't currently have any method to determine line->mesh intersection points, so for now we just sample at about 100(?) points over the line
  const double curveLength = transformedCurve.length();

  if ( !std::isnan( mStepDistance ) )
    transformedCurve = transformedCurve.densifyByDistance( mStepDistance );
  else
    transformedCurve = transformedCurve.densifyByDistance( curveLength / 100 );

  for ( auto it = transformedCurve.vertices_begin(); it != transformedCurve.vertices_end(); ++it )
  {
    QgsPoint point = ( *it );
    const double height = heightAt( point.x(), point.y() );

    try
    {
      point.transform( mLayerToTargetTransform );
    }
    catch ( QgsCsException & )
    {
      continue;
    }
    mResults->rawPoints.append( QgsPoint( point.x(), point.y(), height ) );
  }

  // convert x/y values back to distance/height values
  QgsGeos originalCurveGeos( mProfileCurve.get() );
  originalCurveGeos.prepareGeometry();
  mResults->results.reserve( mResults->rawPoints.size() );
  QString lastError;
  for ( const QgsPoint &pixel : std::as_const( mResults->rawPoints ) )
  {
    const double distance = originalCurveGeos.lineLocatePoint( pixel, &lastError );

    QgsMeshLayerProfileResults::Result res;
    res.distance = distance;
    res.height = pixel.z();
    mResults->results.push_back( res );
  }

  return true;
}

QgsAbstractProfileResults *QgsMeshLayerProfileGenerator::takeResults()
{
  return mResults.release();
}

double QgsMeshLayerProfileGenerator::heightAt( double x, double y )
{
  return QgsMeshLayerUtils::interpolateZForPoint( mTriangularMesh, x, y );
}

