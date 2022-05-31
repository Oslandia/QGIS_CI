/***************************************************************************
  qgs3daxissettings.cpp
  --------------------------------------
  Date                 : April 2022
  copyright            : (C) 2021 B. De Mezzo
  email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3daxissettings.h"

#include <QDomDocument>

#include "qgsreadwritecontext.h"
#include "qgssymbollayerutils.h"

Qgs3DAxisSettings::Qgs3DAxisSettings( const Qgs3DAxisSettings &other )
  : mMode( other.mMode )
  , mHorizontalPosition( other.mHorizontalPosition )
  , mVerticalPosition( other.mVerticalPosition )
{

}

Qgs3DAxisSettings &Qgs3DAxisSettings::operator=( Qgs3DAxisSettings const &rhs )
{
  this->mMode = rhs.mMode;
  this->mHorizontalPosition = rhs.mHorizontalPosition;
  this->mVerticalPosition = rhs.mVerticalPosition;
  return *this;
}

bool Qgs3DAxisSettings::operator==( Qgs3DAxisSettings const &rhs ) const
{
  bool out = true;
  out &= this->mMode == rhs.mMode;
  out &= this->mHorizontalPosition == rhs.mHorizontalPosition;
  out &= this->mVerticalPosition == rhs.mVerticalPosition;
  return out;
}

bool Qgs3DAxisSettings::operator!=( Qgs3DAxisSettings const &rhs ) const
{
  return ! this->operator==( rhs );
}

void Qgs3DAxisSettings::readXml( const QDomElement &element, const QgsReadWriteContext & )
{
  const QString modeStr = element.attribute( QStringLiteral( "mode" ) );
  if ( modeStr == QLatin1String( "Off" ) )
    mMode = Qgs3DAxis::Mode::Off;
  else if ( modeStr == QLatin1String( "Crs" ) )
    mMode = Qgs3DAxis::Mode::Crs;
  else if ( modeStr == QLatin1String( "Cube" ) )
    mMode = Qgs3DAxis::Mode::Cube;

  const QString horizontalStr = element.attribute( QStringLiteral( "horizontal" ) );
  if ( horizontalStr == QLatin1String( "Begin" ) )
    mHorizontalPosition = Qgs3DAxisRenderView::AxisViewportPosition::Begin;
  else if ( horizontalStr == QLatin1String( "Middle" ) )
    mHorizontalPosition = Qgs3DAxisRenderView::AxisViewportPosition::Middle;
  else if ( horizontalStr == QLatin1String( "End" ) )
    mHorizontalPosition = Qgs3DAxisRenderView::AxisViewportPosition::End;

  const QString verticalStr = element.attribute( QStringLiteral( "vertical" ) );
  if ( verticalStr == QLatin1String( "Begin" ) )
    mVerticalPosition = Qgs3DAxisRenderView::AxisViewportPosition::Begin;
  else if ( verticalStr == QLatin1String( "Middle" ) )
    mVerticalPosition = Qgs3DAxisRenderView::AxisViewportPosition::Middle;
  else if ( verticalStr == QLatin1String( "End" ) )
    mVerticalPosition = Qgs3DAxisRenderView::AxisViewportPosition::End;
}

void Qgs3DAxisSettings::writeXml( QDomElement &element, const QgsReadWriteContext & ) const
{
  QString str;
  switch ( mMode )
  {
    case Qgs3DAxis::Mode::Crs:
      str = QLatin1String( "Crs" );
      break;
    case Qgs3DAxis::Mode::Cube:
      str = QLatin1String( "Cube" );
      break;

    case Qgs3DAxis::Mode::Off:
    default:
      str = QLatin1String( "Off" );
      break;
  }
  element.setAttribute( QStringLiteral( "mode" ), str );

  switch ( mHorizontalPosition )
  {
    case Qgs3DAxisRenderView::AxisViewportPosition::Begin:
      str = QLatin1String( "Begin" );
      break;
    case Qgs3DAxisRenderView::AxisViewportPosition::Middle:
      str = QLatin1String( "Middle" );
      break;
    case Qgs3DAxisRenderView::AxisViewportPosition::End:
    default:
      str = QLatin1String( "End" );
      break;
  }
  element.setAttribute( QStringLiteral( "horizontal" ), str );

  switch ( mVerticalPosition )
  {
    case Qgs3DAxisRenderView::AxisViewportPosition::Begin:
      str = QLatin1String( "Begin" );
      break;
    case Qgs3DAxisRenderView::AxisViewportPosition::Middle:
      str = QLatin1String( "Middle" );
      break;
    case Qgs3DAxisRenderView::AxisViewportPosition::End:
    default:
      str = QLatin1String( "End" );
      break;
  }
  element.setAttribute( QStringLiteral( "vertical" ), str );

}
