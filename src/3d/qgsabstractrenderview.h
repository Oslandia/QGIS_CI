/***************************************************************************
  qgsabstractrenderview.h
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

#ifndef QGSABSTRACTRENDERVIEW_H
#define QGSABSTRACTRENDERVIEW_H

#include "qgis_3d.h"

#include <QObject>

#define SIP_NO_FILE

class QColor;
class QRect;
class QSurface;

namespace Qt3DCore
{
  class QEntity;
}

namespace Qt3DRender
{
  class QRenderSettings;
  class QCamera;
  class QFrameGraphNode;
  class QLayer;
}

class QgsShadowRenderingFrameGraph;

/**
 * \ingroup 3d
 * \brief Base class for 3D render view
 *
 * \note Not available in Python bindings
 * \since QGIS 3.26
 */
class _3D_EXPORT QgsAbstractRenderView : public QObject
{
    Q_OBJECT
  public:

    /**
     * Constructor for QgsAbstractRenderView with the specified \a parent object.
     */
    QgsAbstractRenderView( QObject *parent = nullptr );

    //! Sets root entity of the 3D scene
    virtual void setRootEntity( Qt3DCore::QEntity *root ) = 0;

    virtual Qt3DRender::QLayer *layerToFilter() = 0;

    virtual Qt3DRender::QFrameGraphNode *topGraphNode() = 0;

    virtual enableSubTree( bool enable ) = 0;

    /**
     * Returns the shadow rendering frame graph object used to render the scene
     *
     * \since QGIS 3.18
     */
    QgsShadowRenderingFrameGraph *frameGraph() { return mFrameGraph; }

  protected:
    QgsShadowRenderingFrameGraph *mFrameGraph = nullptr;
};


#endif // QGSABSTRACTRENDERVIEW_H
