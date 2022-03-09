/***************************************************************************
  qgs3dboundingbox.cpp
  --------------------------------------
  Date                 : April 2022
  Copyright            : (C) 2022 by Jean Felder
  Email                : jean dot felder at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <Qt3DCore/QEntity>
#include <Qt3DExtras/QPhongMaterial>

#include "qgs3dboundingboxentity.h"
#include "qgs3dwiredmesh.h"
#include "qgsaabb.h"

Qgs3DBoundingBoxEntity::Qgs3DBoundingBoxEntity( Qt3DCore::QNode *parent )
  : Qt3DCore::QEntity( parent )
{
  mBBMesh = new Qgs3DWiredMesh( this );
  addComponent( mBBMesh );

  Qt3DExtras::QPhongMaterial *bboxMaterial = new Qt3DExtras::QPhongMaterial;
  bboxMaterial->setAmbient( mColor );
  addComponent( bboxMaterial );
}

void Qgs3DBoundingBoxEntity::setBox( const QgsAABB &bbox )
{
  mBBMesh->setVertices( bbox.verticesForLines() );
}
