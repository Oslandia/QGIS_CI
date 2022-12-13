/***************************************************************************
  qgsdebugtextureentity.cpp
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

#include "qgsdebugtextureentity.h"
#include "qgspreviewquad.h"
#include <Qt3DRender/QTexture>

#include <Qt3DRender/QParameter>
#include <QUrl>

#include "qgsshadowrenderingframegraph.h"
#include "qgsabstractrenderview.h"

QgsDebugTextureEntity::QgsDebugTextureEntity( QgsShadowRenderingFrameGraph *frameGraph, Qt3DRender::QTexture2D *texture )
  : QgsPreviewQuad( texture
                    , QPointF( 0.9f, 0.9f )
                    , QSizeF( 0.1, 0.1 )
  ,  QVector<Qt3DRender::QParameter *> {new Qt3DRender::QParameter( "isDepth", true )} )
{
  setObjectName( "DebugTextureQuad" );
  setParent( frameGraph->rootEntity() );
  setEnabled( false );

  QgsAbstractRenderView *drv = frameGraph->renderView( QgsShadowRenderingFrameGraph::DEBUG_RENDERVIEW );
  addComponent( drv->layerToFilter() );
}

void QgsDebugTextureEntity::onSettingsChanged( bool enabled, Qt::Corner corner, double size )
{
  setEnabled( enabled );
  if ( enabled )
  {
    switch ( corner )
    {
      case Qt::Corner::TopRightCorner:
        setViewPort( QPointF( 1.0f - size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::TopLeftCorner:
        setViewPort( QPointF( 0.0f + size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomRightCorner:
        setViewPort( QPointF( 1.0f - size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomLeftCorner:
        setViewPort( QPointF( 0.0f + size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
    }
  }
}
