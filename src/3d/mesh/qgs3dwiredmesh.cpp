/***************************************************************************
                         qgs3dwiredmesh.cpp
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

#include "qgs3dwiredmesh.h"

Qgs3DWiredMesh::Qgs3DWiredMesh( Qt3DCore::QNode *parent )
  : Qt3DRender::QGeometryRenderer( parent ),
    mPositionAttribute( new Qt3DRender::QAttribute( this ) ),
    mVertexBuffer( new Qt3DRender::QBuffer( this ) )
{
  mPositionAttribute->setAttributeType( Qt3DRender::QAttribute::VertexAttribute );
  mPositionAttribute->setBuffer( mVertexBuffer );
  mPositionAttribute->setVertexBaseType( Qt3DRender::QAttribute::Float );
  mPositionAttribute->setVertexSize( 3 );
  mPositionAttribute->setName( Qt3DRender::QAttribute::defaultPositionAttributeName() );

  mGeom = new Qt3DRender::QGeometry( this );
  mGeom->addAttribute( mPositionAttribute );

  setInstanceCount( 1 );
  setIndexOffset( 0 );
  setFirstInstance( 0 );
  setPrimitiveType( Qt3DRender::QGeometryRenderer::Lines );
  setGeometry( mGeom );
}

Qgs3DWiredMesh::~Qgs3DWiredMesh() = default;

void Qgs3DWiredMesh::setVertices( const QList<QVector3D> &vertices )
{
  QByteArray vertexBufferData;
  vertexBufferData.resize( vertices.size() * 3 * sizeof( float ) );
  float *rawVertexArray = reinterpret_cast<float *>( vertexBufferData.data() );
  int idx = 0;
  for ( const QVector3D &v : std::as_const( vertices ) )
  {
    rawVertexArray[idx++] = v.x();
    rawVertexArray[idx++] = v.y();
    rawVertexArray[idx++] = v.z();
  }

  mVertexBuffer->setData( vertexBufferData );
  setVertexCount( vertices.count() );
}
