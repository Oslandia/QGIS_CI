/***************************************************************************
  qgs3daxisrenderview.h
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

#ifndef QGS3DAXISRENDERVIEW_H
#define QGS3DAXISRENDERVIEW_H

#include "qgis_3d.h"
#include "qgsabstractrenderview.h"
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
  class QViewport;
  class QSubtreeEnabler;
}

namespace Qt3DExtras
{
  class Qt3DWindow;
}

class QgsShadowRenderingFrameGraph;
class Qgs3DMapSettings;

/**
 * \ingroup 3d
 * \brief Base class for 3D render view
 *
 * \note Not available in Python bindings
 * \since QGIS 3.26
 */
class _3D_EXPORT Qgs3DAxisRenderView : public QgsAbstractRenderView
{
    Q_OBJECT
  public:

    /**
     * \brief The AxisViewportPosition enum
     */
    enum class AxisViewportPosition
    {
      //! top or left
      Begin = 1,
      Middle = 2,
      //! bottom or right
      End = 3
    };
    Q_ENUM( AxisViewportPosition )

    /**
     * Constructor for Qgs3DAxisRenderView with the specified \a parent object.
     */
    Qgs3DAxisRenderView( QObject *parent, Qt3DExtras::Qt3DWindow *parentWindow,
                         Qt3DRender::QCamera *axisCamera, Qgs3DMapSettings &settings );

    //! Sets root entity of the 3D scene
    virtual void setRootEntity( Qt3DCore::QEntity *root ) ;

    //! Returns the layer to be used by entities to be included in this renderview
    virtual Qt3DRender::QLayer *layerToFilter();

    //! Returns the viewport associated to this renderview
    virtual Qt3DRender::QViewport *viewport();

    //! Returns the top node of this renderview branch. Should be a QSubtreeEnabler
    virtual Qt3DRender::QFrameGraphNode *topGraphNode();

    //! Enable or disable via \a enable the renderview sub tree
    virtual void enableSubTree( bool enable );

    //! Return viewport side length (as it is a square)
    int axisViewportSize() const { return mAxisViewportSize; }

    //! Returns axis viewport horizontal position
    AxisViewportPosition axisViewportHorizontalPosition() const { return mAxisViewportHorizPos;}

    //! Returns axis viewport vertical position
    AxisViewportPosition axisViewportVerticalPosition() const { return mAxisViewportVertPos;}

    //! Updates viewport \a size, horizontal position (\a horizPos) and vertical position (\a vertPos).
    void setAxisViewportPosition( int size,
                                  AxisViewportPosition vertPos,
                                  AxisViewportPosition horizPos );

  public slots:
    //! Updates viewport horizontal \a position

    void onAxisHorizPositionChanged( AxisViewportPosition position );

    //! Updates viewport vertical \a position
    void onAxisVertPositionChanged( AxisViewportPosition position );

    //! Updates viewport \a size
    void onAxisViewportSizeUpdate( int size = 0 );

  private:
    Qt3DExtras::Qt3DWindow *mParentWindow;
    Qt3DRender::QCamera *mAxisCamera;
    Qt3DRender::QLayer *mAxisSceneLayer = nullptr;
    Qt3DRender::QViewport *mAxisViewport = nullptr;
    Qt3DRender::QSubtreeEnabler *mShadowRendererEnabler = nullptr;
    Qgs3DMapSettings &mMapSettings;

    int mAxisViewportSize = 160;
    AxisViewportPosition mAxisViewportVertPos = AxisViewportPosition::Begin;
    AxisViewportPosition mAxisViewportHorizPos = AxisViewportPosition::End;

};


#endif // QGS3DAXISRENDERVIEW_H
