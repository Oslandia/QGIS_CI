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
#include "qgslinesymbol.h"
#include "qgsmeshlayerelevationproperties.h"

//
// QgsMeshLayerProfileGenerator
//

QString QgsMeshLayerProfileResults::type() const
{
  return QStringLiteral( "mesh" );
}

QMap<double, double> QgsMeshLayerProfileResults::distanceToHeightMap() const
{
  return results;
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

QgsDoubleRange QgsMeshLayerProfileResults::zRange() const
{
  return QgsDoubleRange( minZ, maxZ );
}

void QgsMeshLayerProfileResults::renderResults( QgsProfileRenderContext &context )
{
  QPainter *painter = context.renderContext().painter();
  if ( !painter )
    return;

  const QgsScopedQPainterState painterState( painter );

  painter->setBrush( Qt::NoBrush );
  painter->setPen( Qt::NoPen );

  const double minDistance = context.distanceRange().lower();
  const double maxDistance = context.distanceRange().upper();
  const double minZ = context.elevationRange().lower();
  const double maxZ = context.elevationRange().upper();

  const QRectF visibleRegion( minDistance, minZ, maxDistance - minDistance, maxZ - minZ );
  QPainterPath clipPath;
  clipPath.addPolygon( context.worldTransform().map( visibleRegion ) );
  painter->setClipPath( clipPath, Qt::ClipOperation::IntersectClip );

  lineSymbol->startRender( context.renderContext() );

  QPolygonF currentLine;
  for ( auto pointIt = results.constBegin(); pointIt != results.constEnd(); ++pointIt )
  {
    if ( std::isnan( pointIt.value() ) )
    {
      if ( currentLine.length() > 1 )
      {
        lineSymbol->renderPolyline( currentLine, nullptr, context.renderContext() );
      }
      currentLine.clear();
      continue;
    }
    currentLine.append( context.worldTransform().map( QPointF( pointIt.key(), pointIt.value() ) ) );
  }
  if ( currentLine.length() > 1 )
  {
    lineSymbol->renderPolyline( currentLine, nullptr, context.renderContext() );
  }

  lineSymbol->stopRender( context.renderContext() );
}

//
// QgsMeshLayerProfileGenerator
//

QgsMeshLayerProfileGenerator::QgsMeshLayerProfileGenerator( QgsMeshLayer *layer, const QgsProfileRequest &request )
  : mFeedback( std::make_unique< QgsFeedback >() )
  , mProfileCurve( request.profileCurve() ? request.profileCurve()->clone() : nullptr )
  , mLineSymbol( qgis::down_cast< QgsMeshLayerElevationProperties* >( layer->elevationProperties() )->profileLineSymbol()->clone() )
  , mSourceCrs( layer->crs() )
  , mTargetCrs( request.crs() )
  , mTransformContext( request.transformContext() )
  , mOffset( layer->elevationProperties()->zOffset() )
  , mScale( layer->elevationProperties()->zScale() )
  , mStepDistance( request.stepDistance() )
{
  layer->updateTriangularMesh();
  mTriangularMesh = *layer->triangularMesh();
}

QgsMeshLayerProfileGenerator::~QgsMeshLayerProfileGenerator() = default;

bool QgsMeshLayerProfileGenerator::generateProfile()
{
  if ( !mProfileCurve || mFeedback->isCanceled() )
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

  if ( mFeedback->isCanceled() )
    return false;

  mResults = std::make_unique< QgsMeshLayerProfileResults >();
  mResults->lineSymbol.reset( mLineSymbol->clone() );

  // we don't currently have any method to determine line->mesh intersection points, so for now we just sample at about 100(?) points over the line
  const double curveLength = transformedCurve.length();

  if ( !std::isnan( mStepDistance ) )
    transformedCurve = transformedCurve.densifyByDistance( mStepDistance );
  else
    transformedCurve = transformedCurve.densifyByDistance( curveLength / 100 );

  if ( mFeedback->isCanceled() )
    return false;

  for ( auto it = transformedCurve.vertices_begin(); it != transformedCurve.vertices_end(); ++it )
  {
    if ( mFeedback->isCanceled() )
      return false;

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

  if ( mFeedback->isCanceled() )
    return false;

  // convert x/y values back to distance/height values
  QgsGeos originalCurveGeos( mProfileCurve.get() );
  originalCurveGeos.prepareGeometry();
  QString lastError;
  for ( const QgsPoint &pixel : std::as_const( mResults->rawPoints ) )
  {
    if ( mFeedback->isCanceled() )
      return false;

    const double distance = originalCurveGeos.lineLocatePoint( pixel, &lastError );

    mResults->minZ = std::min( pixel.z(), mResults->minZ );
    mResults->maxZ = std::max( pixel.z(), mResults->maxZ );
    mResults->results.insert( distance, pixel.z() );
  }

  return true;
}

QgsAbstractProfileResults *QgsMeshLayerProfileGenerator::takeResults()
{
  return mResults.release();
}

QgsFeedback *QgsMeshLayerProfileGenerator::feedback() const
{
  return mFeedback.get();
}

double QgsMeshLayerProfileGenerator::heightAt( double x, double y )
{
  return QgsMeshLayerUtils::interpolateZForPoint( mTriangularMesh, x, y ) * mScale + mOffset;
}

