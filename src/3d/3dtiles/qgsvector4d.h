/***************************************************************************
  qgsvector4d.h
  --------------------------------------
  Date                 : March 2021
  Copyright            : (C) 2021 by Benoit De Mezzo
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVECTOR4D_H
#define QGSVECTOR4D_H

#include "qgis_core.h"
#include "qgsvector3d.h"
#include <QVector4D>
#include "qdebug.h"

/**
 * \ingroup 4d
 * \brief Class for storage of 4D vectors similar to QVector4D, with the difference that it uses double precision
 * instead of single precision floating point numbers.
 *
 * \since QGIS 3.0
 */
class CORE_EXPORT QgsVector4D
{
  public:
    //! Constructs a null vector
    QgsVector4D() = default;

    //! Constructs a vector from given coordinates
    QgsVector4D( double x, double y, double z, double w=0.0 )
      : mX( x ), mY( y ), mZ( z ) , mW( w ) {}

    //! Constructs a vector from given coordinates
    QgsVector4D( const QgsVector3D & v, double w )
        : mX( v.x() ), mY( v.y() ), mZ( v.z() ) , mW( w ) {}

    //! Constructs a vector from single-precision QVector4D
    QgsVector4D( const QVector4D &v )
      : mX( v.x() ), mY( v.y() ), mZ( v.z() ) , mW( v.w() ) {}

    //! Returns TRUE if all three coordinates are zero
    bool isNull() const { return mX == 0 && mY == 0 && mZ == 0 && mW == 0; }

    //! Returns X coordinate
    double x() const { return mX; }
    //! Returns Y coordinate
    double y() const { return mY; }
    //! Returns Z coordinate
    double z() const { return mZ; }
    //! Returns W coordinate
    double w() const { return mW; }

    //! Sets vector coordinates
    void set( double x, double y, double z, double w )
    {
      mX = x;
      mY = y;
      mZ = z;
      mW = w;
    }

    //! Sets vector coordinates
    void setX( double x )
    {
      mX = x;
    }

    //! Sets vector coordinates
    void setY( double y )
    {
      mY = y;
    }

    //! Sets vector coordinates
    void setZ( double z )
    {
      mZ = z;
    }

    //! Sets vector coordinates
    void setW( double w )
    {
      mW = w;
    }

    inline double &operator[](int i)
    {
        Q_ASSERT(uint(i) < 4u);
        switch (i) {
            case 0:
                return mX;
            case 1:
                return mY;
            case 2:
                return mZ;
            case 3:
                return mW;
        }
        return mX; // fake
    }

    inline double operator[](int i) const
    {
        Q_ASSERT(uint(i) < 4u);
        switch (i) {
            case 0:
                return mX;
            case 1:
                return mY;
            case 2:
                return mZ;
            case 3:
                return mW;
        }
        return 0.0;
    }

    bool operator==( const QgsVector4D &other ) const
    {
      return mX == other.mX && mY == other.mY && mZ == other.mZ && mW == other.mW;
    }
    bool operator!=( const QgsVector4D &other ) const
    {
      return !operator==( other );
    }

    //! Returns sum of two vectors
    QgsVector4D operator+( const QgsVector4D &other ) const
    {
      return QgsVector4D( mX + other.mX, mY + other.mY, mZ + other.mZ , mW + other.mW );
    }

    //! Returns difference of two vectors
    QgsVector4D operator-( const QgsVector4D &other ) const
    {
      return QgsVector4D( mX - other.mX, mY - other.mY, mZ - other.mZ, mW - other.mW );
    }

    //! Returns a new vector multiplied by scalar
    QgsVector4D operator *( const double factor ) const
    {

      return QgsVector4D( mX * factor, mY * factor, mZ * factor, mW * factor );
    }

    //! Returns a new vector divided by scalar
    QgsVector4D operator /( const double factor ) const
    {
      return QgsVector4D( mX / factor, mY / factor, mZ / factor, mW / factor );
    }

    //! Returns the dot product of two vectors
    static double dotProduct( const QgsVector4D &v1, const QgsVector4D &v2 )
    {
      return v1.x() * v2.x() + v1.y() * v2.y() + v1.z() * v2.z() + v1.w() * v2.w();
    }

    /*    //! Returns the cross product of two vectors
    static QgsVector4D crossProduct( const QgsVector4D &v1, const QgsVector4D &v2 )
    {
      return QgsVector4D( v1.y() * v2.z() - v1.z() * v2.y(),
                          v1.z() * v2.x() - v1.x() * v2.z(),
                          v1.x() * v2.y() - v1.y() * v2.x(),
                          v1.x() * v2.y() - v1.y() * v2.x() );
                          }*/

    //! Returns the length of the vector
    double length() const
    {
      return sqrt( mX * mX + mY * mY + mZ * mZ + mW * mW );
    }

    //! Normalizes the current vector in place.
    void normalize()
    {
      double len = length();
      if ( !qgsDoubleNear( len, 0.0 ) )
      {
        mX /= len;
        mY /= len;
        mZ /= len;
        mW /= len;
      }
    }

    //! Returns the distance with the \a other QgsVector3
    double distance( const QgsVector4D &other ) const
    {
      return std::sqrt( ( mX - other.x() ) * ( mX - other.x() ) +
                        ( mY - other.y() ) * ( mY - other.y() ) +
                        ( mZ - other.z() ) * ( mZ - other.z() ) +
                        ( mW - other.w() ) * ( mW - other.w() ) );
    }

    //! Returns the perpendicular point of vector \a vp from [\a v1 - \a v2]
    static QgsVector4D perpendicularPoint( const QgsVector4D &v1, const QgsVector4D &v2, const QgsVector4D &vp )
    {
      QgsVector4D d = ( v2 - v1 ) / v2.distance( v1 );
      QgsVector4D v = vp - v2;
      double t = dotProduct( v, d );
      QgsVector4D P = v2 + ( d * t );
      return P;
    }

    /**
     * Returns a string representation of the 4D vector.
     * Members will be truncated to the specified \a precision.
     */
    QString toString( int precision = 17 ) const
    {
      QString str = "Vector4D (";
      str += qgsDoubleToString( mX, precision );
      str += ", ";
      str += qgsDoubleToString( mY, precision );
      str += ", ";
      str += qgsDoubleToString( mZ, precision );
      str += ", ";
      str += qgsDoubleToString( mW, precision );
      str += ')';
      return str;
    }

#ifdef SIP_RUN
    SIP_PYOBJECT __repr__();
    % MethodCode
    QString str = QStringLiteral( "<QgsVector4D: %1>" ).arg( sipCpp->toString() );
    sipRes = PyUnicode_FromString( str.toUtf8().constData() );
    % End
#endif
  private:
    double mX = 0, mY = 0, mZ = 0, mW = 0;
};

Q_CORE_EXPORT QDebug &operator<<(QDebug &, const QgsVector4D &);
Q_CORE_EXPORT QDebug &operator<<(QDebug &, const QgsVector3D &);

#endif // QGSVECTOR4D_H
