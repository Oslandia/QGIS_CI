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
#include "qgsaabb.h"

Qgs3DBoundingBoxSettings::Qgs3DBoundingBoxSettings( const Qgs3DBoundingBoxSettings &other )
  : mEnabled( other.mEnabled )
  , mBoundingBox( other.mBoundingBox )
{

}

Qgs3DBoundingBoxSettings::Qgs3DBoundingBoxSettings( bool enabled, const QgsAABB &boundingBox )
  : mEnabled( enabled )
  , mBoundingBox( boundingBox )
{

}

Qgs3DBoundingBoxSettings &Qgs3DBoundingBoxSettings::operator=( Qgs3DBoundingBoxSettings const &rhs )
{
  this->mEnabled = rhs.mEnabled;
  this->mBoundingBox = rhs.mBoundingBox;
  return *this;
}

bool Qgs3DBoundingBoxSettings::operator==( Qgs3DBoundingBoxSettings const &rhs ) const
{
  bool out = true;
  out &= this->mEnabled == rhs.mEnabled;
  out &= this->mBoundingBox == rhs.mBoundingBox;
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
}

QDomElement Qgs3DBoundingBoxSettings::writeXml( QDomElement &element, const QgsReadWriteContext &context ) const
{
  Q_UNUSED( context )
  if ( element.isNull() )
    return QDomElement();

  QString enabled = mEnabled ? QStringLiteral( "1" ) : QStringLiteral( "0" );
  element.setAttribute( QStringLiteral( "enabled" ), enabled );
  element.setAttribute( QStringLiteral( "extent" ), mBoundingBox.toString() );
  return element;
}
