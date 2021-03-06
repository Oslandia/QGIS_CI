/***************************************************************************
                              qgsselectioncontext.cpp
                             --------------------------
    begin                : June 2022
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

#include "qgsselectioncontext.h"

void QgsSelectionContext::setScale( double scale )
{
  mScale = scale;
}

double QgsSelectionContext::scale() const
{
  return mScale;
}
