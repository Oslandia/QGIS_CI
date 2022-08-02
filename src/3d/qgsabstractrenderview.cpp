/***************************************************************************
  qgsabstractrenderview.cpp
  --------------------------------------
  Date                 : May 2022
  Copyright            : (C) 2022 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsabstractrenderview.h"

#include "qgsshadowrenderingframegraph.h"

QgsAbstractRenderView::QgsAbstractRenderView( QObject *parent )
  : QObject( parent )
{

}

void QgsAbstractRenderView::setTargetOutputs( const QList<Qt3DRender::QRenderTargetOutput *> &targetOutputList )
{
  mTargetOutputs = targetOutputList;
  onTargetOutputUpdate();
}

void QgsAbstractRenderView::updateTargetOutputSize( int width, int height )
{
  for ( Qt3DRender::QRenderTargetOutput *targetOutput : qAsConst( mTargetOutputs ) )
    targetOutput->texture()->setSize( width, height );
}

Qt3DRender::QTexture2D *QgsAbstractRenderView::outputTexture( Qt3DRender::QRenderTargetOutput::AttachmentPoint attachment )
{
  for ( Qt3DRender::QRenderTargetOutput *targetOutput : qAsConst( mTargetOutputs ) )
    if ( targetOutput->attachmentPoint() == attachment )
      return ( Qt3DRender::QTexture2D * )targetOutput->texture();

  return nullptr;
}
