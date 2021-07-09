#ifndef QGSMATRIX4X4_H
#define QGSMATRIX4X4_H

#include "qgis_core.h"
#include "qgsvector4d.h"
#include "qgsvector3d.h"

#include <QQuaternion>
#include <QtCore/qmath.h>
//#include <QtCore/qvariant.h>
#include <QtGui/qtransform.h>

#include <cmath>

#include <QRectF>
#include <QMatrix4x4>
#include "qgscoordinatetransform.h"

class QgsMatrix;
class QTransform;
class QVariant;

class CORE_EXPORT QgsMatrix4x4
{
  public:
    inline QgsMatrix4x4() { setToIdentity(); }
    explicit QgsMatrix4x4( const double *values );
    inline QgsMatrix4x4( double m11, double m12, double m13, double m14,
                         double m21, double m22, double m23, double m24,
                         double m31, double m32, double m33, double m34,
                         double m41, double m42, double m43, double m44 );


    QgsMatrix4x4( const double *values, int cols, int rows );
    QgsMatrix4x4( const QTransform &transform );
    QgsMatrix4x4( const QgsMatrix &matrix );

    inline const double &operator()( int row, int column ) const;
    inline double &operator()( int row, int column );

#ifndef QT_NO_VECTOR4D
    inline QgsVector4D column( int index ) const;
    inline void setColumn( int index, const QgsVector4D &value );

    inline QgsVector4D row( int index ) const;
    inline void setRow( int index, const QgsVector4D &value );
#endif

    inline bool isAffine() const;

    inline bool isIdentity() const;
    inline void setToIdentity();

    inline void fill( double value );

    double determinant() const;
    QgsMatrix4x4 inverted( bool *invertible = nullptr ) const;
    QgsMatrix4x4 transposed() const;
    //QgsMatrix3x3 normalMatrix() const;

    inline QgsMatrix4x4 &operator+=( const QgsMatrix4x4 &other );
    inline QgsMatrix4x4 &operator-=( const QgsMatrix4x4 &other );
    inline QgsMatrix4x4 &operator*=( const QgsMatrix4x4 &other );
    inline QgsMatrix4x4 &operator*=( double factor );
    QgsMatrix4x4 &operator/=( double divisor );
    inline bool operator==( const QgsMatrix4x4 &other ) const;
    inline bool operator!=( const QgsMatrix4x4 &other ) const;

    friend QgsMatrix4x4 operator+( const QgsMatrix4x4 &m1, const QgsMatrix4x4 &m2 );
    friend QgsMatrix4x4 operator-( const QgsMatrix4x4 &m1, const QgsMatrix4x4 &m2 );
    friend QgsMatrix4x4 operator*( const QgsMatrix4x4 &m1, const QgsMatrix4x4 &m2 );
#ifndef QT_NO_VECTOR3D
    friend QgsVector3D operator*( const QgsMatrix4x4 &matrix, const QgsVector3D &vector );
    friend QgsVector3D operator*( const QgsVector3D &vector, const QgsMatrix4x4 &matrix );
#endif
#ifndef QT_NO_VECTOR4D
    friend QgsVector4D operator*( const QgsVector4D &vector, const QgsMatrix4x4 &matrix );
    friend QgsVector4D operator*( const QgsMatrix4x4 &matrix, const QgsVector4D &vector );
#endif
    friend QPoint operator*( const QPoint &point, const QgsMatrix4x4 &matrix );
    friend QPointF operator*( const QPointF &point, const QgsMatrix4x4 &matrix );
    friend QgsMatrix4x4 operator-( const QgsMatrix4x4 &matrix );
    friend QPoint operator*( const QgsMatrix4x4 &matrix, const QPoint &point );
    friend QPointF operator*( const QgsMatrix4x4 &matrix, const QPointF &point );
    friend QgsMatrix4x4 operator*( double factor, const QgsMatrix4x4 &matrix );
    friend QgsMatrix4x4 operator*( const QgsMatrix4x4 &matrix, double factor );
    friend Q_GUI_EXPORT QgsMatrix4x4 operator/( const QgsMatrix4x4 &matrix, double divisor );

    friend inline bool qFuzzyCompare( const QgsMatrix4x4 &m1, const QgsMatrix4x4 &m2 );

#ifndef QT_NO_VECTOR3D
    void scale( const QgsVector3D &vector );
    void translate( const QgsVector3D &vector );
    void rotate( double angle, const QgsVector3D &vector );
#endif
    void scale( double x, double y );
    void scale( double x, double y, double z );
    void scale( double factor );
    void translate( double x, double y );
    void translate( double x, double y, double z );
    void rotate( double angle, double x, double y, double z = 0.0f );
#ifndef QT_NO_QUATERNION
    void rotate( const QQuaternion &quaternion );
#endif

    void ortho( const QRect &rect );
    void ortho( const QRectF &rect );
    void ortho( double left, double right, double bottom, double top, double nearPlane, double farPlane );
    void frustum( double left, double right, double bottom, double top, double nearPlane, double farPlane );
    void perspective( double verticalAngle, double aspectRatio, double nearPlane, double farPlane );
#ifndef QT_NO_VECTOR3D
    void lookAt( const QgsVector3D &eye, const QgsVector3D &center, const QgsVector3D &up );
#endif
    void viewport( const QRectF &rect );
    void viewport( double left, double bottom, double width, double height, double nearPlane = 0.0f, double farPlane = 1.0f );
    void flipCoordinates();

    void copyDataTo( double *values ) const;

    QgsMatrix toAffine() const;
    QTransform toTransform() const;
    QTransform toTransform( double distanceToPlane ) const;
    QMatrix4x4 toQMatrix4x4( const QgsCoordinateTransform *coordTrans ) const;

    QPoint map( const QPoint &point ) const;
    QPointF map( const QPointF &point ) const;
#ifndef QT_NO_VECTOR3D
    QgsVector3D map( const QgsVector3D &point ) const;
    QgsVector3D mapVector( const QgsVector3D &vector ) const;
#endif
#ifndef QT_NO_VECTOR4D
    QgsVector4D map( const QgsVector4D &point ) const;
#endif
    QRect mapRect( const QRect &rect ) const;
    QRectF mapRect( const QRectF &rect ) const;

    inline double *data();
    inline const double *data() const { return *m; }
    inline const double *constData() const { return *m; }

    void optimize();

    /*operator QVariant() const;*/

#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<( QDebug dbg, const QgsMatrix4x4 &m );
#endif

  private:
    double m[4][4];          // Column-major order to match OpenGL.
    int flagBits;           // Flag bits from the enum below.

    // When matrices are multiplied, the flag bits are or-ed together.
    enum
    {
      Identity        = 0x0000, // Identity matrix
      Translation     = 0x0001, // Contains a translation
      Scale           = 0x0002, // Contains a scale
      Rotation2D      = 0x0004, // Contains a rotation about the Z axis
      Rotation        = 0x0008, // Contains an arbitrary rotation
      Perspective     = 0x0010, // Last row is different from (0, 0, 0, 1)
      General         = 0x001f  // General matrix, unknown contents
    };

    // Construct without initializing identity matrix.
    explicit QgsMatrix4x4( int ) { }

    QgsMatrix4x4 orthonormalInverse() const;

    void projectedRotate( double angle, double x, double y, double z );

    friend class QGraphicsRotation;
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG( "-Wdouble-equal" )
QT_WARNING_DISABLE_GCC( "-Wdouble-equal" )
QT_WARNING_DISABLE_INTEL( 1572 )
Q_DECLARE_TYPEINFO( QgsMatrix4x4, Q_MOVABLE_TYPE );

inline QgsMatrix4x4::QgsMatrix4x4
( double m11, double m12, double m13, double m14,
  double m21, double m22, double m23, double m24,
  double m31, double m32, double m33, double m34,
  double m41, double m42, double m43, double m44 )
{
  m[0][0] = m11; m[0][1] = m21; m[0][2] = m31; m[0][3] = m41;
  m[1][0] = m12; m[1][1] = m22; m[1][2] = m32; m[1][3] = m42;
  m[2][0] = m13; m[2][1] = m23; m[2][2] = m33; m[2][3] = m43;
  m[3][0] = m14; m[3][1] = m24; m[3][2] = m34; m[3][3] = m44;
  flagBits = General;
}


inline const double &QgsMatrix4x4::operator()( int aRow, int aColumn ) const
{
  Q_ASSERT( aRow >= 0 && aRow < 4 && aColumn >= 0 && aColumn < 4 );
  return m[aColumn][aRow];
}

inline double &QgsMatrix4x4::operator()( int aRow, int aColumn )
{
  Q_ASSERT( aRow >= 0 && aRow < 4 && aColumn >= 0 && aColumn < 4 );
  flagBits = General;
  return m[aColumn][aRow];
}

#ifndef QT_NO_VECTOR4D
inline QgsVector4D QgsMatrix4x4::column( int index ) const
{
  Q_ASSERT( index >= 0 && index < 4 );
  return QgsVector4D( m[index][0], m[index][1], m[index][2], m[index][3] );
}

inline void QgsMatrix4x4::setColumn( int index, const QgsVector4D &value )
{
  Q_ASSERT( index >= 0 && index < 4 );
  m[index][0] = value.x();
  m[index][1] = value.y();
  m[index][2] = value.z();
  m[index][3] = value.w();
  flagBits = General;
}

inline QgsVector4D QgsMatrix4x4::row( int index ) const
{
  Q_ASSERT( index >= 0 && index < 4 );
  return QgsVector4D( m[0][index], m[1][index], m[2][index], m[3][index] );
}

inline void QgsMatrix4x4::setRow( int index, const QgsVector4D &value )
{
  Q_ASSERT( index >= 0 && index < 4 );
  m[0][index] = value.x();
  m[1][index] = value.y();
  m[2][index] = value.z();
  m[3][index] = value.w();
  flagBits = General;
}
#endif

Q_GUI_EXPORT QgsMatrix4x4 operator/( const QgsMatrix4x4 &matrix, double divisor );

inline bool QgsMatrix4x4::isAffine() const
{
  return m[0][3] == 0.0f && m[1][3] == 0.0f && m[2][3] == 0.0f && m[3][3] == 1.0f;
}

inline bool QgsMatrix4x4::isIdentity() const
{
  if ( flagBits == Identity )
    return true;
  if ( m[0][0] != 1.0f || m[0][1] != 0.0f || m[0][2] != 0.0f )
    return false;
  if ( m[0][3] != 0.0f || m[1][0] != 0.0f || m[1][1] != 1.0f )
    return false;
  if ( m[1][2] != 0.0f || m[1][3] != 0.0f || m[2][0] != 0.0f )
    return false;
  if ( m[2][1] != 0.0f || m[2][2] != 1.0f || m[2][3] != 0.0f )
    return false;
  if ( m[3][0] != 0.0f || m[3][1] != 0.0f || m[3][2] != 0.0f )
    return false;
  return ( m[3][3] == 1.0f );
}

inline void QgsMatrix4x4::setToIdentity()
{
  m[0][0] = 1.0f;
  m[0][1] = 0.0f;
  m[0][2] = 0.0f;
  m[0][3] = 0.0f;
  m[1][0] = 0.0f;
  m[1][1] = 1.0f;
  m[1][2] = 0.0f;
  m[1][3] = 0.0f;
  m[2][0] = 0.0f;
  m[2][1] = 0.0f;
  m[2][2] = 1.0f;
  m[2][3] = 0.0f;
  m[3][0] = 0.0f;
  m[3][1] = 0.0f;
  m[3][2] = 0.0f;
  m[3][3] = 1.0f;
  flagBits = Identity;
}

inline void QgsMatrix4x4::fill( double value )
{
  m[0][0] = value;
  m[0][1] = value;
  m[0][2] = value;
  m[0][3] = value;
  m[1][0] = value;
  m[1][1] = value;
  m[1][2] = value;
  m[1][3] = value;
  m[2][0] = value;
  m[2][1] = value;
  m[2][2] = value;
  m[2][3] = value;
  m[3][0] = value;
  m[3][1] = value;
  m[3][2] = value;
  m[3][3] = value;
  flagBits = General;
}

inline QgsMatrix4x4 &QgsMatrix4x4::operator+=( const QgsMatrix4x4 &other )
{
  m[0][0] += other.m[0][0];
  m[0][1] += other.m[0][1];
  m[0][2] += other.m[0][2];
  m[0][3] += other.m[0][3];
  m[1][0] += other.m[1][0];
  m[1][1] += other.m[1][1];
  m[1][2] += other.m[1][2];
  m[1][3] += other.m[1][3];
  m[2][0] += other.m[2][0];
  m[2][1] += other.m[2][1];
  m[2][2] += other.m[2][2];
  m[2][3] += other.m[2][3];
  m[3][0] += other.m[3][0];
  m[3][1] += other.m[3][1];
  m[3][2] += other.m[3][2];
  m[3][3] += other.m[3][3];
  flagBits = General;
  return *this;
}

inline QgsMatrix4x4 &QgsMatrix4x4::operator-=( const QgsMatrix4x4 &other )
{
  m[0][0] -= other.m[0][0];
  m[0][1] -= other.m[0][1];
  m[0][2] -= other.m[0][2];
  m[0][3] -= other.m[0][3];
  m[1][0] -= other.m[1][0];
  m[1][1] -= other.m[1][1];
  m[1][2] -= other.m[1][2];
  m[1][3] -= other.m[1][3];
  m[2][0] -= other.m[2][0];
  m[2][1] -= other.m[2][1];
  m[2][2] -= other.m[2][2];
  m[2][3] -= other.m[2][3];
  m[3][0] -= other.m[3][0];
  m[3][1] -= other.m[3][1];
  m[3][2] -= other.m[3][2];
  m[3][3] -= other.m[3][3];
  flagBits = General;
  return *this;
}

inline QgsMatrix4x4 &QgsMatrix4x4::operator*=( const QgsMatrix4x4 &o )
{
  const QgsMatrix4x4 other = o; // prevent aliasing when &o == this ### Qt 6: take o by value
  flagBits |= other.flagBits;

  if ( flagBits < Rotation2D )
  {
    m[3][0] += m[0][0] * other.m[3][0];
    m[3][1] += m[1][1] * other.m[3][1];
    m[3][2] += m[2][2] * other.m[3][2];

    m[0][0] *= other.m[0][0];
    m[1][1] *= other.m[1][1];
    m[2][2] *= other.m[2][2];
    return *this;
  }

  double m0, m1, m2;
  m0 = m[0][0] * other.m[0][0]
       + m[1][0] * other.m[0][1]
       + m[2][0] * other.m[0][2]
       + m[3][0] * other.m[0][3];
  m1 = m[0][0] * other.m[1][0]
       + m[1][0] * other.m[1][1]
       + m[2][0] * other.m[1][2]
       + m[3][0] * other.m[1][3];
  m2 = m[0][0] * other.m[2][0]
       + m[1][0] * other.m[2][1]
       + m[2][0] * other.m[2][2]
       + m[3][0] * other.m[2][3];
  m[3][0] = m[0][0] * other.m[3][0]
            + m[1][0] * other.m[3][1]
            + m[2][0] * other.m[3][2]
            + m[3][0] * other.m[3][3];
  m[0][0] = m0;
  m[1][0] = m1;
  m[2][0] = m2;

  m0 = m[0][1] * other.m[0][0]
       + m[1][1] * other.m[0][1]
       + m[2][1] * other.m[0][2]
       + m[3][1] * other.m[0][3];
  m1 = m[0][1] * other.m[1][0]
       + m[1][1] * other.m[1][1]
       + m[2][1] * other.m[1][2]
       + m[3][1] * other.m[1][3];
  m2 = m[0][1] * other.m[2][0]
       + m[1][1] * other.m[2][1]
       + m[2][1] * other.m[2][2]
       + m[3][1] * other.m[2][3];
  m[3][1] = m[0][1] * other.m[3][0]
            + m[1][1] * other.m[3][1]
            + m[2][1] * other.m[3][2]
            + m[3][1] * other.m[3][3];
  m[0][1] = m0;
  m[1][1] = m1;
  m[2][1] = m2;

  m0 = m[0][2] * other.m[0][0]
       + m[1][2] * other.m[0][1]
       + m[2][2] * other.m[0][2]
       + m[3][2] * other.m[0][3];
  m1 = m[0][2] * other.m[1][0]
       + m[1][2] * other.m[1][1]
       + m[2][2] * other.m[1][2]
       + m[3][2] * other.m[1][3];
  m2 = m[0][2] * other.m[2][0]
       + m[1][2] * other.m[2][1]
       + m[2][2] * other.m[2][2]
       + m[3][2] * other.m[2][3];
  m[3][2] = m[0][2] * other.m[3][0]
            + m[1][2] * other.m[3][1]
            + m[2][2] * other.m[3][2]
            + m[3][2] * other.m[3][3];
  m[0][2] = m0;
  m[1][2] = m1;
  m[2][2] = m2;

  m0 = m[0][3] * other.m[0][0]
       + m[1][3] * other.m[0][1]
       + m[2][3] * other.m[0][2]
       + m[3][3] * other.m[0][3];
  m1 = m[0][3] * other.m[1][0]
       + m[1][3] * other.m[1][1]
       + m[2][3] * other.m[1][2]
       + m[3][3] * other.m[1][3];
  m2 = m[0][3] * other.m[2][0]
       + m[1][3] * other.m[2][1]
       + m[2][3] * other.m[2][2]
       + m[3][3] * other.m[2][3];
  m[3][3] = m[0][3] * other.m[3][0]
            + m[1][3] * other.m[3][1]
            + m[2][3] * other.m[3][2]
            + m[3][3] * other.m[3][3];
  m[0][3] = m0;
  m[1][3] = m1;
  m[2][3] = m2;
  return *this;
}

inline QgsMatrix4x4 &QgsMatrix4x4::operator*=( double factor )
{
  m[0][0] *= factor;
  m[0][1] *= factor;
  m[0][2] *= factor;
  m[0][3] *= factor;
  m[1][0] *= factor;
  m[1][1] *= factor;
  m[1][2] *= factor;
  m[1][3] *= factor;
  m[2][0] *= factor;
  m[2][1] *= factor;
  m[2][2] *= factor;
  m[2][3] *= factor;
  m[3][0] *= factor;
  m[3][1] *= factor;
  m[3][2] *= factor;
  m[3][3] *= factor;
  flagBits = General;
  return *this;
}

inline bool QgsMatrix4x4::operator==( const QgsMatrix4x4 &other ) const
{
  return m[0][0] == other.m[0][0] &&
         m[0][1] == other.m[0][1] &&
         m[0][2] == other.m[0][2] &&
         m[0][3] == other.m[0][3] &&
         m[1][0] == other.m[1][0] &&
         m[1][1] == other.m[1][1] &&
         m[1][2] == other.m[1][2] &&
         m[1][3] == other.m[1][3] &&
         m[2][0] == other.m[2][0] &&
         m[2][1] == other.m[2][1] &&
         m[2][2] == other.m[2][2] &&
         m[2][3] == other.m[2][3] &&
         m[3][0] == other.m[3][0] &&
         m[3][1] == other.m[3][1] &&
         m[3][2] == other.m[3][2] &&
         m[3][3] == other.m[3][3];
}

inline bool QgsMatrix4x4::operator!=( const QgsMatrix4x4 &other ) const
{
  return m[0][0] != other.m[0][0] ||
         m[0][1] != other.m[0][1] ||
         m[0][2] != other.m[0][2] ||
         m[0][3] != other.m[0][3] ||
         m[1][0] != other.m[1][0] ||
         m[1][1] != other.m[1][1] ||
         m[1][2] != other.m[1][2] ||
         m[1][3] != other.m[1][3] ||
         m[2][0] != other.m[2][0] ||
         m[2][1] != other.m[2][1] ||
         m[2][2] != other.m[2][2] ||
         m[2][3] != other.m[2][3] ||
         m[3][0] != other.m[3][0] ||
         m[3][1] != other.m[3][1] ||
         m[3][2] != other.m[3][2] ||
         m[3][3] != other.m[3][3];
}

inline QgsMatrix4x4 operator+( const QgsMatrix4x4 &m1, const QgsMatrix4x4 &m2 )
{
  QgsMatrix4x4 m( 1 );
  m.m[0][0] = m1.m[0][0] + m2.m[0][0];
  m.m[0][1] = m1.m[0][1] + m2.m[0][1];
  m.m[0][2] = m1.m[0][2] + m2.m[0][2];
  m.m[0][3] = m1.m[0][3] + m2.m[0][3];
  m.m[1][0] = m1.m[1][0] + m2.m[1][0];
  m.m[1][1] = m1.m[1][1] + m2.m[1][1];
  m.m[1][2] = m1.m[1][2] + m2.m[1][2];
  m.m[1][3] = m1.m[1][3] + m2.m[1][3];
  m.m[2][0] = m1.m[2][0] + m2.m[2][0];
  m.m[2][1] = m1.m[2][1] + m2.m[2][1];
  m.m[2][2] = m1.m[2][2] + m2.m[2][2];
  m.m[2][3] = m1.m[2][3] + m2.m[2][3];
  m.m[3][0] = m1.m[3][0] + m2.m[3][0];
  m.m[3][1] = m1.m[3][1] + m2.m[3][1];
  m.m[3][2] = m1.m[3][2] + m2.m[3][2];
  m.m[3][3] = m1.m[3][3] + m2.m[3][3];
  m.flagBits = QgsMatrix4x4::General;
  return m;
}

inline QgsMatrix4x4 operator-( const QgsMatrix4x4 &m1, const QgsMatrix4x4 &m2 )
{
  QgsMatrix4x4 m( 1 );
  m.m[0][0] = m1.m[0][0] - m2.m[0][0];
  m.m[0][1] = m1.m[0][1] - m2.m[0][1];
  m.m[0][2] = m1.m[0][2] - m2.m[0][2];
  m.m[0][3] = m1.m[0][3] - m2.m[0][3];
  m.m[1][0] = m1.m[1][0] - m2.m[1][0];
  m.m[1][1] = m1.m[1][1] - m2.m[1][1];
  m.m[1][2] = m1.m[1][2] - m2.m[1][2];
  m.m[1][3] = m1.m[1][3] - m2.m[1][3];
  m.m[2][0] = m1.m[2][0] - m2.m[2][0];
  m.m[2][1] = m1.m[2][1] - m2.m[2][1];
  m.m[2][2] = m1.m[2][2] - m2.m[2][2];
  m.m[2][3] = m1.m[2][3] - m2.m[2][3];
  m.m[3][0] = m1.m[3][0] - m2.m[3][0];
  m.m[3][1] = m1.m[3][1] - m2.m[3][1];
  m.m[3][2] = m1.m[3][2] - m2.m[3][2];
  m.m[3][3] = m1.m[3][3] - m2.m[3][3];
  m.flagBits = QgsMatrix4x4::General;
  return m;
}

inline QgsMatrix4x4 operator*( const QgsMatrix4x4 &m1, const QgsMatrix4x4 &m2 )
{
  int flagBits = m1.flagBits | m2.flagBits;
  if ( flagBits < QgsMatrix4x4::Rotation2D )
  {
    QgsMatrix4x4 m = m1;
    m.m[3][0] += m.m[0][0] * m2.m[3][0];
    m.m[3][1] += m.m[1][1] * m2.m[3][1];
    m.m[3][2] += m.m[2][2] * m2.m[3][2];

    m.m[0][0] *= m2.m[0][0];
    m.m[1][1] *= m2.m[1][1];
    m.m[2][2] *= m2.m[2][2];
    m.flagBits = flagBits;
    return m;
  }

  QgsMatrix4x4 m( 1 );
  m.m[0][0] = m1.m[0][0] * m2.m[0][0]
              + m1.m[1][0] * m2.m[0][1]
              + m1.m[2][0] * m2.m[0][2]
              + m1.m[3][0] * m2.m[0][3];
  m.m[0][1] = m1.m[0][1] * m2.m[0][0]
              + m1.m[1][1] * m2.m[0][1]
              + m1.m[2][1] * m2.m[0][2]
              + m1.m[3][1] * m2.m[0][3];
  m.m[0][2] = m1.m[0][2] * m2.m[0][0]
              + m1.m[1][2] * m2.m[0][1]
              + m1.m[2][2] * m2.m[0][2]
              + m1.m[3][2] * m2.m[0][3];
  m.m[0][3] = m1.m[0][3] * m2.m[0][0]
              + m1.m[1][3] * m2.m[0][1]
              + m1.m[2][3] * m2.m[0][2]
              + m1.m[3][3] * m2.m[0][3];

  m.m[1][0] = m1.m[0][0] * m2.m[1][0]
              + m1.m[1][0] * m2.m[1][1]
              + m1.m[2][0] * m2.m[1][2]
              + m1.m[3][0] * m2.m[1][3];
  m.m[1][1] = m1.m[0][1] * m2.m[1][0]
              + m1.m[1][1] * m2.m[1][1]
              + m1.m[2][1] * m2.m[1][2]
              + m1.m[3][1] * m2.m[1][3];
  m.m[1][2] = m1.m[0][2] * m2.m[1][0]
              + m1.m[1][2] * m2.m[1][1]
              + m1.m[2][2] * m2.m[1][2]
              + m1.m[3][2] * m2.m[1][3];
  m.m[1][3] = m1.m[0][3] * m2.m[1][0]
              + m1.m[1][3] * m2.m[1][1]
              + m1.m[2][3] * m2.m[1][2]
              + m1.m[3][3] * m2.m[1][3];

  m.m[2][0] = m1.m[0][0] * m2.m[2][0]
              + m1.m[1][0] * m2.m[2][1]
              + m1.m[2][0] * m2.m[2][2]
              + m1.m[3][0] * m2.m[2][3];
  m.m[2][1] = m1.m[0][1] * m2.m[2][0]
              + m1.m[1][1] * m2.m[2][1]
              + m1.m[2][1] * m2.m[2][2]
              + m1.m[3][1] * m2.m[2][3];
  m.m[2][2] = m1.m[0][2] * m2.m[2][0]
              + m1.m[1][2] * m2.m[2][1]
              + m1.m[2][2] * m2.m[2][2]
              + m1.m[3][2] * m2.m[2][3];
  m.m[2][3] = m1.m[0][3] * m2.m[2][0]
              + m1.m[1][3] * m2.m[2][1]
              + m1.m[2][3] * m2.m[2][2]
              + m1.m[3][3] * m2.m[2][3];

  m.m[3][0] = m1.m[0][0] * m2.m[3][0]
              + m1.m[1][0] * m2.m[3][1]
              + m1.m[2][0] * m2.m[3][2]
              + m1.m[3][0] * m2.m[3][3];
  m.m[3][1] = m1.m[0][1] * m2.m[3][0]
              + m1.m[1][1] * m2.m[3][1]
              + m1.m[2][1] * m2.m[3][2]
              + m1.m[3][1] * m2.m[3][3];
  m.m[3][2] = m1.m[0][2] * m2.m[3][0]
              + m1.m[1][2] * m2.m[3][1]
              + m1.m[2][2] * m2.m[3][2]
              + m1.m[3][2] * m2.m[3][3];
  m.m[3][3] = m1.m[0][3] * m2.m[3][0]
              + m1.m[1][3] * m2.m[3][1]
              + m1.m[2][3] * m2.m[3][2]
              + m1.m[3][3] * m2.m[3][3];
  m.flagBits = flagBits;
  return m;
}

#ifndef QT_NO_VECTOR3D

inline QgsVector3D operator*( const QgsVector3D &vector, const QgsMatrix4x4 &matrix )
{
  double x, y, z, w;
  x = vector.x() * matrix.m[0][0] +
      vector.y() * matrix.m[0][1] +
      vector.z() * matrix.m[0][2] +
      matrix.m[0][3];
  y = vector.x() * matrix.m[1][0] +
      vector.y() * matrix.m[1][1] +
      vector.z() * matrix.m[1][2] +
      matrix.m[1][3];
  z = vector.x() * matrix.m[2][0] +
      vector.y() * matrix.m[2][1] +
      vector.z() * matrix.m[2][2] +
      matrix.m[2][3];
  w = vector.x() * matrix.m[3][0] +
      vector.y() * matrix.m[3][1] +
      vector.z() * matrix.m[3][2] +
      matrix.m[3][3];
  if ( w == 1.0f )
    return QgsVector3D( x, y, z );
  else
    return QgsVector3D( x / w, y / w, z / w );
}

inline QgsVector3D operator*( const QgsMatrix4x4 &matrix, const QgsVector3D &vector )
{
  double x, y, z, w;
  if ( matrix.flagBits == QgsMatrix4x4::Identity )
  {
    return vector;
  }
  else if ( matrix.flagBits < QgsMatrix4x4::Rotation2D )
  {
    // Translation | Scale
    return QgsVector3D( vector.x() * matrix.m[0][0] + matrix.m[3][0],
                        vector.y() * matrix.m[1][1] + matrix.m[3][1],
                        vector.z() * matrix.m[2][2] + matrix.m[3][2] );
  }
  else if ( matrix.flagBits < QgsMatrix4x4::Rotation )
  {
    // Translation | Scale | Rotation2D
    return QgsVector3D( vector.x() * matrix.m[0][0] + vector.y() * matrix.m[1][0] + matrix.m[3][0],
                        vector.x() * matrix.m[0][1] + vector.y() * matrix.m[1][1] + matrix.m[3][1],
                        vector.z() * matrix.m[2][2] + matrix.m[3][2] );
  }
  else
  {
    x = vector.x() * matrix.m[0][0] +
        vector.y() * matrix.m[1][0] +
        vector.z() * matrix.m[2][0] +
        matrix.m[3][0];
    y = vector.x() * matrix.m[0][1] +
        vector.y() * matrix.m[1][1] +
        vector.z() * matrix.m[2][1] +
        matrix.m[3][1];
    z = vector.x() * matrix.m[0][2] +
        vector.y() * matrix.m[1][2] +
        vector.z() * matrix.m[2][2] +
        matrix.m[3][2];
    w = vector.x() * matrix.m[0][3] +
        vector.y() * matrix.m[1][3] +
        vector.z() * matrix.m[2][3] +
        matrix.m[3][3];
    if ( w == 1.0f )
      return QgsVector3D( x, y, z );
    else
      return QgsVector3D( x / w, y / w, z / w );
  }
}

#endif

#ifndef QT_NO_VECTOR4D

inline QgsVector4D operator*( const QgsVector4D &vector, const QgsMatrix4x4 &matrix )
{
  double x, y, z, w;
  x = vector.x() * matrix.m[0][0] +
      vector.y() * matrix.m[0][1] +
      vector.z() * matrix.m[0][2] +
      vector.w() * matrix.m[0][3];
  y = vector.x() * matrix.m[1][0] +
      vector.y() * matrix.m[1][1] +
      vector.z() * matrix.m[1][2] +
      vector.w() * matrix.m[1][3];
  z = vector.x() * matrix.m[2][0] +
      vector.y() * matrix.m[2][1] +
      vector.z() * matrix.m[2][2] +
      vector.w() * matrix.m[2][3];
  w = vector.x() * matrix.m[3][0] +
      vector.y() * matrix.m[3][1] +
      vector.z() * matrix.m[3][2] +
      vector.w() * matrix.m[3][3];
  return QgsVector4D( x, y, z, w );
}

inline QgsVector4D operator*( const QgsMatrix4x4 &matrix, const QgsVector4D &vector )
{
  double x, y, z, w;
  x = vector.x() * matrix.m[0][0] +
      vector.y() * matrix.m[1][0] +
      vector.z() * matrix.m[2][0] +
      vector.w() * matrix.m[3][0];
  y = vector.x() * matrix.m[0][1] +
      vector.y() * matrix.m[1][1] +
      vector.z() * matrix.m[2][1] +
      vector.w() * matrix.m[3][1];
  z = vector.x() * matrix.m[0][2] +
      vector.y() * matrix.m[1][2] +
      vector.z() * matrix.m[2][2] +
      vector.w() * matrix.m[3][2];
  w = vector.x() * matrix.m[0][3] +
      vector.y() * matrix.m[1][3] +
      vector.z() * matrix.m[2][3] +
      vector.w() * matrix.m[3][3];
  return QgsVector4D( x, y, z, w );
}

#endif

inline QPoint operator*( const QPoint &point, const QgsMatrix4x4 &matrix )
{
  double xin, yin;
  double x, y, w;
  xin = point.x();
  yin = point.y();
  x = xin * matrix.m[0][0] +
      yin * matrix.m[0][1] +
      matrix.m[0][3];
  y = xin * matrix.m[1][0] +
      yin * matrix.m[1][1] +
      matrix.m[1][3];
  w = xin * matrix.m[3][0] +
      yin * matrix.m[3][1] +
      matrix.m[3][3];
  if ( w == 1.0f )
    return QPoint( qRound( x ), qRound( y ) );
  else
    return QPoint( qRound( x / w ), qRound( y / w ) );
}

inline QPointF operator*( const QPointF &point, const QgsMatrix4x4 &matrix )
{
  double xin, yin;
  double x, y, w;
  xin = double( point.x() );
  yin = double( point.y() );
  x = xin * matrix.m[0][0] +
      yin * matrix.m[0][1] +
      matrix.m[0][3];
  y = xin * matrix.m[1][0] +
      yin * matrix.m[1][1] +
      matrix.m[1][3];
  w = xin * matrix.m[3][0] +
      yin * matrix.m[3][1] +
      matrix.m[3][3];
  if ( w == 1.0f )
  {
    return QPointF( qreal( x ), qreal( y ) );
  }
  else
  {
    return QPointF( qreal( x / w ), qreal( y / w ) );
  }
}

inline QPoint operator*( const QgsMatrix4x4 &matrix, const QPoint &point )
{
  double xin, yin;
  double x, y, w;
  xin = point.x();
  yin = point.y();
  if ( matrix.flagBits == QgsMatrix4x4::Identity )
  {
    return point;
  }
  else if ( matrix.flagBits < QgsMatrix4x4::Rotation2D )
  {
    // Translation | Scale
    return QPoint( qRound( xin * matrix.m[0][0] + matrix.m[3][0] ),
                   qRound( yin * matrix.m[1][1] + matrix.m[3][1] ) );
  }
  else if ( matrix.flagBits < QgsMatrix4x4::Perspective )
  {
    return QPoint( qRound( xin * matrix.m[0][0] + yin * matrix.m[1][0] + matrix.m[3][0] ),
                   qRound( xin * matrix.m[0][1] + yin * matrix.m[1][1] + matrix.m[3][1] ) );
  }
  else
  {
    x = xin * matrix.m[0][0] +
        yin * matrix.m[1][0] +
        matrix.m[3][0];
    y = xin * matrix.m[0][1] +
        yin * matrix.m[1][1] +
        matrix.m[3][1];
    w = xin * matrix.m[0][3] +
        yin * matrix.m[1][3] +
        matrix.m[3][3];
    if ( w == 1.0f )
      return QPoint( qRound( x ), qRound( y ) );
    else
      return QPoint( qRound( x / w ), qRound( y / w ) );
  }
}

inline QPointF operator*( const QgsMatrix4x4 &matrix, const QPointF &point )
{
  qreal xin, yin;
  qreal x, y, w;
  xin = point.x();
  yin = point.y();
  if ( matrix.flagBits == QgsMatrix4x4::Identity )
  {
    return point;
  }
  else if ( matrix.flagBits < QgsMatrix4x4::Rotation2D )
  {
    // Translation | Scale
    return QPointF( xin * qreal( matrix.m[0][0] ) + qreal( matrix.m[3][0] ),
                    yin * qreal( matrix.m[1][1] ) + qreal( matrix.m[3][1] ) );
  }
  else if ( matrix.flagBits < QgsMatrix4x4::Perspective )
  {
    return QPointF( xin * qreal( matrix.m[0][0] ) + yin * qreal( matrix.m[1][0] ) +
                    qreal( matrix.m[3][0] ),
                    xin * qreal( matrix.m[0][1] ) + yin * qreal( matrix.m[1][1] ) +
                    qreal( matrix.m[3][1] ) );
  }
  else
  {
    x = xin * qreal( matrix.m[0][0] ) +
        yin * qreal( matrix.m[1][0] ) +
        qreal( matrix.m[3][0] );
    y = xin * qreal( matrix.m[0][1] ) +
        yin * qreal( matrix.m[1][1] ) +
        qreal( matrix.m[3][1] );
    w = xin * qreal( matrix.m[0][3] ) +
        yin * qreal( matrix.m[1][3] ) +
        qreal( matrix.m[3][3] );
    if ( w == 1.0 )
    {
      return QPointF( qreal( x ), qreal( y ) );
    }
    else
    {
      return QPointF( qreal( x / w ), qreal( y / w ) );
    }
  }
}

inline QgsMatrix4x4 operator-( const QgsMatrix4x4 &matrix )
{
  QgsMatrix4x4 m( 1 );
  m.m[0][0] = -matrix.m[0][0];
  m.m[0][1] = -matrix.m[0][1];
  m.m[0][2] = -matrix.m[0][2];
  m.m[0][3] = -matrix.m[0][3];
  m.m[1][0] = -matrix.m[1][0];
  m.m[1][1] = -matrix.m[1][1];
  m.m[1][2] = -matrix.m[1][2];
  m.m[1][3] = -matrix.m[1][3];
  m.m[2][0] = -matrix.m[2][0];
  m.m[2][1] = -matrix.m[2][1];
  m.m[2][2] = -matrix.m[2][2];
  m.m[2][3] = -matrix.m[2][3];
  m.m[3][0] = -matrix.m[3][0];
  m.m[3][1] = -matrix.m[3][1];
  m.m[3][2] = -matrix.m[3][2];
  m.m[3][3] = -matrix.m[3][3];
  m.flagBits = QgsMatrix4x4::General;
  return m;
}

inline QgsMatrix4x4 operator*( double factor, const QgsMatrix4x4 &matrix )
{
  QgsMatrix4x4 m( 1 );
  m.m[0][0] = matrix.m[0][0] * factor;
  m.m[0][1] = matrix.m[0][1] * factor;
  m.m[0][2] = matrix.m[0][2] * factor;
  m.m[0][3] = matrix.m[0][3] * factor;
  m.m[1][0] = matrix.m[1][0] * factor;
  m.m[1][1] = matrix.m[1][1] * factor;
  m.m[1][2] = matrix.m[1][2] * factor;
  m.m[1][3] = matrix.m[1][3] * factor;
  m.m[2][0] = matrix.m[2][0] * factor;
  m.m[2][1] = matrix.m[2][1] * factor;
  m.m[2][2] = matrix.m[2][2] * factor;
  m.m[2][3] = matrix.m[2][3] * factor;
  m.m[3][0] = matrix.m[3][0] * factor;
  m.m[3][1] = matrix.m[3][1] * factor;
  m.m[3][2] = matrix.m[3][2] * factor;
  m.m[3][3] = matrix.m[3][3] * factor;
  m.flagBits = QgsMatrix4x4::General;
  return m;
}

inline QgsMatrix4x4 operator*( const QgsMatrix4x4 &matrix, double factor )
{
  QgsMatrix4x4 m( 1 );
  m.m[0][0] = matrix.m[0][0] * factor;
  m.m[0][1] = matrix.m[0][1] * factor;
  m.m[0][2] = matrix.m[0][2] * factor;
  m.m[0][3] = matrix.m[0][3] * factor;
  m.m[1][0] = matrix.m[1][0] * factor;
  m.m[1][1] = matrix.m[1][1] * factor;
  m.m[1][2] = matrix.m[1][2] * factor;
  m.m[1][3] = matrix.m[1][3] * factor;
  m.m[2][0] = matrix.m[2][0] * factor;
  m.m[2][1] = matrix.m[2][1] * factor;
  m.m[2][2] = matrix.m[2][2] * factor;
  m.m[2][3] = matrix.m[2][3] * factor;
  m.m[3][0] = matrix.m[3][0] * factor;
  m.m[3][1] = matrix.m[3][1] * factor;
  m.m[3][2] = matrix.m[3][2] * factor;
  m.m[3][3] = matrix.m[3][3] * factor;
  m.flagBits = QgsMatrix4x4::General;
  return m;
}

inline bool qFuzzyCompare( const QgsMatrix4x4 &m1, const QgsMatrix4x4 &m2 )
{
  return qFuzzyCompare( m1.m[0][0], m2.m[0][0] ) &&
         qFuzzyCompare( m1.m[0][1], m2.m[0][1] ) &&
         qFuzzyCompare( m1.m[0][2], m2.m[0][2] ) &&
         qFuzzyCompare( m1.m[0][3], m2.m[0][3] ) &&
         qFuzzyCompare( m1.m[1][0], m2.m[1][0] ) &&
         qFuzzyCompare( m1.m[1][1], m2.m[1][1] ) &&
         qFuzzyCompare( m1.m[1][2], m2.m[1][2] ) &&
         qFuzzyCompare( m1.m[1][3], m2.m[1][3] ) &&
         qFuzzyCompare( m1.m[2][0], m2.m[2][0] ) &&
         qFuzzyCompare( m1.m[2][1], m2.m[2][1] ) &&
         qFuzzyCompare( m1.m[2][2], m2.m[2][2] ) &&
         qFuzzyCompare( m1.m[2][3], m2.m[2][3] ) &&
         qFuzzyCompare( m1.m[3][0], m2.m[3][0] ) &&
         qFuzzyCompare( m1.m[3][1], m2.m[3][1] ) &&
         qFuzzyCompare( m1.m[3][2], m2.m[3][2] ) &&
         qFuzzyCompare( m1.m[3][3], m2.m[3][3] );
}

inline QPoint QgsMatrix4x4::map( const QPoint &point ) const
{
  return *this * point;
}

inline QPointF QgsMatrix4x4::map( const QPointF &point ) const
{
  return *this * point;
}

#ifndef QT_NO_VECTOR3D

inline QgsVector3D QgsMatrix4x4::map( const QgsVector3D &point ) const
{
  return *this * point;
}

inline QgsVector3D QgsMatrix4x4::mapVector( const QgsVector3D &vector ) const
{
  if ( flagBits < Scale )
  {
    // Translation
    return vector;
  }
  else if ( flagBits < Rotation2D )
  {
    // Translation | Scale
    return QgsVector3D( vector.x() * m[0][0],
                        vector.y() * m[1][1],
                        vector.z() * m[2][2] );
  }
  else
  {
    return QgsVector3D( vector.x() * m[0][0] +
                        vector.y() * m[1][0] +
                        vector.z() * m[2][0],
                        vector.x() * m[0][1] +
                        vector.y() * m[1][1] +
                        vector.z() * m[2][1],
                        vector.x() * m[0][2] +
                        vector.y() * m[1][2] +
                        vector.z() * m[2][2] );
  }
}

#endif

#ifndef QT_NO_VECTOR4D

inline QgsVector4D QgsMatrix4x4::map( const QgsVector4D &point ) const
{
  return *this * point;
}

#endif

inline double *QgsMatrix4x4::data()
{
  // We have to assume that the caller will modify the matrix elements,
  // so we flip it over to "General" mode.
  flagBits = General;
  return *m;
}

inline void QgsMatrix4x4::viewport( const QRectF &rect )
{
  viewport( double( rect.x() ), double( rect.y() ), double( rect.width() ), double( rect.height() ) );
}

QT_WARNING_POP

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<( QDebug dbg, const QgsMatrix4x4 &m );
#endif

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<( QDataStream &, const QgsMatrix4x4 & );
Q_GUI_EXPORT QDataStream &operator>>( QDataStream &, QgsMatrix4x4 & );
#endif

#if QT_DEPRECATED_SINCE(5, 0)
template <int N, int M>
QT_DEPRECATED QgsMatrix4x4 qGenericMatrixToMatrix4x4( const QGenericMatrix<N, M, double> &matrix )
{
  return QgsMatrix4x4( matrix.constData(), N, M );
}

template <int N, int M>
QT_DEPRECATED QGenericMatrix<N, M, double> qGenericMatrixFromMatrix4x4( const QgsMatrix4x4 &matrix )
{
  QGenericMatrix<N, M, double> result;
  const double *m = matrix.constData();
  double *values = result.data();
  for ( int col = 0; col < N; ++col )
  {
    for ( int row = 0; row < M; ++row )
    {
      if ( col < 4 && row < 4 )
        values[col * M + row] = m[col * 4 + row];
      else if ( col == row )
        values[col * M + row] = 1.0f;
      else
        values[col * M + row] = 0.0f;
    }
  }
  return result;
}
#endif

#endif
