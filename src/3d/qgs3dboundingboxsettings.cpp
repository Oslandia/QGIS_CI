/***************************************************************************
  qgs3dboundingboxsettings.cpp
  --------------------------------------
  Date                 : May 2022
  copyright            : (C) 2022 Jean Felder
  email                : jean dot felder at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3dboundingboxsettings.h"

#include <QDomDocument>

#include "qgsreadwritecontext.h"
#include "qgssymbollayerutils.h"
#include "qgsaabb.h"

Qgs3DBoundingBoxSettings::Qgs3DBoundingBoxSettings( const Qgs3DBoundingBoxSettings &other )
  : mEnabled( other.mEnabled )
  , mBoundingBox( other.mBoundingBox )
  , mNrTicks( other.mNrTicks )
  , mColor( other.mColor )
  , mFull( other.mFull )
  , mShowIn2DView( other.mShowIn2DView )
{

}

Qgs3DBoundingBoxSettings::Qgs3DBoundingBoxSettings( bool enabled, const QgsAABB &boundingBox, int nrTicks, QColor color, bool full, bool showIn2DView )
  : mEnabled( enabled )
  , mBoundingBox( boundingBox )
  , mNrTicks( nrTicks )
  , mColor( color )
  , mFull( full )
  , mShowIn2DView( showIn2DView )
{

}

Qgs3DBoundingBoxSettings &Qgs3DBoundingBoxSettings::operator=( Qgs3DBoundingBoxSettings const &rhs )
{
  this->mEnabled = rhs.mEnabled;
  this->mBoundingBox = rhs.mBoundingBox;
  this->mNrTicks = rhs.mNrTicks;
  this->mColor = rhs.mColor;
  this->mFull = rhs.mFull;
  this->mShowIn2DView = rhs.mShowIn2DView;
  return *this;
}

bool Qgs3DBoundingBoxSettings::operator==( Qgs3DBoundingBoxSettings const &rhs ) const
{
  bool out = true;
  out &= this->mEnabled == rhs.mEnabled;
  out &= this->mBoundingBox == rhs.mBoundingBox;
  out &= this->mNrTicks == rhs.mNrTicks;
  out &= this->mColor == rhs.mColor;
  out &= this->mFull == rhs.mFull;
  out &= this->mShowIn2DView == rhs.mShowIn2DView;
  return out;
}

bool Qgs3DBoundingBoxSettings::operator!=( Qgs3DBoundingBoxSettings const &rhs ) const
{
  return ! this->operator==( rhs );
}

void Qgs3DBoundingBoxSettings::readXml( const QDomElement &element, const QgsReadWriteContext & )
{
  const QString enabled = element.attribute( QStringLiteral( "enabled" ), QStringLiteral( "0" ) );
  mEnabled = enabled.toInt() ? true : false;

  const QString extentString = element.attribute( QStringLiteral( "extent" ) );
  mBoundingBox = QgsAABB::fromString( extentString );

  const QString nrTicks = element.attribute( QStringLiteral( "nr-ticks" ), QString::number( 7 ) );
  mNrTicks = nrTicks.toInt();

  mColor = QgsSymbolLayerUtils::decodeColor( element.attribute( QStringLiteral( "color" ), QStringLiteral( "0,0,0,255" ) ) );

  const QString full = element.attribute( QStringLiteral( "full" ), QStringLiteral( "1" ) );
  mFull = full.toInt() ? true : false;

  const QString showIn2DView = element.attribute( QStringLiteral( "show-in-2d-view" ), QStringLiteral( "0" ) );
  mShowIn2DView = showIn2DView.toInt() ? true : false;
}

QDomElement Qgs3DBoundingBoxSettings::writeXml( QDomElement &element, const QgsReadWriteContext &context ) const
{
  Q_UNUSED( context )
  if ( element.isNull() )
    return QDomElement();

  QString enabled = mEnabled ? QStringLiteral( "1" ) : QStringLiteral( "0" );
  element.setAttribute( QStringLiteral( "enabled" ), enabled );
  element.setAttribute( QStringLiteral( "extent" ), mBoundingBox.toString() );
  element.setAttribute( QStringLiteral( "nr-ticks" ),  QString::number( mNrTicks ) );
  element.setAttribute( QStringLiteral( "color" ), QgsSymbolLayerUtils::encodeColor( mColor ) );
  QString full = mFull ? QStringLiteral( "1" ) : QStringLiteral( "0" );
  element.setAttribute( QStringLiteral( "full" ), full );
  QString showIn2DView = mShowIn2DView ? QStringLiteral( "1" ) : QStringLiteral( "0" );
  element.setAttribute( QStringLiteral( "show-in-2d-view" ),  showIn2DView );
  return element;
}
