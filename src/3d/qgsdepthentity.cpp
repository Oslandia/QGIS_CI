/***************************************************************************
  qgsdepthentity.cpp
  --------------------------------------
  Date                 : December 2022
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdepthentity.h"

#include <Qt3DRender/QParameter>
#include <QUrl>

#include "qgsshadowrenderingframegraph.h"
#include "qgsabstractrenderview.h"

QgsDepthEntity::QgsDepthEntity( Qt3DRender::QTexture2D *texture, QNode *parent )
  : QgsRenderPassQuad( parent )
{
  setObjectName( "depthRenderQuad" );

  QMatrix4x4 modelMatrix;
  modelMatrix.setToIdentity();

  // construct material

  mTextureParameter = new Qt3DRender::QParameter( "depthTexture", texture );
  mTextureTransformParameter = new Qt3DRender::QParameter( "modelMatrix", QVariant::fromValue( modelMatrix ) );
  mMaterial->addParameter( mTextureParameter );
  mMaterial->addParameter( mTextureTransformParameter );

  const QString vertexShaderPath = QStringLiteral( "qrc:/shaders/depth_render.vert" );
  const QString fragmentShaderPath = QStringLiteral( "qrc:/shaders/depth_render.frag" );

  mShader->setVertexShaderCode( Qt3DRender::QShaderProgram::loadSource( QUrl( vertexShaderPath ) ) );
  mShader->setFragmentShaderCode( Qt3DRender::QShaderProgram::loadSource( QUrl( fragmentShaderPath ) ) );
}
