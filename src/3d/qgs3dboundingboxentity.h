/***************************************************************************
  qgs3dboundingbox.h
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

#ifndef QGS3DBOUNDINGBOXENTITY_H
#define QGS3DBOUNDINGBOXENTITY_H

#include "qgis_3d.h"

namespace Qt3DCore
{
  class QEntity;
}

class Qgs3DWiredMesh;
class QgsAABB;

#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief Entity to display a bounding box around the scene
 * \since QGIS 3.26
 */
class _3D_EXPORT Qgs3DBoundingBoxEntity: public Qt3DCore::QEntity
{
    Q_OBJECT

  public:
    Qgs3DBoundingBoxEntity( Qt3DCore::QNode *parent = nullptr );
    void setBox( const QgsAABB &box );

  private:
    Qgs3DWiredMesh *mBBMesh = nullptr;
    QColor mColor = Qt::black;
};

#endif // QGS3DBOUNDINGBOXENTITY_H
