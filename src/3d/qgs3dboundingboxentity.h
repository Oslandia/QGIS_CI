/***************************************************************************
  qgs3dboundingbox.h
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

#ifndef QGS3DBOUNDINGBOX_H
#define QGS3DBOUNDINGBOX_H

namespace Qt3DCore
{
  class QEntity;
}

class AABBMesh;
class QgsAABB;

#define SIP_NO_FILE

class Qgs3DBoundingBoxEntity: public Qt3DCore::QEntity
{
    Q_OBJECT

  public:
    Qgs3DBoundingBoxEntity( Qt3DCore::QNode *parent = nullptr );
    void setBox( const QgsAABB &box );

  private:
    AABBMesh *mAabbMesh = nullptr;
};

#endif // QGS3DBOUNDINGBOX_H
