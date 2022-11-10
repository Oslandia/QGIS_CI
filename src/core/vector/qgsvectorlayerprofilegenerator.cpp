/***************************************************************************
                         qgsvectorlayerprofilegenerator.cpp
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
#include "qgsvectorlayerprofilegenerator.h"
#include "qgsprofilerequest.h"
#include "qgscurve.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayerelevationproperties.h"
#include "qgscoordinatetransform.h"
#include "qgsgeos.h"
#include "qgsvectorlayerfeatureiterator.h"
#include "qgsterrainprovider.h"
#include "qgspolygon.h"
#include "qgstessellator.h"
#include "qgsmultipolygon.h"
#include "qgsmeshlayerutils.h"
#include "qgsmultipoint.h"
#include "qgsmultilinestring.h"
#include "qgslinesymbol.h"
#include "qgsfillsymbol.h"
#include "qgsmarkersymbol.h"
#include "qgsprofilepoint.h"
#include "qgsprofilesnapping.h"
#include "qgsexpressioncontextutils.h"
#include <QPolygonF>

//
// QgsVectorLayerProfileResults
//

QString QgsVectorLayerProfileResults::type() const
{
  return QStringLiteral( "vector" );
}

QVector<QgsGeometry> QgsVectorLayerProfileResults::asGeometries() const
{
  QVector<QgsGeometry> res;
  res.reserve( features.size() );
  for ( auto it = features.constBegin(); it != features.constEnd(); ++it )
  {
    for ( const Feature &feature : it.value() )
    {
      res.append( feature.geometry );
    }
  }
  return res;
}

QgsProfileSnapResult QgsVectorLayerProfileResults::snapPoint( const QgsProfilePoint &point, const QgsProfileSnapContext &context )
{
  switch ( profileType )
  {
    case Qgis::VectorProfileType::IndividualFeatures:
      return snapPointToIndividualFeatures( point, context );
    case Qgis::VectorProfileType::ContinuousSurface:
      return QgsAbstractProfileSurfaceResults::snapPoint( point, context );
  }
  BUILTIN_UNREACHABLE
}


QVector<QgsProfileIdentifyResults> QgsVectorLayerProfileResults::identify( const QgsDoubleRange &distanceRange, const QgsDoubleRange &elevationRange, const QgsProfileIdentifyContext & )
{
  QgsFeatureIds ids;
  auto visitFeature = [&ids]( QgsFeatureId featureId )
  {
    ids << featureId;
  };

  visitFeaturesInRange( distanceRange, elevationRange, visitFeature );
  if ( ids.empty() )
    return {};

  QVector< QVariantMap> idsList;
  for ( auto it = ids.constBegin(); it != ids.constEnd(); ++it )
    idsList.append( QVariantMap( {{QStringLiteral( "id" ), *it}} ) );

  return { QgsProfileIdentifyResults( mLayer, idsList ) };
}

QVector<QgsProfileIdentifyResults> QgsVectorLayerProfileResults::identify( const QgsProfilePoint &point, const QgsProfileIdentifyContext &context )
{
  QHash< QgsFeatureId, QVariantMap > features;
  auto visitFeature = [&features]( QgsFeatureId featureId, double delta, double distance, double elevation )
  {
    auto it = features.find( featureId );
    if ( it == features.end() )
    {
      features[ featureId ] = QVariantMap( {{QStringLiteral( "id" ), featureId },
        {QStringLiteral( "delta" ), delta },
        {QStringLiteral( "distance" ), distance },
        {QStringLiteral( "elevation" ), elevation }
      } );
    }
    else
    {
      const double currentDelta = it.value().value( QStringLiteral( "delta" ) ).toDouble();
      if ( delta < currentDelta )
      {
        *it = QVariantMap( {{QStringLiteral( "id" ), featureId },
          {QStringLiteral( "delta" ), delta },
          {QStringLiteral( "distance" ), distance },
          {QStringLiteral( "elevation" ), elevation }
        } );
      }
    }
  };

  visitFeaturesAtPoint( point, context.maximumPointDistanceDelta, context.maximumPointElevationDelta, context.maximumSurfaceElevationDelta, visitFeature, true );

  QVector< QVariantMap> attributes;
  for ( auto it = features.constBegin(); it != features.constEnd(); ++it )
    attributes.append( *it );

  QVector<QgsProfileIdentifyResults> res;

  if ( !attributes.empty() )
    res.append( QgsProfileIdentifyResults( mLayer, attributes ) );

  if ( res.empty() && profileType == Qgis::VectorProfileType::ContinuousSurface )
  {
    const QVector<QgsProfileIdentifyResults> surfaceResults = QgsAbstractProfileSurfaceResults::identify( point, context );
    res.reserve( surfaceResults.size() );
    for ( const QgsProfileIdentifyResults &surfaceResult : surfaceResults )
    {
      res.append( QgsProfileIdentifyResults( mLayer, surfaceResult.results() ) );
    }
  }

  return res;
}

QgsProfileSnapResult QgsVectorLayerProfileResults::snapPointToIndividualFeatures( const QgsProfilePoint &point, const QgsProfileSnapContext &context )
{
  QgsProfileSnapResult res;
  double bestSnapDistance = std::numeric_limits< double >::max();

  auto visitFeature = [&bestSnapDistance, &res]( QgsFeatureId, double delta, double distance, double elevation )
  {
    if ( distance < bestSnapDistance )
    {
      bestSnapDistance = delta;
      res.snappedPoint = QgsProfilePoint( distance, elevation );
    }
  };

  visitFeaturesAtPoint( point, context.maximumPointDistanceDelta, context.maximumPointElevationDelta, context.maximumSurfaceElevationDelta, visitFeature, false );

  return res;
}

void QgsVectorLayerProfileResults::visitFeaturesAtPoint( const QgsProfilePoint &point, double maximumPointDistanceDelta, double maximumPointElevationDelta, double maximumSurfaceElevationDelta,  const std::function< void( QgsFeatureId, double delta, double distance, double elevation ) > &visitor, bool visitWithin )
{
  // TODO -- add spatial index if performance is an issue

  const QgsPoint targetPoint( point.distance(), point.elevation() );

  for ( auto it = features.constBegin(); it != features.constEnd(); ++it )
  {
    for ( const Feature &feature : it.value() )
    {
      const QgsRectangle featureBounds = feature.crossSectionGeometry.boundingBox();
      if ( ( featureBounds.xMinimum() - maximumPointDistanceDelta <= point.distance() ) && ( featureBounds.xMaximum() + maximumPointDistanceDelta >= point.distance() ) )
      {
        switch ( feature.crossSectionGeometry.type() )
        {
          case QgsWkbTypes::PointGeometry:
          {
            for ( auto partIt = feature.crossSectionGeometry.const_parts_begin(); partIt != feature.crossSectionGeometry.const_parts_end(); ++partIt )
            {
              if ( const QgsPoint *candidatePoint = qgsgeometry_cast< const QgsPoint * >( *partIt ) )
              {
                const double snapDistanceDelta = std::fabs( point.distance() - candidatePoint->x() );
                if ( snapDistanceDelta > maximumPointDistanceDelta )
                  continue;

                const double snapHeightDelta = std::fabs( point.elevation() - candidatePoint->y() );
                if ( snapHeightDelta > maximumPointElevationDelta )
                  continue;

                const double snapDistance = candidatePoint->distance( targetPoint );
                visitor( feature.featureId, snapDistance, candidatePoint->x(), candidatePoint->y() );
              }
            }
            break;
          }

          case QgsWkbTypes::LineGeometry:
          {
            for ( auto partIt = feature.crossSectionGeometry.const_parts_begin(); partIt != feature.crossSectionGeometry.const_parts_end(); ++partIt )
            {
              if ( const QgsCurve *line = qgsgeometry_cast< const QgsCurve * >( *partIt ) )
              {
                // might be a vertical line
                if ( const QgsLineString *lineString = qgsgeometry_cast< const QgsLineString * >( line ) )
                {
                  if ( lineString->numPoints() == 2 && qgsDoubleNear( lineString->pointN( 0 ).x(), lineString->pointN( 1 ).x() ) )
                  {
                    const double snapDistanceDelta = std::fabs( point.distance() - lineString->pointN( 0 ).x() );
                    if ( snapDistanceDelta > maximumPointDistanceDelta )
                      continue;

                    const double snapHeightDelta = std::fabs( point.elevation() - lineString->pointN( 0 ).y() );
                    if ( snapHeightDelta <= maximumPointElevationDelta )
                    {
                      const double snapDistanceP1 = lineString->pointN( 0 ).distance( targetPoint );
                      visitor( feature.featureId, snapDistanceP1, lineString->pointN( 0 ).x(), lineString->pointN( 0 ).y() );
                    }

                    const double snapHeightDelta2 = std::fabs( point.elevation() - lineString->pointN( 1 ).y() );
                    if ( snapHeightDelta2 <= maximumPointElevationDelta )
                    {
                      const double snapDistanceP2 = lineString->pointN( 1 ).distance( targetPoint );
                      visitor( feature.featureId, snapDistanceP2, lineString->pointN( 1 ).x(), lineString->pointN( 1 ).y() );
                    }

                    if ( visitWithin )
                    {
                      double elevation1 = lineString->pointN( 0 ).y();
                      double elevation2 = lineString->pointN( 1 ).y();
                      if ( elevation1 > elevation2 )
                        std::swap( elevation1, elevation2 );

                      if ( point.elevation() > elevation1 && point.elevation() < elevation2 )
                      {
                        const double snapDistance = std::fabs( lineString->pointN( 0 ).x() - point.distance() );
                        visitor( feature.featureId, snapDistance, lineString->pointN( 0 ).x(), point.elevation() );
                      }
                    }
                    continue;
                  }
                }

                const QgsRectangle partBounds = ( *partIt )->boundingBox();
                if ( point.distance() < partBounds.xMinimum() - maximumPointDistanceDelta || point.distance() > partBounds.xMaximum() + maximumPointDistanceDelta )
                  continue;

                const double snappedDistance = point.distance() < partBounds.xMinimum() ? partBounds.xMinimum()
                                               : point.distance() > partBounds.xMaximum() ? partBounds.xMaximum() : point.distance();

                const QgsGeometry cutLine( new QgsLineString( QgsPoint( snappedDistance, minZ ), QgsPoint( snappedDistance, maxZ ) ) );
                QgsGeos cutLineGeos( cutLine.constGet() );

                const QgsGeometry points( cutLineGeos.intersection( line ) );

                for ( auto vertexIt = points.vertices_begin(); vertexIt != points.vertices_end(); ++vertexIt )
                {
                  const double snapHeightDelta = std::fabs( point.elevation() - ( *vertexIt ).y() );
                  if ( snapHeightDelta > maximumSurfaceElevationDelta )
                    continue;

                  const double snapDistance = ( *vertexIt ).distance( targetPoint );
                  visitor( feature.featureId, snapDistance, ( *vertexIt ).x(), ( *vertexIt ).y() );
                }
              }
            }
            break;
          }

          case QgsWkbTypes::PolygonGeometry:
          {
            if ( visitWithin )
            {
              if ( feature.crossSectionGeometry.intersects( QgsGeometry::fromPointXY( QgsPointXY( point.distance(), point.elevation() ) ) ) )
              {
                visitor( feature.featureId, 0, point.distance(), point.elevation() );
                break;
              }
            }
            for ( auto partIt = feature.crossSectionGeometry.const_parts_begin(); partIt != feature.crossSectionGeometry.const_parts_end(); ++partIt )
            {
              if ( const QgsCurve *exterior = qgsgeometry_cast< const QgsPolygon * >( *partIt )->exteriorRing() )
              {
                const QgsRectangle partBounds = ( *partIt )->boundingBox();
                if ( point.distance() < partBounds.xMinimum() - maximumPointDistanceDelta || point.distance() > partBounds.xMaximum() + maximumPointDistanceDelta )
                  continue;

                const double snappedDistance = point.distance() < partBounds.xMinimum() ? partBounds.xMinimum()
                                               : point.distance() > partBounds.xMaximum() ? partBounds.xMaximum() : point.distance();

                const QgsGeometry cutLine( new QgsLineString( QgsPoint( snappedDistance, minZ ), QgsPoint( snappedDistance, maxZ ) ) );
                QgsGeos cutLineGeos( cutLine.constGet() );

                const QgsGeometry points( cutLineGeos.intersection( exterior ) );
                for ( auto vertexIt = points.vertices_begin(); vertexIt != points.vertices_end(); ++vertexIt )
                {
                  const double snapHeightDelta = std::fabs( point.elevation() - ( *vertexIt ).y() );
                  if ( snapHeightDelta > maximumSurfaceElevationDelta )
                    continue;

                  const double snapDistance = ( *vertexIt ).distance( targetPoint );
                  visitor( feature.featureId, snapDistance, ( *vertexIt ).x(), ( *vertexIt ).y() );
                }
              }
            }
            break;
          }
          case QgsWkbTypes::UnknownGeometry:
          case QgsWkbTypes::NullGeometry:
            break;
        }
      }
    }
  }
}

void QgsVectorLayerProfileResults::visitFeaturesInRange( const QgsDoubleRange &distanceRange, const QgsDoubleRange &elevationRange, const std::function<void ( QgsFeatureId )> &visitor )
{
  // TODO -- add spatial index if performance is an issue
  const QgsRectangle profileRange( distanceRange.lower(), elevationRange.lower(), distanceRange.upper(), elevationRange.upper() );
  const QgsGeometry profileRangeGeometry = QgsGeometry::fromRect( profileRange );
  QgsGeos profileRangeGeos( profileRangeGeometry.constGet() );
  profileRangeGeos.prepareGeometry();

  for ( auto it = features.constBegin(); it != features.constEnd(); ++it )
  {
    for ( const Feature &feature : it.value() )
    {
      if ( feature.crossSectionGeometry.boundingBoxIntersects( profileRange ) )
      {
        switch ( feature.crossSectionGeometry.type() )
        {
          case QgsWkbTypes::PointGeometry:
          {
            for ( auto partIt = feature.crossSectionGeometry.const_parts_begin(); partIt != feature.crossSectionGeometry.const_parts_end(); ++partIt )
            {
              if ( const QgsPoint *candidatePoint = qgsgeometry_cast< const QgsPoint * >( *partIt ) )
              {
                if ( profileRange.contains( candidatePoint->x(), candidatePoint->y() ) )
                {
                  visitor( feature.featureId );
                }
              }
            }
            break;
          }

          case QgsWkbTypes::LineGeometry:
          case QgsWkbTypes::PolygonGeometry:
          {
            if ( profileRangeGeos.intersects( feature.crossSectionGeometry.constGet() ) )
            {
              visitor( feature.featureId );
            }
            break;
          }

          case QgsWkbTypes::UnknownGeometry:
          case QgsWkbTypes::NullGeometry:
            break;
        }
      }
    }
  }
}

void QgsVectorLayerProfileResults::renderResults( QgsProfileRenderContext &context )
{
  const QgsExpressionContextScopePopper scopePopper( context.renderContext().expressionContext(), mLayer ? mLayer->createExpressionContextScope() : nullptr );
  switch ( profileType )
  {
    case Qgis::VectorProfileType::IndividualFeatures:
      renderResultsAsIndividualFeatures( context );
      break;
    case Qgis::VectorProfileType::ContinuousSurface:
      QgsAbstractProfileSurfaceResults::renderResults( context );
      if ( mShowMarkerSymbolInSurfacePlots )
        renderMarkersOverContinuousSurfacePlot( context );
      break;
  }
}

void QgsVectorLayerProfileResults::renderResultsAsIndividualFeatures( QgsProfileRenderContext &context )
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

  const QgsRectangle clipPathRect( clipPath.boundingRect() );

  auto renderResult = [&context, &clipPathRect]( const Feature & profileFeature, QgsMarkerSymbol * markerSymbol, QgsLineSymbol * lineSymbol, QgsFillSymbol * fillSymbol )
  {
    if ( profileFeature.crossSectionGeometry.isEmpty() )
      return;

    QgsGeometry transformed = profileFeature.crossSectionGeometry;
    transformed.transform( context.worldTransform() );

    if ( !transformed.boundingBoxIntersects( clipPathRect ) )
      return;

    // we can take some shortcuts here, because we know that the geometry will already be segmentized and can't be a curved type
    switch ( transformed.type() )
    {
      case QgsWkbTypes::PointGeometry:
      {
        if ( const QgsPoint *point = qgsgeometry_cast< const QgsPoint * >( transformed.constGet() ) )
        {
          markerSymbol->renderPoint( QPointF( point->x(), point->y() ), nullptr, context.renderContext() );
        }
        else if ( const QgsMultiPoint *multipoint = qgsgeometry_cast< const QgsMultiPoint * >( transformed.constGet() ) )
        {
          const int numGeometries = multipoint->numGeometries();
          for ( int i = 0; i < numGeometries; ++i )
          {
            markerSymbol->renderPoint( QPointF( multipoint->pointN( i )->x(), multipoint->pointN( i )->y() ), nullptr, context.renderContext() );
          }
        }
        break;
      }

      case QgsWkbTypes::LineGeometry:
      {
        if ( const QgsLineString *line = qgsgeometry_cast< const QgsLineString * >( transformed.constGet() ) )
        {
          lineSymbol->renderPolyline( line->asQPolygonF(), nullptr, context.renderContext() );
        }
        else if ( const QgsMultiLineString *multiLinestring = qgsgeometry_cast< const QgsMultiLineString * >( transformed.constGet() ) )
        {
          const int numGeometries = multiLinestring->numGeometries();
          for ( int i = 0; i < numGeometries; ++i )
          {
            lineSymbol->renderPolyline( multiLinestring->lineStringN( i )->asQPolygonF(), nullptr, context.renderContext() );
          }
        }
        break;
      }

      case QgsWkbTypes::PolygonGeometry:
      {
        if ( const QgsPolygon *polygon = qgsgeometry_cast< const QgsPolygon * >( transformed.constGet() ) )
        {
          if ( const QgsCurve *exterior = polygon->exteriorRing() )
            fillSymbol->renderPolygon( exterior->asQPolygonF(), nullptr, nullptr, context.renderContext() );
        }
        else if ( const QgsMultiPolygon *multiPolygon = qgsgeometry_cast< const QgsMultiPolygon * >( transformed.constGet() ) )
        {
          const int numGeometries = multiPolygon->numGeometries();
          for ( int i = 0; i < numGeometries; ++i )
          {
            fillSymbol->renderPolygon( multiPolygon->polygonN( i )->exteriorRing()->asQPolygonF(), nullptr, nullptr, context.renderContext() );
          }
        }
        break;
      }

      case QgsWkbTypes::UnknownGeometry:
      case QgsWkbTypes::NullGeometry:
        return;
    }
  };

  QgsFeatureRequest req;
  req.setFilterFids( qgis::listToSet( features.keys() ) );

  if ( respectLayerSymbology && mLayer && mLayer->renderer() )
  {
    std::unique_ptr< QgsFeatureRenderer > renderer( mLayer->renderer()->clone() );
    renderer->startRender( context.renderContext(), mLayer->fields() );

    // if we are respecting the layer's symbology then we'll fire off a feature request and iterate through
    // features from the profile, rendering each in turn
    QSet<QString> attributes = renderer->usedAttributes( context.renderContext() );

    std::unique_ptr< QgsMarkerSymbol > marker( mMarkerSymbol->clone() );
    std::unique_ptr< QgsLineSymbol > line( mLineSymbol->clone() );
    std::unique_ptr< QgsFillSymbol > fill( mFillSymbol->clone() );
    attributes.unite( marker->usedAttributes( context.renderContext() ) );
    attributes.unite( line->usedAttributes( context.renderContext() ) );
    attributes.unite( fill->usedAttributes( context.renderContext() ) );

    req.setSubsetOfAttributes( attributes, mLayer->fields() );

    QgsFeature feature;
    QgsFeatureIterator it = mLayer->getFeatures( req );
    while ( it.nextFeature( feature ) )
    {
      context.renderContext().expressionContext().setFeature( feature );
      QgsSymbol *rendererSymbol = renderer->symbolForFeature( feature, context.renderContext() );
      if ( !rendererSymbol )
        continue;

      marker->setColor( rendererSymbol->color() );
      marker->setOpacity( rendererSymbol->opacity() );
      line->setColor( rendererSymbol->color() );
      line->setOpacity( rendererSymbol->opacity() );
      fill->setColor( rendererSymbol->color() );
      fill->setOpacity( rendererSymbol->opacity() );

      marker->startRender( context.renderContext() );
      line->startRender( context.renderContext() );
      fill->startRender( context.renderContext() );

      const QVector< Feature > profileFeatures = features.value( feature.id() );
      for ( const Feature &profileFeature : profileFeatures )
      {
        renderResult( profileFeature,
                      rendererSymbol->type() == Qgis::SymbolType::Marker ? qgis::down_cast< QgsMarkerSymbol * >( rendererSymbol ) : marker.get(),
                      rendererSymbol->type() == Qgis::SymbolType::Line ? qgis::down_cast< QgsLineSymbol * >( rendererSymbol )  : line.get(),
                      rendererSymbol->type() == Qgis::SymbolType::Fill ? qgis::down_cast< QgsFillSymbol * >( rendererSymbol )  : fill.get() );
      }

      marker->stopRender( context.renderContext() );
      line->stopRender( context.renderContext() );
      fill->stopRender( context.renderContext() );
    }

    renderer->stopRender( context.renderContext() );
  }
  else
  {
    QSet<QString> attributes;
    attributes.unite( mMarkerSymbol->usedAttributes( context.renderContext() ) );
    attributes.unite( mFillSymbol->usedAttributes( context.renderContext() ) );
    attributes.unite( mLineSymbol->usedAttributes( context.renderContext() ) );

    mMarkerSymbol->startRender( context.renderContext() );
    mFillSymbol->startRender( context.renderContext() );
    mLineSymbol->startRender( context.renderContext() );
    req.setSubsetOfAttributes( attributes, mLayer->fields() );

    QgsFeature feature;
    QgsFeatureIterator it = mLayer->getFeatures( req );
    while ( it.nextFeature( feature ) )
    {
      context.renderContext().expressionContext().setFeature( feature );
      const QVector< Feature > profileFeatures = features.value( feature.id() );
      for ( const Feature &profileFeature : profileFeatures )
      {
        renderResult( profileFeature, mMarkerSymbol.get(), mLineSymbol.get(), mFillSymbol.get() );
      }
    }
    mMarkerSymbol->stopRender( context.renderContext() );
    mFillSymbol->stopRender( context.renderContext() );
    mLineSymbol->stopRender( context.renderContext() );
  }
}

void QgsVectorLayerProfileResults::renderMarkersOverContinuousSurfacePlot( QgsProfileRenderContext &context )
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

  mMarkerSymbol->startRender( context.renderContext() );

  for ( auto pointIt = mDistanceToHeightMap.constBegin(); pointIt != mDistanceToHeightMap.constEnd(); ++pointIt )
  {
    if ( std::isnan( pointIt.value() ) )
      continue;

    mMarkerSymbol->renderPoint( context.worldTransform().map( QPointF( pointIt.key(), pointIt.value() ) ), nullptr, context.renderContext() );
  }
  mMarkerSymbol->stopRender( context.renderContext() );
}

void QgsVectorLayerProfileResults::copyPropertiesFromGenerator( const QgsAbstractProfileGenerator *generator )
{
  QgsAbstractProfileSurfaceResults::copyPropertiesFromGenerator( generator );
  const QgsVectorLayerProfileGenerator *vlGenerator = qgis::down_cast<  const QgsVectorLayerProfileGenerator * >( generator );

  profileType = vlGenerator->mType;
  respectLayerSymbology = vlGenerator->mRespectLayerSymbology;
  mMarkerSymbol.reset( vlGenerator->mProfileMarkerSymbol->clone() );
  mShowMarkerSymbolInSurfacePlots = vlGenerator->mShowMarkerSymbolInSurfacePlots;
}

//
// QgsVectorLayerProfileGenerator
//

QgsVectorLayerProfileGenerator::QgsVectorLayerProfileGenerator( QgsVectorLayer *layer, const QgsProfileRequest &request )
  : mId( layer->id() )
  , mFeedback( std::make_unique< QgsFeedback >() )
  , mProfileCurve( request.profileCurve() ? request.profileCurve()->clone() : nullptr )
  , mTerrainProvider( request.terrainProvider() ? request.terrainProvider()->clone() : nullptr )
  , mTolerance( request.tolerance() )
  , mSourceCrs( layer->crs() )
  , mTargetCrs( request.crs() )
  , mTransformContext( request.transformContext() )
  , mExtent( layer->extent() )
  , mSource( std::make_unique< QgsVectorLayerFeatureSource >( layer ) )
  , mOffset( layer->elevationProperties()->zOffset() )
  , mScale( layer->elevationProperties()->zScale() )
  , mType( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->type() )
  , mClamping( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->clamping() )
  , mBinding( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->binding() )
  , mExtrusionEnabled( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->extrusionEnabled() )
  , mExtrusionHeight( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->extrusionHeight() )
  , mExpressionContext( request.expressionContext() )
  , mFields( layer->fields() )
  , mDataDefinedProperties( layer->elevationProperties()->dataDefinedProperties() )
  , mWkbType( layer->wkbType() )
  , mRespectLayerSymbology( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->respectLayerSymbology() )
  , mProfileMarkerSymbol( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->profileMarkerSymbol()->clone() )
  , mShowMarkerSymbolInSurfacePlots( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->showMarkerSymbolInSurfacePlots() )
  , mLayer( layer )
{
  if ( mTerrainProvider )
    mTerrainProvider->prepare(); // must be done on main thread

  mSymbology = qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->profileSymbology();
  mLineSymbol.reset( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->profileLineSymbol()->clone() );
  mFillSymbol.reset( qgis::down_cast< QgsVectorLayerElevationProperties * >( layer->elevationProperties() )->profileFillSymbol()->clone() );
}

QString QgsVectorLayerProfileGenerator::sourceId() const
{
  return mId;
}

QgsVectorLayerProfileGenerator::~QgsVectorLayerProfileGenerator() = default;

bool QgsVectorLayerProfileGenerator::generateProfile( const QgsProfileGenerationContext & )
{
  if ( !mProfileCurve || mFeedback->isCanceled() )
    return false;

  // we need to transform the profile curve to the vector's CRS
  mTransformedCurve.reset( mProfileCurve->clone() );
  mLayerToTargetTransform = QgsCoordinateTransform( mSourceCrs, mTargetCrs, mTransformContext );
  if ( mTerrainProvider )
    mTargetToTerrainProviderTransform = QgsCoordinateTransform( mTargetCrs, mTerrainProvider->crs(), mTransformContext );

  try
  {
    mTransformedCurve->transform( mLayerToTargetTransform, Qgis::TransformDirection::Reverse );
  }
  catch ( QgsCsException & )
  {
    QgsDebugMsg( QStringLiteral( "Error transforming profile line to vector CRS" ) );
    return false;
  }

  const QgsRectangle profileCurveBoundingBox = mTransformedCurve->boundingBox();
  if ( !profileCurveBoundingBox.intersects( mExtent ) )
    return false;

  if ( mFeedback->isCanceled() )
    return false;

  mResults = std::make_unique< QgsVectorLayerProfileResults >();
  mResults->mLayer = mLayer;
  mResults->copyPropertiesFromGenerator( this );

  mProfileCurveEngine.reset( new QgsGeos( mProfileCurve.get() ) );
  mProfileCurveEngine->prepareGeometry();

  mProfileBox = mProfileCurveEngine->buffer( mTolerance, 8, Qgis::EndCapStyle::Flat, Qgis::JoinStyle::Round, 2 );
  mProfileBoxEngine.reset( new QgsGeos( mProfileBox ) );
  mProfileBoxEngine->prepareGeometry();

  mDataDefinedProperties.prepare( mExpressionContext );

  if ( mFeedback->isCanceled() )
    return false;

  switch ( QgsWkbTypes::geometryType( mWkbType ) )
  {
    case QgsWkbTypes::PointGeometry:
      if ( !generateProfileForPoints() )
        return false;
      break;

    case QgsWkbTypes::LineGeometry:
      if ( !generateProfileForLinesWithBox() )
        return false;
      break;

    case QgsWkbTypes::PolygonGeometry:
      if ( !generateProfileForPolygons() )
        return false;
      break;

    case QgsWkbTypes::NullGeometry:
      return false;

    case QgsWkbTypes::UnknownGeometry:
      return false;
  }

  return true;
}

QgsAbstractProfileResults *QgsVectorLayerProfileGenerator::takeResults()
{
  return mResults.release();
}

QgsFeedback *QgsVectorLayerProfileGenerator::feedback() const
{
  return mFeedback.get();
}

bool QgsVectorLayerProfileGenerator::generateProfileForPoints()
{
  // get features from layer
  QgsFeatureRequest request;
  request.setDestinationCrs( mTargetCrs, mTransformContext );
  request.setDistanceWithin( QgsGeometry( mProfileCurve->clone() ), mTolerance );
  request.setSubsetOfAttributes( mDataDefinedProperties.referencedFields( mExpressionContext ), mFields );
  request.setFeedback( mFeedback.get() );

  // our feature request is using the optimised distance within check (allowing use of spatial index)
  // BUT this will also include points which are within the tolerance distance before/after the end of line.
  // So we also need to double check that they fall within the flat buffered curve too.
  std::unique_ptr< QgsAbstractGeometry > bufferedCurve( mProfileCurveEngine->buffer( mTolerance, 8, Qgis::EndCapStyle::Flat, Qgis::JoinStyle::Round, 2 ) );
  QgsGeos bufferedCurveEngine( bufferedCurve.get() );
  bufferedCurveEngine.prepareGeometry();

  auto processPoint = [this, &bufferedCurveEngine]( const QgsFeature & feature, const QgsPoint * point )
  {
    if ( !bufferedCurveEngine.intersects( point ) )
      return;

    const double offset = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ZOffset, mExpressionContext, mOffset );

    const double height = featureZToHeight( point->x(), point->y(), point->z(), offset );
    mResults->mRawPoints.append( QgsPoint( point->x(), point->y(), height ) );
    mResults->minZ = std::min( mResults->minZ, height );
    mResults->maxZ = std::max( mResults->maxZ, height );

    QString lastError;
    const double distance = mProfileCurveEngine->lineLocatePoint( *point, &lastError );
    mResults->mDistanceToHeightMap.insert( distance, height );

    QgsVectorLayerProfileResults::Feature resultFeature;
    resultFeature.featureId = feature.id();
    if ( mExtrusionEnabled )
    {
      const double extrusion = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ExtrusionHeight, mExpressionContext, mExtrusionHeight );

      resultFeature.geometry = QgsGeometry( new QgsLineString( QgsPoint( point->x(), point->y(), height ),
                                            QgsPoint( point->x(), point->y(), height + extrusion ) ) );
      resultFeature.crossSectionGeometry = QgsGeometry( new QgsLineString( QgsPoint( distance, height ),
                                           QgsPoint( distance, height + extrusion ) ) );
      mResults->minZ = std::min( mResults->minZ, height + extrusion );
      mResults->maxZ = std::max( mResults->maxZ, height + extrusion );
    }
    else
    {
      resultFeature.geometry = QgsGeometry( new QgsPoint( point->x(), point->y(), height ) );
      resultFeature.crossSectionGeometry = QgsGeometry( new QgsPoint( distance, height ) );
    }
    mResults->features[resultFeature.featureId].append( resultFeature );
  };

  QgsFeature feature;
  QgsFeatureIterator it = mSource->getFeatures( request );
  while ( it.nextFeature( feature ) )
  {
    if ( mFeedback->isCanceled() )
      return false;

    mExpressionContext.setFeature( feature );

    const QgsGeometry g = feature.geometry();
    if ( g.isMultipart() )
    {
      for ( auto it = g.const_parts_begin(); it != g.const_parts_end(); ++it )
      {
        processPoint( feature, qgsgeometry_cast< const QgsPoint * >( *it ) );
      }
    }
    else
    {
      processPoint( feature, qgsgeometry_cast< const QgsPoint * >( g.constGet() ) );
    }
  }
  return true;
}

bool QgsVectorLayerProfileGenerator::generateProfileForLines()
{
  // get features from layer
  QgsFeatureRequest request;
  request.setDestinationCrs( mTargetCrs, mTransformContext );
  request.setFilterRect( mProfileCurve->boundingBox() );
  request.setSubsetOfAttributes( mDataDefinedProperties.referencedFields( mExpressionContext ), mFields );
  request.setFeedback( mFeedback.get() );

  auto processCurve = [this]( const QgsFeature & feature, const QgsCurve * curve )
  {
    QString error;
    std::unique_ptr< QgsAbstractGeometry > intersection( mProfileCurveEngine->intersection( curve, &error ) );
    if ( !intersection )
      return;

    if ( mFeedback->isCanceled() )
      return;

    QgsGeos curveGeos( curve );
    curveGeos.prepareGeometry();

    if ( mFeedback->isCanceled() )
      return;

    for ( auto it = intersection->const_parts_begin(); it != intersection->const_parts_end(); ++it )
    {
      if ( mFeedback->isCanceled() )
        return;

      if ( const QgsPoint *intersectionPoint = qgsgeometry_cast< const QgsPoint * >( *it ) )
      {
        // unfortunately we need to do some work to interpolate the z value for the line -- GEOS doesn't give us this
        const double distance = curveGeos.lineLocatePoint( *intersectionPoint, &error );
        std::unique_ptr< QgsPoint > interpolatedPoint( curve->interpolatePoint( distance ) );

        const double offset = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ZOffset, mExpressionContext, mOffset );

        const double height = featureZToHeight( interpolatedPoint->x(), interpolatedPoint->y(), interpolatedPoint->z(), offset );
        mResults->mRawPoints.append( QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height ) );
        mResults->minZ = std::min( mResults->minZ, height );
        mResults->maxZ = std::max( mResults->maxZ, height );

        const double distanceAlongProfileCurve = mProfileCurveEngine->lineLocatePoint( *interpolatedPoint, &error );
        mResults->mDistanceToHeightMap.insert( distanceAlongProfileCurve, height );

        QgsVectorLayerProfileResults::Feature resultFeature;
        resultFeature.featureId = feature.id();
        if ( mExtrusionEnabled )
        {
          const double extrusion = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ExtrusionHeight, mExpressionContext, mExtrusionHeight );

          resultFeature.geometry = QgsGeometry( new QgsLineString( QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height ),
                                                QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height + extrusion ) ) );
          resultFeature.crossSectionGeometry = QgsGeometry( new QgsLineString( QgsPoint( distanceAlongProfileCurve, height ),
                                               QgsPoint( distanceAlongProfileCurve, height + extrusion ) ) );
          mResults->minZ = std::min( mResults->minZ, height + extrusion );
          mResults->maxZ = std::max( mResults->maxZ, height + extrusion );
        }
        else
        {
          resultFeature.geometry = QgsGeometry( new QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height ) );
          resultFeature.crossSectionGeometry = QgsGeometry( new QgsPoint( distanceAlongProfileCurve, height ) );
        }
        mResults->features[resultFeature.featureId].append( resultFeature );
      }
    }
  };

  QgsFeature feature;
  QgsFeatureIterator it = mSource->getFeatures( request );
  while ( it.nextFeature( feature ) )
  {
    if ( mFeedback->isCanceled() )
      return false;

    if ( !mProfileCurveEngine->intersects( feature.geometry().constGet() ) )
      continue;

    mExpressionContext.setFeature( feature );

    const QgsGeometry g = feature.geometry();
    if ( g.isMultipart() )
    {
      for ( auto it = g.const_parts_begin(); it != g.const_parts_end(); ++it )
      {
        if ( !mProfileCurveEngine->intersects( *it ) )
          continue;

        processCurve( feature, qgsgeometry_cast< const QgsCurve * >( *it ) );
      }
    }
    else
    {
      processCurve( feature, qgsgeometry_cast< const QgsCurve * >( g.constGet() ) );
    }
  }
  return true;
}

bool QgsVectorLayerProfileGenerator::generateProfileForLinesWithBox()
{
  // get features from layer
  QgsFeatureRequest request;
  request.setDestinationCrs( mTargetCrs, mTransformContext );
  request.setFilterRect( mProfileBox->boundingBox() );
  request.setSubsetOfAttributes( mDataDefinedProperties.referencedFields( mExpressionContext ), mFields );
  request.setFeedback( mFeedback.get() );

  std::cout << "generateProfileForLinesWithBox" << std::endl;

  auto processCurve = [this]( const QgsFeature & feature, const QgsCurve * curve )
  {
    auto processPoint = [this, curve]( const QgsPoint & intersectionPoint, const QgsGeos & curveGeos,  const QgsFeature & feature )
    {

      QString error;

      // unfortunately we need to do some work to interpolate the z value for the line -- GEOS doesn't give us this
      const double distance = curveGeos.lineLocatePoint( intersectionPoint, &error );
      std::unique_ptr< QgsPoint > interpolatedPoint( curve->interpolatePoint( distance ) );
      std::cout << "interpolatedPoint x" << interpolatedPoint->x() << std::endl;
      std::cout << "interpolatedPoint y" << interpolatedPoint->y() << std::endl;

      const double offset = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ZOffset, mExpressionContext, mOffset );

      const double height = featureZToHeight( interpolatedPoint->x(), interpolatedPoint->y(), interpolatedPoint->z(), offset );
      mResults->mRawPoints.append( QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height ) );
      mResults->minZ = std::min( mResults->minZ, height );
      mResults->maxZ = std::max( mResults->maxZ, height );

      const double distanceAlongProfileCurve = mProfileCurveEngine->lineLocatePoint( *interpolatedPoint, &error );
      mResults->mDistanceToHeightMap.insert( distanceAlongProfileCurve, height );

      QgsVectorLayerProfileResults::Feature resultFeature;
      resultFeature.featureId = feature.id();
      if ( mExtrusionEnabled )
      {
        const double extrusion = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ExtrusionHeight, mExpressionContext, mExtrusionHeight );

        resultFeature.geometry = QgsGeometry( new QgsLineString( QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height ),
                                              QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height + extrusion ) ) );
        resultFeature.crossSectionGeometry = QgsGeometry( new QgsLineString( QgsPoint( distanceAlongProfileCurve, height ),
                                             QgsPoint( distanceAlongProfileCurve, height + extrusion ) ) );
        mResults->minZ = std::min( mResults->minZ, height + extrusion );
        mResults->maxZ = std::max( mResults->maxZ, height + extrusion );
      }
      else
      {
        resultFeature.geometry = QgsGeometry( new QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height ) );
        resultFeature.crossSectionGeometry = QgsGeometry( new QgsPoint( distanceAlongProfileCurve, height ) );
      }
      mResults->features[resultFeature.featureId].append( resultFeature );
    };

    auto processIntersectionCurve = [this, curve]( const QgsCurve & intersectionCurve, const QgsGeos & curveGeos,  const QgsFeature & feature )
    {
      std::cout << "curve intersection " << intersectionCurve.asWkt( 3 ).toStdString() << std::endl;

      QString error;
      QVector< QgsGeometry > transformedParts;
      QVector< QgsGeometry > crossSectionParts;

      QString lastError;
      QgsVectorLayerProfileResults::Feature resultFeature;
      resultFeature.featureId = feature.id();
      double lastDistanceAlongProfileCurve = 0.0;

      for ( auto it = intersectionCurve.vertices_begin(); it != intersectionCurve.vertices_end(); ++it )
      {

        std::cout << "curve intersection part " << ( *it ).geometryType().toStdString() << std::endl;

        const QgsPoint &intersectionPoint = ( *it );
        std::cout << "intersectionPoint " << intersectionPoint.asWkt( 3 ).toStdString() << std::endl;

        // unfortunately we need to do some work to interpolate the z value for the line -- GEOS doesn't give us this
        const double distance = curveGeos.lineLocatePoint( intersectionPoint, &error );
        std::cout << "curve intersection distance " << distance << std::endl;
        std::unique_ptr< QgsPoint > interpolatedPoint( curve->interpolatePoint( distance ) );
        
        std::cout << "interpolatedPoint x" << interpolatedPoint->x() << std::endl;
        std::cout << "interpolatedPoint y" << interpolatedPoint->y() << std::endl;
        std::cout << "interpolatedPoint z" << interpolatedPoint->z() << std::endl;

        const double offset = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ZOffset, mExpressionContext, mOffset );

        const double height = featureZToHeight( interpolatedPoint->x(), interpolatedPoint->y(), interpolatedPoint->z(), offset );
        mResults->mRawPoints.append( QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height ) );
        mResults->minZ = std::min( mResults->minZ, height );
        mResults->maxZ = std::max( mResults->maxZ, height );

        const double distanceAlongProfileCurve = mProfileCurveEngine->lineLocatePoint( *interpolatedPoint, &error );
        lastDistanceAlongProfileCurve = distanceAlongProfileCurve;
        mResults->mDistanceToHeightMap.insert( distanceAlongProfileCurve, height );

        if ( mExtrusionEnabled )
        {
          const double extrusion = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ExtrusionHeight, mExpressionContext, mExtrusionHeight );

          transformedParts.append( QgsGeometry( new QgsLineString( QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height ),
                                                QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height + extrusion ) ) ) );
          crossSectionParts.append( QgsGeometry( new QgsLineString( QgsPoint( distanceAlongProfileCurve, height ),
                                                 QgsPoint( distanceAlongProfileCurve, height + extrusion ) ) ) );
          mResults->minZ = std::min( mResults->minZ, height + extrusion );
          mResults->maxZ = std::max( mResults->maxZ, height + extrusion );
        }
        else
        {
          transformedParts.append( QgsGeometry( new QgsPoint( interpolatedPoint->x(), interpolatedPoint->y(), height ) ) );
          crossSectionParts.append( QgsGeometry( new QgsPoint( distanceAlongProfileCurve, height ) ) );
        }
      }
      
      mResults->mDistanceToHeightMap.insert( lastDistanceAlongProfileCurve + 0.001, qQNaN() );
      
      resultFeature.geometry = transformedParts.size() > 1 ? QgsGeometry::unaryUnion( transformedParts ) : transformedParts.value( 0 );
      if ( !crossSectionParts.empty() )
      {

        std::cout << "Create linestring from point list " << crossSectionParts.size() << std::endl;
        //Create linestring from point list
        QgsLineString *line = new QgsLineString();
        for ( auto it = crossSectionParts.begin(); it != crossSectionParts.end(); ++it )
        {
          std::cout << "Add vertexe" << std::endl;
          line->addVertex( QgsPoint( ( *it ).asPoint() ) );
        }

        std::cout << "Define geom from linestring" << std::endl;
        resultFeature.crossSectionGeometry = QgsGeometry( line );

        /*
        QgsGeometry unioned = QgsGeometry::unaryUnion( crossSectionParts );
        if ( unioned.type() == QgsWkbTypes::LineGeometry )
          unioned = unioned.mergeLines();
        resultFeature.crossSectionGeometry = unioned;
        */
      }

      mResults->features[resultFeature.featureId].append( resultFeature );
    };

    QString error;
    std::unique_ptr< QgsAbstractGeometry > intersection( mProfileBoxEngine->intersection( curve, &error ) );
    if ( !intersection )
    {
      std::cout << "not intersected" << std::endl;
      return;
    }
    else
    {
      std::cout << "intersection " << intersection->partCount() << " error" << error.toStdString() << std::endl;
      std::cout << "intersection geometry"  << intersection->geometryType().toStdString() << std::endl;
    }

    if ( mFeedback->isCanceled() )
      return;

    bool isEmpty = intersection->isEmpty();


    // Intersection is empty : GEOS issue for vertical intersection : use feature geometry as intersection
    if ( isEmpty)
    {
      std::string wkt_std = intersection->asWkt().toStdString();
      std::cout << "intersection wkt " << wkt_std << std::endl;
      std::cout << "feature wkt " << feature.geometry().asWkt().toStdString() << std::endl;

      intersection.reset(feature.geometry().constGet()->clone());
      std::cout << "intersection is empty. New intersection" << intersection->asWkt( 3 ).toStdString() << std::endl;
    }

    QgsGeos curveGeos( curve );
    curveGeos.prepareGeometry();

    if ( mFeedback->isCanceled() )
      return;

    for ( auto it = intersection->const_parts_begin(); it != intersection->const_parts_end(); ++it )
    {

      std::cout << "intersection geometry contains" << ( *it )->geometryType().toStdString() << std::endl;

      if ( mFeedback->isCanceled() )
        return;

      if ( const QgsPoint *intersectionPoint = qgsgeometry_cast< const QgsPoint * >( *it ) )
      {
        processPoint( *intersectionPoint, curveGeos, feature );

      }
      else if ( const QgsCurve *intersectionCurve = qgsgeometry_cast< const QgsCurve * >( *it ) )
      {
        processIntersectionCurve( *intersectionCurve, curveGeos, feature );
      }
    }
  };

  QgsFeature feature;
  QgsFeatureIterator it = mSource->getFeatures( request );
  while ( it.nextFeature( feature ) )
  {
    if ( mFeedback->isCanceled() )
      return false;

    if ( !mProfileBoxEngine->intersects( feature.geometry().constGet() ) )
      continue;

    
    std::cout << "feature intersect with box" << std::endl;

    mExpressionContext.setFeature( feature );

    const QgsGeometry g = feature.geometry();
    if ( g.isMultipart() )
    {
      for ( auto it = g.const_parts_begin(); it != g.const_parts_end(); ++it )
      {
        if ( !mProfileBoxEngine->intersects( *it ) )
          continue;

        processCurve( feature, qgsgeometry_cast< const QgsCurve * >( *it ) );
      }
    }
    else
    {
      processCurve( feature, qgsgeometry_cast< const QgsCurve * >( g.constGet() ) );
    }
  }
  return true;
}

bool QgsVectorLayerProfileGenerator::generateProfileForPolygons()
{
  // get features from layer
  QgsFeatureRequest request;
  request.setDestinationCrs( mTargetCrs, mTransformContext );
  request.setFilterRect( mProfileCurve->boundingBox() );
  request.setSubsetOfAttributes( mDataDefinedProperties.referencedFields( mExpressionContext ), mFields );
  request.setFeedback( mFeedback.get() );

  auto interpolatePointOnTriangle = []( const QgsPolygon * triangle, double x, double y ) -> QgsPoint
  {
    QgsPoint p1, p2, p3;
    Qgis::VertexType vt;
    triangle->exteriorRing()->pointAt( 0, p1, vt );
    triangle->exteriorRing()->pointAt( 1, p2, vt );
    triangle->exteriorRing()->pointAt( 2, p3, vt );
    const double z = QgsMeshLayerUtils::interpolateFromVerticesData( p1, p2, p3, p1.z(), p2.z(), p3.z(), QgsPointXY( x, y ) );
    return QgsPoint( x, y, z );
  };

  std::function< void( const QgsPolygon *triangle, const QgsAbstractGeometry *intersect, QVector< QgsGeometry > &, QVector< QgsGeometry > & ) > processTriangleLineIntersect;
  processTriangleLineIntersect = [this, &interpolatePointOnTriangle, &processTriangleLineIntersect]( const QgsPolygon * triangle, const QgsAbstractGeometry * intersect, QVector< QgsGeometry > &transformedParts, QVector< QgsGeometry > &crossSectionParts )
  {
    // intersect may be a (multi)point or (multi)linestring
    switch ( QgsWkbTypes::geometryType( intersect->wkbType() ) )
    {
      case QgsWkbTypes::PointGeometry:
        if ( const QgsMultiPoint *mp = qgsgeometry_cast< const QgsMultiPoint * >( intersect ) )
        {
          const int numPoint = mp->numGeometries();
          for ( int i = 0; i < numPoint; ++i )
          {
            processTriangleLineIntersect( triangle, mp->geometryN( i ), transformedParts, crossSectionParts );
          }
        }
        else if ( const QgsPoint *p = qgsgeometry_cast< const QgsPoint * >( intersect ) )
        {
          const QgsPoint interpolatedPoint = interpolatePointOnTriangle( triangle, p->x(), p->y() );
          mResults->mRawPoints.append( interpolatedPoint );
          mResults->minZ = std::min( mResults->minZ, interpolatedPoint.z() );
          mResults->maxZ = std::max( mResults->maxZ, interpolatedPoint.z() );

          QString lastError;
          const double distance = mProfileCurveEngine->lineLocatePoint( *p, &lastError );
          mResults->mDistanceToHeightMap.insert( distance, interpolatedPoint.z() );

          if ( mExtrusionEnabled )
          {
            const double extrusion = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ExtrusionHeight, mExpressionContext, mExtrusionHeight );

            transformedParts.append( QgsGeometry( new QgsLineString( interpolatedPoint,
                                                  QgsPoint( interpolatedPoint.x(), interpolatedPoint.y(), interpolatedPoint.z() + extrusion ) ) ) );
            crossSectionParts.append( QgsGeometry( new QgsLineString( QgsPoint( distance, interpolatedPoint.z() ),
                                                   QgsPoint( distance, interpolatedPoint.z() + extrusion ) ) ) );
            mResults->minZ = std::min( mResults->minZ, interpolatedPoint.z() + extrusion );
            mResults->maxZ = std::max( mResults->maxZ, interpolatedPoint.z() + extrusion );
          }
          else
          {
            transformedParts.append( QgsGeometry( new QgsPoint( interpolatedPoint ) ) );
            crossSectionParts.append( QgsGeometry( new QgsPoint( distance, interpolatedPoint.z() ) ) );
          }
        }
        break;
      case QgsWkbTypes::LineGeometry:
        if ( const QgsMultiLineString *ml = qgsgeometry_cast< const QgsMultiLineString * >( intersect ) )
        {
          const int numLines = ml->numGeometries();
          for ( int i = 0; i < numLines; ++i )
          {
            processTriangleLineIntersect( triangle, ml->geometryN( i ), transformedParts, crossSectionParts );
          }
        }
        else if ( const QgsLineString *ls = qgsgeometry_cast< const QgsLineString * >( intersect ) )
        {
          const int numPoints = ls->numPoints();
          QVector< double > newX;
          newX.resize( numPoints );
          QVector< double > newY;
          newY.resize( numPoints );
          QVector< double > newZ;
          newZ.resize( numPoints );
          QVector< double > newDistance;
          newDistance.resize( numPoints );

          const double *inX = ls->xData();
          const double *inY = ls->yData();
          double *outX = newX.data();
          double *outY = newY.data();
          double *outZ = newZ.data();
          double *outDistance = newDistance.data();

          QVector< double > extrudedZ;
          double *extZOut = nullptr;
          double extrusion = 0;
          if ( mExtrusionEnabled )
          {
            extrudedZ.resize( numPoints );
            extZOut = extrudedZ.data();

            extrusion = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ExtrusionHeight, mExpressionContext, mExtrusionHeight );
          }

          QString lastError;
          for ( int i = 0 ; i < numPoints; ++i )
          {
            double x = *inX++;
            double y = *inY++;

            QgsPoint interpolatedPoint = interpolatePointOnTriangle( triangle, x, y );
            *outX++ = x;
            *outY++ = y;
            *outZ++ = interpolatedPoint.z();
            if ( extZOut )
              *extZOut++ = interpolatedPoint.z() + extrusion;

            mResults->mRawPoints.append( interpolatedPoint );
            mResults->minZ = std::min( mResults->minZ, interpolatedPoint.z() );
            mResults->maxZ = std::max( mResults->maxZ, interpolatedPoint.z() );
            if ( mExtrusionEnabled )
            {
              mResults->minZ = std::min( mResults->minZ, interpolatedPoint.z() + extrusion );
              mResults->maxZ = std::max( mResults->maxZ, interpolatedPoint.z() + extrusion );
            }

            const double distance = mProfileCurveEngine->lineLocatePoint( interpolatedPoint, &lastError );
            *outDistance++ = distance;

            mResults->mDistanceToHeightMap.insert( distance, interpolatedPoint.z() );
          }

          if ( mExtrusionEnabled )
          {
            std::unique_ptr< QgsLineString > ring = std::make_unique< QgsLineString >( newX, newY, newZ );
            std::unique_ptr< QgsLineString > extrudedRing = std::make_unique< QgsLineString >( newX, newY, extrudedZ );
            std::unique_ptr< QgsLineString > reversedExtrusion( extrudedRing->reversed() );
            ring->append( reversedExtrusion.get() );
            ring->close();
            transformedParts.append( QgsGeometry( new QgsPolygon( ring.release() ) ) );


            std::unique_ptr< QgsLineString > distanceVHeightRing = std::make_unique< QgsLineString >( newDistance, newZ );
            std::unique_ptr< QgsLineString > extrudedDistanceVHeightRing = std::make_unique< QgsLineString >( newDistance, extrudedZ );
            std::unique_ptr< QgsLineString > reversedDistanceVHeightExtrusion( extrudedDistanceVHeightRing->reversed() );
            distanceVHeightRing->append( reversedDistanceVHeightExtrusion.get() );
            distanceVHeightRing->close();
            crossSectionParts.append( QgsGeometry( new QgsPolygon( distanceVHeightRing.release() ) ) );
          }
          else
          {
            transformedParts.append( QgsGeometry( new QgsLineString( newX, newY, newZ ) ) );
            crossSectionParts.append( QgsGeometry( new QgsLineString( newDistance, newZ ) ) );
          }
        }
        break;

      case QgsWkbTypes::PolygonGeometry:
      case QgsWkbTypes::UnknownGeometry:
      case QgsWkbTypes::NullGeometry:
        return;
    }
  };

  auto processPolygon = [this, &processTriangleLineIntersect]( const QgsCurvePolygon * polygon, QVector< QgsGeometry > &transformedParts, QVector< QgsGeometry > &crossSectionParts, double offset )
  {
    std::unique_ptr< QgsPolygon > clampedPolygon;
    if ( const QgsPolygon *p = qgsgeometry_cast< const QgsPolygon * >( polygon ) )
    {
      clampedPolygon.reset( p->clone() );
    }
    else
    {
      clampedPolygon.reset( qgsgeometry_cast< QgsPolygon * >( polygon->segmentize() ) );
    }
    clampAltitudes( clampedPolygon.get(), offset );

    if ( mFeedback->isCanceled() )
      return;

    const QgsRectangle bounds = clampedPolygon->boundingBox();
    QgsTessellator t( bounds, false, false, false, false );
    t.addPolygon( *clampedPolygon, 0 );

    QgsGeometry tessellation( t.asMultiPolygon() );
    if ( mFeedback->isCanceled() )
      return;

    tessellation.translate( bounds.xMinimum(), bounds.yMinimum() );

    // iterate through the tessellation, finding triangles which intersect the line
    const int numTriangles = qgsgeometry_cast< const QgsMultiPolygon * >( tessellation.constGet() )->numGeometries();
    for ( int i = 0; i < numTriangles; ++i )
    {
      if ( mFeedback->isCanceled() )
        return;

      const QgsPolygon *triangle = qgsgeometry_cast< const QgsPolygon * >( qgsgeometry_cast< const QgsMultiPolygon * >( tessellation.constGet() )->geometryN( i ) );
      if ( !mProfileCurveEngine->intersects( triangle ) )
        continue;

      QString error;
      std::unique_ptr< QgsAbstractGeometry > intersection( mProfileCurveEngine->intersection( triangle, &error ) );
      if ( !intersection )
        continue;

      processTriangleLineIntersect( triangle, intersection.get(), transformedParts, crossSectionParts );
    }
  };

  QgsFeature feature;
  QgsFeatureIterator it = mSource->getFeatures( request );
  while ( it.nextFeature( feature ) )
  {
    if ( mFeedback->isCanceled() )
      return false;

    if ( !mProfileCurveEngine->intersects( feature.geometry().constGet() ) )
      continue;

    mExpressionContext.setFeature( feature );

    const double offset = mDataDefinedProperties.valueAsDouble( QgsMapLayerElevationProperties::ZOffset, mExpressionContext, mOffset );

    const QgsGeometry g = feature.geometry();
    QVector< QgsGeometry > transformedParts;
    QVector< QgsGeometry > crossSectionParts;
    if ( g.isMultipart() )
    {
      for ( auto it = g.const_parts_begin(); it != g.const_parts_end(); ++it )
      {
        if ( mFeedback->isCanceled() )
          break;

        if ( !mProfileCurveEngine->intersects( *it ) )
          continue;

        processPolygon( qgsgeometry_cast< const QgsCurvePolygon * >( *it ), transformedParts, crossSectionParts, offset );
      }
    }
    else
    {
      processPolygon( qgsgeometry_cast< const QgsCurvePolygon * >( g.constGet() ), transformedParts, crossSectionParts, offset );
    }

    if ( mFeedback->isCanceled() )
      return false;

    QgsVectorLayerProfileResults::Feature resultFeature;
    resultFeature.featureId = feature.id();
    resultFeature.geometry = transformedParts.size() > 1 ? QgsGeometry::collectGeometry( transformedParts ) : transformedParts.value( 0 );
    if ( !crossSectionParts.empty() )
    {
      QgsGeometry unioned = QgsGeometry::unaryUnion( crossSectionParts );
      if ( unioned.type() == QgsWkbTypes::LineGeometry )
        unioned = unioned.mergeLines();
      resultFeature.crossSectionGeometry = unioned;
    }
    mResults->features[resultFeature.featureId].append( resultFeature );
  }
  return true;
}

double QgsVectorLayerProfileGenerator::terrainHeight( double x, double y )
{
  if ( !mTerrainProvider )
    return std::numeric_limits<double>::quiet_NaN();

  // transform feature point to terrain provider crs
  try
  {
    double dummyZ = 0;
    mTargetToTerrainProviderTransform.transformInPlace( x, y, dummyZ );
  }
  catch ( QgsCsException & )
  {
    return std::numeric_limits<double>::quiet_NaN();
  }

  return mTerrainProvider->heightAt( x, y );
}

double QgsVectorLayerProfileGenerator::featureZToHeight( double x, double y, double z, double offset )
{
  switch ( mClamping )
  {
    case Qgis::AltitudeClamping::Absolute:
      break;

    case Qgis::AltitudeClamping::Relative:
    case Qgis::AltitudeClamping::Terrain:
    {
      const double terrainZ = terrainHeight( x, y );
      if ( !std::isnan( terrainZ ) )
      {
        switch ( mClamping )
        {
          case Qgis::AltitudeClamping::Relative:
            if ( std::isnan( z ) )
              z = terrainZ;
            else
              z += terrainZ;
            break;

          case Qgis::AltitudeClamping::Terrain:
            z = terrainZ;
            break;

          case Qgis::AltitudeClamping::Absolute:
            break;
        }
      }
      break;
    }
  }

  return ( std::isnan( z ) ? 0 : z ) * mScale + offset;
}

void QgsVectorLayerProfileGenerator::clampAltitudes( QgsLineString *lineString, const QgsPoint &centroid, double offset )
{
  for ( int i = 0; i < lineString->nCoordinates(); ++i )
  {
    if ( mFeedback->isCanceled() )
      break;

    double terrainZ = 0;
    switch ( mClamping )
    {
      case Qgis::AltitudeClamping::Relative:
      case Qgis::AltitudeClamping::Terrain:
      {
        QgsPointXY pt;
        switch ( mBinding )
        {
          case Qgis::AltitudeBinding::Vertex:
            pt.setX( lineString->xAt( i ) );
            pt.setY( lineString->yAt( i ) );
            break;

          case Qgis::AltitudeBinding::Centroid:
            pt.set( centroid.x(), centroid.y() );
            break;
        }

        terrainZ = terrainHeight( pt.x(), pt.y() );
        break;
      }

      case Qgis::AltitudeClamping::Absolute:
        break;
    }

    double geomZ = 0;

    switch ( mClamping )
    {
      case Qgis::AltitudeClamping::Absolute:
      case Qgis::AltitudeClamping::Relative:
        geomZ = lineString->zAt( i );
        break;

      case Qgis::AltitudeClamping::Terrain:
        break;
    }

    const double z = ( terrainZ + ( std::isnan( geomZ ) ?  0 : geomZ ) ) * mScale + offset;
    lineString->setZAt( i, z );
  }
}

bool QgsVectorLayerProfileGenerator::clampAltitudes( QgsPolygon *polygon, double offset )
{
  if ( !polygon->is3D() )
    polygon->addZValue( 0 );

  QgsPoint centroid;
  switch ( mBinding )
  {
    case Qgis::AltitudeBinding::Vertex:
      break;

    case Qgis::AltitudeBinding::Centroid:
      centroid = polygon->centroid();
      break;
  }

  QgsCurve *curve = const_cast<QgsCurve *>( polygon->exteriorRing() );
  QgsLineString *lineString = qgsgeometry_cast<QgsLineString *>( curve );
  if ( !lineString )
    return false;

  clampAltitudes( lineString, centroid, offset );

  for ( int i = 0; i < polygon->numInteriorRings(); ++i )
  {
    if ( mFeedback->isCanceled() )
      break;

    QgsCurve *curve = const_cast<QgsCurve *>( polygon->interiorRing( i ) );
    QgsLineString *lineString = qgsgeometry_cast<QgsLineString *>( curve );
    if ( !lineString )
      return false;

    clampAltitudes( lineString, centroid, offset );
  }
  return true;
}

