/***************************************************************************
  qgs3dboundingbox.cpp
  --------------------------------------
  Date                 : March 2022
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

#include "qgsaabb.h"
#include "qgschunkboundsentity_p.h"
#include "qgs3dboundingboxentity.h"

Qgs3DBoundingBoxEntity::Qgs3DBoundingBoxEntity( Qt3DCore::QNode *parent )
  : Qt3DCore::QEntity( parent )
{
  mAabbMesh = new AABBMesh;
  addComponent( mAabbMesh );

  Qt3DExtras::QPhongMaterial *bboxMaterial = new Qt3DExtras::QPhongMaterial;
  bboxMaterial->setAmbient( Qt::yellow );
  addComponent( bboxMaterial );
}

void Qgs3DBoundingBoxEntity::setBox( const QgsAABB &bbox )
{
  QList<QgsAABB> bboxes;
  bboxes << bbox;
  mAabbMesh->setBoxes( bboxes );
}
