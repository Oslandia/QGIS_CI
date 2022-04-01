/***************************************************************************
                          qgselevationprofilecanvas.h
                          ---------------
    begin                : March 2022
    copyright            : (C) 2022 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSELEVATIONPROFILECANVAS_H
#define QGSELEVATIONPROFILECANVAS_H

#include "qgsconfig.h"
#include "qgis_sip.h"
#include "qgis_gui.h"
#include "qgsplotcanvas.h"
#include "qgsmaplayer.h"
#include "qgscoordinatereferencesystem.h"

class QgsElevationProfilePlotItem;
class QgsAbstractProfileResults;
class QgsProfilePlotRenderer;
class QgsCurve;

/**
 * \ingroup gui
 * \brief A canvas for elevation profiles.
 *
 * \since QGIS 3.26
 */
class GUI_EXPORT QgsElevationProfileCanvas : public QgsPlotCanvas
{

    Q_OBJECT

  public:

    /**
     * Constructor for QgsElevationProfileCanvas, with the specified \a parent widget.
     */
    QgsElevationProfileCanvas( QWidget *parent SIP_TRANSFERTHIS = nullptr );

    ~QgsElevationProfileCanvas() override;
    QgsCoordinateReferenceSystem crs() const override;
    QgsPoint toMapCoordinates( const QgsPointXY &point ) const override;
    QgsPointXY toCanvasCoordinates( const QgsPoint &point ) const override;
    void resizeEvent( QResizeEvent *event ) override;
    void showEvent( QShowEvent *event ) override;
    void cancelJobs() override SIP_SKIP;

    /**
     * Triggers an update of the profile, causing the profile extraction to perform in the
     * background.
     */
    void refresh() override;

    /**
     * Sets the \a project associated with the profile.
     *
     * This must be set before any layers which utilize terrain based elevation settings can be
     * included in the canvas.
     */
    void setProject( QgsProject *project );

    /**
     * Sets the list of \a layers to include in the profile.
     *
     * \see layers()
     */
    void setLayers( const QList< QgsMapLayer * > &layers );

    /**
     * Returns the list of layers included in the profile.
     *
     * \see setLayers()
     */
    QList< QgsMapLayer * > layers() const;

    /**
     * Sets the \a crs associated with the canvas' map coordinates.
     *
     * \see crs()
     */
    void setCrs( const QgsCoordinateReferenceSystem &crs );

    /**
     * Sets the profile \a curve.
     *
     * The CRS associated with \a curve is set via setCrs().
     *
     * Ownership is transferred to the plot canvas.
     *
     * \see profileCurve()
     */
    void setProfileCurve( QgsCurve *curve SIP_TRANSFER );

    /**
     * Returns the profile curve.
     *
     * The CRS associated with the curve is retrieved via crs().
     *
     * \see setProfileCurve()
     */
    QgsCurve *profileCurve() const;

    /**
     * Sets the visible area of the plot.
     */
    void setVisiblePlotRange( double minimumDistance, double maximumDistance, double minimumElevation, double maximumElevation );

  signals:

    /**
     * Emitted when the number of active background jobs changes.
     */
    void activeJobCountChanged( int count );

  public slots:

    /**
     * Zooms to the full extent of the profile.
     */
    void zoomFull();

    /**
     * Clears the current profile.
     */
    void clear();

  private slots:

    void generationFinished();

  private:

    QgsCoordinateReferenceSystem mCrs;
    QgsProject *mProject = nullptr;

    QgsWeakMapLayerPointerList mLayers;

    QgsElevationProfilePlotItem *mPlotItem = nullptr;

    QgsProfilePlotRenderer *mCurrentJob = nullptr;

    std::unique_ptr< QgsCurve > mProfileCurve;
};

#endif // QGSELEVATIONPROFILECANVAS_H
