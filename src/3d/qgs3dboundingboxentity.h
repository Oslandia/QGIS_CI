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
#include "qgsaabb.h"

namespace Qt3DCore
{
  class QEntity;
}

namespace Qt3DExtras
{
  class QText2DEntity;
}

namespace Qt3DRender
{
  class QCamera;
}

class Qgs3DBillboardLabel;
class Qgs3DBoundingBoxSettings;
class Qgs3DMapSettings;
class Qgs3DWiredMesh;
class QgsCameraController;
class QgsVector3D;

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
    Qgs3DBoundingBoxEntity( Qt3DCore::QEntity *parent, Qgs3DMapSettings *mapSettings, QgsCameraController *cameraCtrl );
    virtual ~Qgs3DBoundingBoxEntity();
    void setParameters( Qgs3DBoundingBoxSettings const &settings );

  private slots:
    void onCameraChanged();

  private:
    void updateVisibleFaces();
    void computeLabelsRanges( QVector3D &rangeMin, QVector3D &rangeMax, QVector3D &rangeIncr );
    void clearLabels();
    void createLabelsForFullBoundingBox( Qt::Axis axis, const QVector3D &rangeMin, const QVector3D &rangeMax, const QVector3D &rangeIncr, QList<QVector3D> &vertices );
    void createLabelsForPartialBoundingBox( const QVector3D &rangeMin, const QVector3D &rangeMax, const QVector3D &rangeIncr, QList<QVector3D> &vertices );

    QList<QVector3D> computeBoundingBoxVertices();

    Qgs3DMapSettings *mMapSettings = nullptr;

    Qgs3DBoundingBoxSettings *mSettings = nullptr;
    Qgs3DWiredMesh *mBBMesh = nullptr;
    // QList<Qt3DExtras::QText2DEntity *> mLabels;
    QList<Qgs3DBillboardLabel *> mLabels;
    QFont mLabelsFont;
    QList<QgsAABB::Face> mVisibleFaces;
    QVector3D mInitialCameraPosition = QVector3D( 0, 0, 0 );
    QgsCameraController *mCameraCtrl = nullptr;

    QMetaObject::Connection mCameraConnection;

};

#endif // QGS3DBOUNDINGBOXENTITY_H
