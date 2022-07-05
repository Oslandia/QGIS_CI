/***************************************************************************
  qgs3dboundingboxsettings.h
  --------------------------------------
  Date                 : May 2022
  copyright            : (C) 2022 Jean Felder
  email                : jean dot felder at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DBOUNDINGBOXSETTINGS_H
#define QGS3DBOUNDINGBOXSETTINGS_H

#include <QColor>
#include <QString>

#include "qgis_3d.h"
#include "qgsaabb.h"

class QgsReadWriteContext;
class QDomElement;

#define SIP_NO_FILE

/**
 * \brief Contains the configuration of a 3d bounding box.
 *
 * \ingroup 3d
 * \since QGIS 3.26
 */
class _3D_EXPORT Qgs3DBoundingBoxSettings
{
  public:
    //! default constructor
    Qgs3DBoundingBoxSettings() = default;
    // constructor
    Qgs3DBoundingBoxSettings( bool enabled, const QgsAABB &boundingBox, int nrTicks = 7, QColor color = Qt::black, bool full = true, bool showIn2DView = false );
    //! copy constructor
    Qgs3DBoundingBoxSettings( const Qgs3DBoundingBoxSettings &other );
    //! delete assignment operator
    Qgs3DBoundingBoxSettings &operator=( Qgs3DBoundingBoxSettings const &rhs );

    //! Returns true if both objects are equal
    bool operator==( Qgs3DBoundingBoxSettings const &rhs ) const;

    //! Returns true if objects are not equal
    bool operator!=( Qgs3DBoundingBoxSettings const &rhs ) const;

    //! Reads settings from a DOM \a element
    void readXml( const QDomElement &element, const QgsReadWriteContext &context );
    //! Writes settings to a DOM \a element
    QDomElement writeXml( QDomElement &element, const QgsReadWriteContext &context ) const;

    //! Returns if bounding box is visible
    bool isEnabled() const { return mEnabled; }
    //! Sets the bounding box visibility
    void setEnabled( bool enabled ) { mEnabled = enabled; }

    //! Returns bounding box coordinates
    QgsAABB coords() const { return mBoundingBox; }

    //! Returns bounding box number of ticks
    int nrTicks() const { return mNrTicks; }

    //! Returns bounding box color
    QColor color() const { return mColor; }

    /**
     * Returns bounding full state
     *
     * When a bounding is in full state, the whole cuboid is visible.
     * Otherwise, only the background faces are visible.
     */
    bool isFull() const { return mFull; }

    //! Returns whether the bounding box is displayed in the 2D View
    bool showIn2DView() const { return mShowIn2DView; }

  private:
    bool mEnabled = false;
    QgsAABB mBoundingBox;
    int mNrTicks = 7;
    QColor mColor = Qt::black;
    bool mFull = true;
    bool mShowIn2DView = false;
};

#endif // QGS3DBOUNDINGBOXSETTINGS_H
