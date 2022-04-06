/***************************************************************************
                         qgs3dwiredmesh.h
                         -------------------------
    begin                : april 2022
    copyright            : (C) 2022 by Benoît De Mezzo
    email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DWIREDMESH_H
#define QGS3DWIREDMESH_H

#include <QVector3D>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QGeometryRenderer>

#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief Geometry renderer for lines, draws a wired mesh
 *
 * \since QGIS 3.26
 */
class Qgs3DWiredMesh : public Qt3DRender::QGeometryRenderer
{
    Q_OBJECT

  public:

    /**
     * \brief Defaul Qgs3DWiredMesh constructor
     */
    Qgs3DWiredMesh( Qt3DCore::QNode *parent = nullptr );
    ~Qgs3DWiredMesh() override;

    /**
     * \brief add or replace mesh vertices coordinates
     */
    void setVertices( const QList<QVector3D> &vertices );

  private:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Qt3DRender::QGeometry *mGeom = nullptr;
    Qt3DRender::QAttribute *mPositionAttribute = nullptr;
    Qt3DRender::QBuffer *mVertexBuffer = nullptr;
#else
    Qt3DCore::QGeometry *mGeom = nullptr;
    Qt3DCore::QAttribute *mPositionAttribute = nullptr;
    Qt3DCore::QBuffer *mVertexBuffer = nullptr;
#endif
};

#endif // QGS3DWIREDMESH_H
