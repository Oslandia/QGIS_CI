/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgsmatrix4x4.h"

#ifndef QT_NO_MATRIX4X4

/*!
    \class QgsMatrix4x4
    \brief The QgsMatrix4x4 class represents a 4x4 transformation matrix in 3D space.
    \since 4.6
    \ingroup painting-3D
    \inmodule QtGui

    The QgsMatrix4x4 class in general is treated as a row-major matrix, in that the
    constructors and operator() functions take data in row-major format, as is
    familiar in C-style usage.

    Internally the data is stored as column-major format, so as to be optimal for
    passing to OpenGL functions, which expect column-major data.

    When using these functions be aware that they return data in \b{column-major}
    format:
    \list
    \li data()
    \li constData()
    \endlist

    \sa QgsVector3D, QGenericMatrix
*/

static const double inv_dist_to_plane = 1.0 / 1024.0;

/*!
    \fn QgsMatrix4x4::QgsMatrix4x4()

    Constructs an identity matrix.
*/

/*!
    \fn QgsMatrix4x4::QgsMatrix4x4(Qt::Initialization)
    \since 5.5
    \internal

    Constructs a matrix without initializing the contents.
*/

/*!
    Constructs a matrix from the given 16 doubleing-point \a values.
    The contents of the array \a values is assumed to be in
    row-major order.

    If the matrix has a special type (identity, translate, scale, etc),
    the programmer should follow this constructor with a call to
    optimize() if they wish QgsMatrix4x4 to optimize further
    calls to translate(), scale(), etc.

    \sa copyDataTo(), optimize()
*/
QgsMatrix4x4::QgsMatrix4x4( const double *values )
{
  for ( int row = 0; row < 4; ++row )
    for ( int col = 0; col < 4; ++col )
      m[col][row] = values[row * 4 + col];
  flagBits = General;
}

/*!
    \fn QgsMatrix4x4::QgsMatrix4x4(double m11, double m12, double m13, double m14, double m21, double m22, double m23, double m24, double m31, double m32, double m33, double m34, double m41, double m42, double m43, double m44)

    Constructs a matrix from the 16 elements \a m11, \a m12, \a m13, \a m14,
    \a m21, \a m22, \a m23, \a m24, \a m31, \a m32, \a m33, \a m34,
    \a m41, \a m42, \a m43, and \a m44.  The elements are specified in
    row-major order.

    If the matrix has a special type (identity, translate, scale, etc),
    the programmer should follow this constructor with a call to
    optimize() if they wish QgsMatrix4x4 to optimize further
    calls to translate(), scale(), etc.

    \sa optimize()
*/

/*!
    \fn template <int N, int M> QgsMatrix4x4::QgsMatrix4x4(const QGenericMatrix<N, M, double>& matrix)

    Constructs a 4x4 matrix from the left-most 4 columns and top-most
    4 rows of \a matrix.  If \a matrix has less than 4 columns or rows,
    the remaining elements are filled with elements from the identity
    matrix.

    \sa toGenericMatrix()
*/

/*!
    \fn QGenericMatrix<N, M, double> QgsMatrix4x4::toGenericMatrix() const

    Constructs a NxM generic matrix from the left-most N columns and
    top-most M rows of this 4x4 matrix.  If N or M is greater than 4,
    then the remaining elements are filled with elements from the
    identity matrix.
*/

/*!
    \internal
*/
QgsMatrix4x4::QgsMatrix4x4( const double *values, int cols, int rows )
{
  for ( int col = 0; col < 4; ++col )
  {
    for ( int row = 0; row < 4; ++row )
    {
      if ( col < cols && row < rows )
        m[col][row] = values[col * rows + row];
      else if ( col == row )
        m[col][row] = 1.0;
      else
        m[col][row] = 0.0;
    }
  }
  flagBits = General;
}

/*!
    Constructs a 4x4 matrix from the conventional Qt 2D
    transformation matrix \a transform.

    If \a transform has a special type (identity, translate, scale, etc),
    the programmer should follow this constructor with a call to
    optimize() if they wish QgsMatrix4x4 to optimize further
    calls to translate(), scale(), etc.

    \sa toTransform(), optimize()
*/
QgsMatrix4x4::QgsMatrix4x4( const QTransform &transform )
{
  m[0][0] = transform.m11();
  m[0][1] = transform.m12();
  m[0][2] = 0.0;
  m[0][3] = transform.m13();
  m[1][0] = transform.m21();
  m[1][1] = transform.m22();
  m[1][2] = 0.0;
  m[1][3] = transform.m23();
  m[2][0] = 0.0;
  m[2][1] = 0.0;
  m[2][2] = 1.0;
  m[2][3] = 0.0;
  m[3][0] = transform.dx();
  m[3][1] = transform.dy();
  m[3][2] = 0.0;
  m[3][3] = transform.m33();
  flagBits = General;
}

/*!
    \fn const double& QgsMatrix4x4::operator()(int row, int column) const

    Returns a constant reference to the element at position
    (\a row, \a column) in this matrix.

    \sa column(), row()
*/

/*!
    \fn double& QgsMatrix4x4::operator()(int row, int column)

    Returns a reference to the element at position (\a row, \a column)
    in this matrix so that the element can be assigned to.

    \sa optimize(), setColumn(), setRow()
*/

/*!
    \fn QgsVector4D QgsMatrix4x4::column(int index) const

    Returns the elements of column \a index as a 4D vector.

    \sa setColumn(), row()
*/

/*!
    \fn void QgsMatrix4x4::setColumn(int index, const QgsVector4D& value)

    Sets the elements of column \a index to the components of \a value.

    \sa column(), setRow()
*/

/*!
    \fn QgsVector4D QgsMatrix4x4::row(int index) const

    Returns the elements of row \a index as a 4D vector.

    \sa setRow(), column()
*/

/*!
    \fn void QgsMatrix4x4::setRow(int index, const QgsVector4D& value)

    Sets the elements of row \a index to the components of \a value.

    \sa row(), setColumn()
*/

/*!
    \fn bool QgsMatrix4x4::isAffine() const
    \since 5.5

    Returns \c true if this matrix is affine matrix; false otherwise.

    An affine matrix is a 4x4 matrix with row 3 equal to (0, 0, 0, 1),
    e.g. no projective coefficients.

    \sa isIdentity()
*/

/*!
    \fn bool QgsMatrix4x4::isIdentity() const

    Returns \c true if this matrix is the identity; false otherwise.

    \sa setToIdentity()
*/

/*!
    \fn void QgsMatrix4x4::setToIdentity()

    Sets this matrix to the identity.

    \sa isIdentity()
*/

/*!
    \fn void QgsMatrix4x4::fill(double value)

    Fills all elements of this matrx with \a value.
*/

static inline double matrixDet2( const double m[4][4], int col0, int col1, int row0, int row1 )
{
  return m[col0][row0] * m[col1][row1] - m[col0][row1] * m[col1][row0];
}


// The 4x4 matrix inverse algorithm is based on that described at:
// http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q24
// Some optimization has been done to avoid making copies of 3x3
// sub-matrices and to unroll the loops.

// Calculate the determinant of a 3x3 sub-matrix.
//     | A B C |
// M = | D E F |   det(M) = A * (EI - HF) - B * (DI - GF) + C * (DH - GE)
//     | G H I |
static inline double matrixDet3
( const double m[4][4], int col0, int col1, int col2,
  int row0, int row1, int row2 )
{
  return m[col0][row0] * matrixDet2( m, col1, col2, row1, row2 )
         - m[col1][row0] * matrixDet2( m, col0, col2, row1, row2 )
         + m[col2][row0] * matrixDet2( m, col0, col1, row1, row2 );
}

// Calculate the determinant of a 4x4 matrix.
static inline double matrixDet4( const double m[4][4] )
{
  double det;
  det  = m[0][0] * matrixDet3( m, 1, 2, 3, 1, 2, 3 );
  det -= m[1][0] * matrixDet3( m, 0, 2, 3, 1, 2, 3 );
  det += m[2][0] * matrixDet3( m, 0, 1, 3, 1, 2, 3 );
  det -= m[3][0] * matrixDet3( m, 0, 1, 2, 1, 2, 3 );
  return det;
}

static inline void copyToDoubles( const double m[4][4], double mm[4][4] )
{
  for ( int i = 0; i < 4; ++i )
    for ( int j = 0; j < 4; ++j )
      mm[i][j] = double( m[i][j] );
}

/*!
    Returns the determinant of this matrix.
*/
double QgsMatrix4x4::determinant() const
{
  if ( ( flagBits & ~( Translation | Rotation2D | Rotation ) ) == Identity )
    return 1.0;

  double mm[4][4];
  copyToDoubles( m, mm );
  if ( flagBits < Rotation2D )
    return mm[0][0] * mm[1][1] * mm[2][2]; // Translation | Scale
  if ( flagBits < Perspective )
    return matrixDet3( mm, 0, 1, 2, 0, 1, 2 );
  return matrixDet4( mm );
}

/*!
    Returns the inverse of this matrix.  Returns the identity if
    this matrix cannot be inverted; i.e. determinant() is zero.
    If \a invertible is not null, then true will be written to
    that location if the matrix can be inverted; false otherwise.

    If the matrix is recognized as the identity or an orthonormal
    matrix, then this function will quickly invert the matrix
    using optimized routines.

    \sa determinant(), normalMatrix()
*/
QgsMatrix4x4 QgsMatrix4x4::inverted( bool *invertible ) const
{
  // Handle some of the easy cases first.
  if ( flagBits == Identity )
  {
    if ( invertible )
      *invertible = true;
    return QgsMatrix4x4();
  }
  else if ( flagBits == Translation )
  {
    QgsMatrix4x4 inv;
    inv.m[3][0] = -m[3][0];
    inv.m[3][1] = -m[3][1];
    inv.m[3][2] = -m[3][2];
    inv.flagBits = Translation;
    if ( invertible )
      *invertible = true;
    return inv;
  }
  else if ( flagBits < Rotation2D )
  {
    // Translation | Scale
    if ( m[0][0] == 0 || m[1][1] == 0 || m[2][2] == 0 )
    {
      if ( invertible )
        *invertible = false;
      return QgsMatrix4x4();
    }
    QgsMatrix4x4 inv;
    inv.m[0][0] = 1.0 / m[0][0];
    inv.m[1][1] = 1.0 / m[1][1];
    inv.m[2][2] = 1.0 / m[2][2];
    inv.m[3][0] = -m[3][0] * inv.m[0][0];
    inv.m[3][1] = -m[3][1] * inv.m[1][1];
    inv.m[3][2] = -m[3][2] * inv.m[2][2];
    inv.flagBits = flagBits;

    if ( invertible )
      *invertible = true;
    return inv;
  }
  else if ( ( flagBits & ~( Translation | Rotation2D | Rotation ) ) == Identity )
  {
    if ( invertible )
      *invertible = true;
    return orthonormalInverse();
  }
  else if ( flagBits < Perspective )
  {
    QgsMatrix4x4 inv;

    double mm[4][4];
    copyToDoubles( m, mm );

    double det = matrixDet3( mm, 0, 1, 2, 0, 1, 2 );
    if ( det == 0.0 )
    {
      if ( invertible )
        *invertible = false;
      return QgsMatrix4x4();
    }
    det = 1.0 / det;

    inv.m[0][0] =  matrixDet2( mm, 1, 2, 1, 2 ) * det;
    inv.m[0][1] = -matrixDet2( mm, 0, 2, 1, 2 ) * det;
    inv.m[0][2] =  matrixDet2( mm, 0, 1, 1, 2 ) * det;
    inv.m[0][3] = 0;
    inv.m[1][0] = -matrixDet2( mm, 1, 2, 0, 2 ) * det;
    inv.m[1][1] =  matrixDet2( mm, 0, 2, 0, 2 ) * det;
    inv.m[1][2] = -matrixDet2( mm, 0, 1, 0, 2 ) * det;
    inv.m[1][3] = 0;
    inv.m[2][0] =  matrixDet2( mm, 1, 2, 0, 1 ) * det;
    inv.m[2][1] = -matrixDet2( mm, 0, 2, 0, 1 ) * det;
    inv.m[2][2] =  matrixDet2( mm, 0, 1, 0, 1 ) * det;
    inv.m[2][3] = 0;
    inv.m[3][0] = -inv.m[0][0] * m[3][0] - inv.m[1][0] * m[3][1] - inv.m[2][0] * m[3][2];
    inv.m[3][1] = -inv.m[0][1] * m[3][0] - inv.m[1][1] * m[3][1] - inv.m[2][1] * m[3][2];
    inv.m[3][2] = -inv.m[0][2] * m[3][0] - inv.m[1][2] * m[3][1] - inv.m[2][2] * m[3][2];
    inv.m[3][3] = 1;
    inv.flagBits = flagBits;

    if ( invertible )
      *invertible = true;
    return inv;
  }

  QgsMatrix4x4 inv;

  double mm[4][4];
  copyToDoubles( m, mm );

  double det = matrixDet4( mm );
  if ( det == 0.0 )
  {
    if ( invertible )
      *invertible = false;
    return QgsMatrix4x4();
  }
  det = 1.0 / det;

  inv.m[0][0] =  matrixDet3( mm, 1, 2, 3, 1, 2, 3 ) * det;
  inv.m[0][1] = -matrixDet3( mm, 0, 2, 3, 1, 2, 3 ) * det;
  inv.m[0][2] =  matrixDet3( mm, 0, 1, 3, 1, 2, 3 ) * det;
  inv.m[0][3] = -matrixDet3( mm, 0, 1, 2, 1, 2, 3 ) * det;
  inv.m[1][0] = -matrixDet3( mm, 1, 2, 3, 0, 2, 3 ) * det;
  inv.m[1][1] =  matrixDet3( mm, 0, 2, 3, 0, 2, 3 ) * det;
  inv.m[1][2] = -matrixDet3( mm, 0, 1, 3, 0, 2, 3 ) * det;
  inv.m[1][3] =  matrixDet3( mm, 0, 1, 2, 0, 2, 3 ) * det;
  inv.m[2][0] =  matrixDet3( mm, 1, 2, 3, 0, 1, 3 ) * det;
  inv.m[2][1] = -matrixDet3( mm, 0, 2, 3, 0, 1, 3 ) * det;
  inv.m[2][2] =  matrixDet3( mm, 0, 1, 3, 0, 1, 3 ) * det;
  inv.m[2][3] = -matrixDet3( mm, 0, 1, 2, 0, 1, 3 ) * det;
  inv.m[3][0] = -matrixDet3( mm, 1, 2, 3, 0, 1, 2 ) * det;
  inv.m[3][1] =  matrixDet3( mm, 0, 2, 3, 0, 1, 2 ) * det;
  inv.m[3][2] = -matrixDet3( mm, 0, 1, 3, 0, 1, 2 ) * det;
  inv.m[3][3] =  matrixDet3( mm, 0, 1, 2, 0, 1, 2 ) * det;
  inv.flagBits = flagBits;

  if ( invertible )
    *invertible = true;
  return inv;
}

/*!
    Returns the normal matrix corresponding to this 4x4 transformation.
    The normal matrix is the transpose of the inverse of the top-left
    3x3 part of this 4x4 matrix.  If the 3x3 sub-matrix is not invertible,
    this function returns the identity.

    \sa inverted()
*/
/*
QgsMatrix3x3 QgsMatrix4x4::normalMatrix() const
{
    QgsMatrix3x3 inv;

    // Handle the simple cases first.
    if (flagBits < Scale) {
        // Translation
        return inv;
    } else if (flagBits < Rotation2D) {
        // Translation | Scale
        if (m[0][0] == 0.0 || m[1][1] == 0.0 || m[2][2] == 0.0)
            return inv;
        inv.data()[0] = 1.0 / m[0][0];
        inv.data()[4] = 1.0 / m[1][1];
        inv.data()[8] = 1.0 / m[2][2];
        return inv;
    } else if ((flagBits & ~(Translation | Rotation2D | Rotation)) == Identity) {
        double *invm = inv.data();
        invm[0 + 0 * 3] = m[0][0];
        invm[1 + 0 * 3] = m[0][1];
        invm[2 + 0 * 3] = m[0][2];
        invm[0 + 1 * 3] = m[1][0];
        invm[1 + 1 * 3] = m[1][1];
        invm[2 + 1 * 3] = m[1][2];
        invm[0 + 2 * 3] = m[2][0];
        invm[1 + 2 * 3] = m[2][1];
        invm[2 + 2 * 3] = m[2][2];
        return inv;
    }

    double mm[4][4];
    copyToDoubles(m, mm);
    double det = matrixDet3(mm, 0, 1, 2, 0, 1, 2);
    if (det == 0.0)
        return inv;
    det = 1.0 / det;

    double *invm = inv.data();

    // Invert and transpose in a single step.
    invm[0 + 0 * 3] =  (mm[1][1] * mm[2][2] - mm[2][1] * mm[1][2]) * det;
    invm[1 + 0 * 3] = -(mm[1][0] * mm[2][2] - mm[1][2] * mm[2][0]) * det;
    invm[2 + 0 * 3] =  (mm[1][0] * mm[2][1] - mm[1][1] * mm[2][0]) * det;
    invm[0 + 1 * 3] = -(mm[0][1] * mm[2][2] - mm[2][1] * mm[0][2]) * det;
    invm[1 + 1 * 3] =  (mm[0][0] * mm[2][2] - mm[0][2] * mm[2][0]) * det;
    invm[2 + 1 * 3] = -(mm[0][0] * mm[2][1] - mm[0][1] * mm[2][0]) * det;
    invm[0 + 2 * 3] =  (mm[0][1] * mm[1][2] - mm[0][2] * mm[1][1]) * det;
    invm[1 + 2 * 3] = -(mm[0][0] * mm[1][2] - mm[0][2] * mm[1][0]) * det;
    invm[2 + 2 * 3] =  (mm[0][0] * mm[1][1] - mm[1][0] * mm[0][1]) * det;

    return inv;
}
*/
/*!
    Returns this matrix, transposed about its diagonal.
*/
QgsMatrix4x4 QgsMatrix4x4::transposed() const
{
  QgsMatrix4x4 result;
  for ( int row = 0; row < 4; ++row )
  {
    for ( int col = 0; col < 4; ++col )
    {
      result.m[col][row] = m[row][col];
    }
  }
  // When a translation is transposed, it becomes a perspective transformation.
  result.flagBits = ( flagBits & Translation ? General : flagBits );
  return result;
}

/*!
    \fn QgsMatrix4x4& QgsMatrix4x4::operator+=(const QgsMatrix4x4& other)

    Adds the contents of \a other to this matrix.
*/

/*!
    \fn QgsMatrix4x4& QgsMatrix4x4::operator-=(const QgsMatrix4x4& other)

    Subtracts the contents of \a other from this matrix.
*/

/*!
    \fn QgsMatrix4x4& QgsMatrix4x4::operator*=(const QgsMatrix4x4& other)

    Multiplies the contents of \a other by this matrix.
*/

/*!
    \fn QgsMatrix4x4& QgsMatrix4x4::operator*=(double factor)
    \overload

    Multiplies all elements of this matrix by \a factor.
*/

/*!
    \overload

    Divides all elements of this matrix by \a divisor.
*/
QgsMatrix4x4 &QgsMatrix4x4::operator/=( double divisor )
{
  m[0][0] /= divisor;
  m[0][1] /= divisor;
  m[0][2] /= divisor;
  m[0][3] /= divisor;
  m[1][0] /= divisor;
  m[1][1] /= divisor;
  m[1][2] /= divisor;
  m[1][3] /= divisor;
  m[2][0] /= divisor;
  m[2][1] /= divisor;
  m[2][2] /= divisor;
  m[2][3] /= divisor;
  m[3][0] /= divisor;
  m[3][1] /= divisor;
  m[3][2] /= divisor;
  m[3][3] /= divisor;
  flagBits = General;
  return *this;
}

/*!
    \fn bool QgsMatrix4x4::operator==(const QgsMatrix4x4& other) const

    Returns \c true if this matrix is identical to \a other; false otherwise.
    This operator uses an exact doubleing-point comparison.
*/

/*!
    \fn bool QgsMatrix4x4::operator!=(const QgsMatrix4x4& other) const

    Returns \c true if this matrix is not identical to \a other; false otherwise.
    This operator uses an exact doubleing-point comparison.
*/

/*!
    \fn QgsMatrix4x4 operator+(const QgsMatrix4x4& m1, const QgsMatrix4x4& m2)
    \relates QgsMatrix4x4

    Returns the sum of \a m1 and \a m2.
*/

/*!
    \fn QgsMatrix4x4 operator-(const QgsMatrix4x4& m1, const QgsMatrix4x4& m2)
    \relates QgsMatrix4x4

    Returns the difference of \a m1 and \a m2.
*/

/*!
    \fn QgsMatrix4x4 operator*(const QgsMatrix4x4& m1, const QgsMatrix4x4& m2)
    \relates QgsMatrix4x4

    Returns the product of \a m1 and \a m2.
*/

#ifndef QT_NO_VECTOR3D

/*!
    \fn QgsVector3D operator*(const QgsVector3D& vector, const QgsMatrix4x4& matrix)
    \relates QgsMatrix4x4

    Returns the result of transforming \a vector according to \a matrix,
    with the matrix applied post-vector.
*/

/*!
    \fn QgsVector3D operator*(const QgsMatrix4x4& matrix, const QgsVector3D& vector)
    \relates QgsMatrix4x4

    Returns the result of transforming \a vector according to \a matrix,
    with the matrix applied pre-vector.
*/

#endif

#ifndef QT_NO_VECTOR4D

/*!
    \fn QgsVector4D operator*(const QgsVector4D& vector, const QgsMatrix4x4& matrix)
    \relates QgsMatrix4x4

    Returns the result of transforming \a vector according to \a matrix,
    with the matrix applied post-vector.
*/

/*!
    \fn QgsVector4D operator*(const QgsMatrix4x4& matrix, const QgsVector4D& vector)
    \relates QgsMatrix4x4

    Returns the result of transforming \a vector according to \a matrix,
    with the matrix applied pre-vector.
*/

#endif

/*!
    \fn QPoint operator*(const QPoint& point, const QgsMatrix4x4& matrix)
    \relates QgsMatrix4x4

    Returns the result of transforming \a point according to \a matrix,
    with the matrix applied post-point.
*/

/*!
    \fn QPointF operator*(const QPointF& point, const QgsMatrix4x4& matrix)
    \relates QgsMatrix4x4

    Returns the result of transforming \a point according to \a matrix,
    with the matrix applied post-point.
*/

/*!
    \fn QPoint operator*(const QgsMatrix4x4& matrix, const QPoint& point)
    \relates QgsMatrix4x4

    Returns the result of transforming \a point according to \a matrix,
    with the matrix applied pre-point.
*/

/*!
    \fn QPointF operator*(const QgsMatrix4x4& matrix, const QPointF& point)
    \relates QgsMatrix4x4

    Returns the result of transforming \a point according to \a matrix,
    with the matrix applied pre-point.
*/

/*!
    \fn QgsMatrix4x4 operator-(const QgsMatrix4x4& matrix)
    \overload
    \relates QgsMatrix4x4

    Returns the negation of \a matrix.
*/

/*!
    \fn QgsMatrix4x4 operator*(double factor, const QgsMatrix4x4& matrix)
    \relates QgsMatrix4x4

    Returns the result of multiplying all elements of \a matrix by \a factor.
*/

/*!
    \fn QgsMatrix4x4 operator*(const QgsMatrix4x4& matrix, double factor)
    \relates QgsMatrix4x4

    Returns the result of multiplying all elements of \a matrix by \a factor.
*/

/*!
    \relates QgsMatrix4x4

    Returns the result of dividing all elements of \a matrix by \a divisor.
*/
QgsMatrix4x4 operator/( const QgsMatrix4x4 &matrix, double divisor )
{
  QgsMatrix4x4 m;
  m.m[0][0] = matrix.m[0][0] / divisor;
  m.m[0][1] = matrix.m[0][1] / divisor;
  m.m[0][2] = matrix.m[0][2] / divisor;
  m.m[0][3] = matrix.m[0][3] / divisor;
  m.m[1][0] = matrix.m[1][0] / divisor;
  m.m[1][1] = matrix.m[1][1] / divisor;
  m.m[1][2] = matrix.m[1][2] / divisor;
  m.m[1][3] = matrix.m[1][3] / divisor;
  m.m[2][0] = matrix.m[2][0] / divisor;
  m.m[2][1] = matrix.m[2][1] / divisor;
  m.m[2][2] = matrix.m[2][2] / divisor;
  m.m[2][3] = matrix.m[2][3] / divisor;
  m.m[3][0] = matrix.m[3][0] / divisor;
  m.m[3][1] = matrix.m[3][1] / divisor;
  m.m[3][2] = matrix.m[3][2] / divisor;
  m.m[3][3] = matrix.m[3][3] / divisor;
  m.flagBits = QgsMatrix4x4::General;
  return m;
}



#ifndef QT_NO_VECTOR3D

/*!
    Multiplies this matrix by another that scales coordinates by
    the components of \a vector.

    \sa translate(), rotate()
*/
void QgsMatrix4x4::scale( const QgsVector3D &vector )
{
  double vx = vector.x();
  double vy = vector.y();
  double vz = vector.z();
  if ( flagBits < Scale )
  {
    m[0][0] = vx;
    m[1][1] = vy;
    m[2][2] = vz;
  }
  else if ( flagBits < Rotation2D )
  {
    m[0][0] *= vx;
    m[1][1] *= vy;
    m[2][2] *= vz;
  }
  else if ( flagBits < Rotation )
  {
    m[0][0] *= vx;
    m[0][1] *= vx;
    m[1][0] *= vy;
    m[1][1] *= vy;
    m[2][2] *= vz;
  }
  else
  {
    m[0][0] *= vx;
    m[0][1] *= vx;
    m[0][2] *= vx;
    m[0][3] *= vx;
    m[1][0] *= vy;
    m[1][1] *= vy;
    m[1][2] *= vy;
    m[1][3] *= vy;
    m[2][0] *= vz;
    m[2][1] *= vz;
    m[2][2] *= vz;
    m[2][3] *= vz;
  }
  flagBits |= Scale;
}

#endif

/*!
    \overload

    Multiplies this matrix by another that scales coordinates by the
    components \a x, and \a y.

    \sa translate(), rotate()
*/
void QgsMatrix4x4::scale( double x, double y )
{
  if ( flagBits < Scale )
  {
    m[0][0] = x;
    m[1][1] = y;
  }
  else if ( flagBits < Rotation2D )
  {
    m[0][0] *= x;
    m[1][1] *= y;
  }
  else if ( flagBits < Rotation )
  {
    m[0][0] *= x;
    m[0][1] *= x;
    m[1][0] *= y;
    m[1][1] *= y;
  }
  else
  {
    m[0][0] *= x;
    m[0][1] *= x;
    m[0][2] *= x;
    m[0][3] *= x;
    m[1][0] *= y;
    m[1][1] *= y;
    m[1][2] *= y;
    m[1][3] *= y;
  }
  flagBits |= Scale;
}

/*!
    \overload

    Multiplies this matrix by another that scales coordinates by the
    components \a x, \a y, and \a z.

    \sa translate(), rotate()
*/
void QgsMatrix4x4::scale( double x, double y, double z )
{
  if ( flagBits < Scale )
  {
    m[0][0] = x;
    m[1][1] = y;
    m[2][2] = z;
  }
  else if ( flagBits < Rotation2D )
  {
    m[0][0] *= x;
    m[1][1] *= y;
    m[2][2] *= z;
  }
  else if ( flagBits < Rotation )
  {
    m[0][0] *= x;
    m[0][1] *= x;
    m[1][0] *= y;
    m[1][1] *= y;
    m[2][2] *= z;
  }
  else
  {
    m[0][0] *= x;
    m[0][1] *= x;
    m[0][2] *= x;
    m[0][3] *= x;
    m[1][0] *= y;
    m[1][1] *= y;
    m[1][2] *= y;
    m[1][3] *= y;
    m[2][0] *= z;
    m[2][1] *= z;
    m[2][2] *= z;
    m[2][3] *= z;
  }
  flagBits |= Scale;
}

/*!
    \overload

    Multiplies this matrix by another that scales coordinates by the
    given \a factor.

    \sa translate(), rotate()
*/
void QgsMatrix4x4::scale( double factor )
{
  if ( flagBits < Scale )
  {
    m[0][0] = factor;
    m[1][1] = factor;
    m[2][2] = factor;
  }
  else if ( flagBits < Rotation2D )
  {
    m[0][0] *= factor;
    m[1][1] *= factor;
    m[2][2] *= factor;
  }
  else if ( flagBits < Rotation )
  {
    m[0][0] *= factor;
    m[0][1] *= factor;
    m[1][0] *= factor;
    m[1][1] *= factor;
    m[2][2] *= factor;
  }
  else
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
  }
  flagBits |= Scale;
}

#ifndef QT_NO_VECTOR3D
/*!
    Multiplies this matrix by another that translates coordinates by
    the components of \a vector.

    \sa scale(), rotate()
*/

void QgsMatrix4x4::translate( const QgsVector3D &vector )
{
  double vx = vector.x();
  double vy = vector.y();
  double vz = vector.z();
  if ( flagBits == Identity )
  {
    m[3][0] = vx;
    m[3][1] = vy;
    m[3][2] = vz;
  }
  else if ( flagBits == Translation )
  {
    m[3][0] += vx;
    m[3][1] += vy;
    m[3][2] += vz;
  }
  else if ( flagBits == Scale )
  {
    m[3][0] = m[0][0] * vx;
    m[3][1] = m[1][1] * vy;
    m[3][2] = m[2][2] * vz;
  }
  else if ( flagBits == ( Translation | Scale ) )
  {
    m[3][0] += m[0][0] * vx;
    m[3][1] += m[1][1] * vy;
    m[3][2] += m[2][2] * vz;
  }
  else if ( flagBits < Rotation )
  {
    m[3][0] += m[0][0] * vx + m[1][0] * vy;
    m[3][1] += m[0][1] * vx + m[1][1] * vy;
    m[3][2] += m[2][2] * vz;
  }
  else
  {
    m[3][0] += m[0][0] * vx + m[1][0] * vy + m[2][0] * vz;
    m[3][1] += m[0][1] * vx + m[1][1] * vy + m[2][1] * vz;
    m[3][2] += m[0][2] * vx + m[1][2] * vy + m[2][2] * vz;
    m[3][3] += m[0][3] * vx + m[1][3] * vy + m[2][3] * vz;
  }
  flagBits |= Translation;
}
#endif

/*!
    \overload

    Multiplies this matrix by another that translates coordinates
    by the components \a x, and \a y.

    \sa scale(), rotate()
*/
void QgsMatrix4x4::translate( double x, double y )
{
  if ( flagBits == Identity )
  {
    m[3][0] = x;
    m[3][1] = y;
  }
  else if ( flagBits == Translation )
  {
    m[3][0] += x;
    m[3][1] += y;
  }
  else if ( flagBits == Scale )
  {
    m[3][0] = m[0][0] * x;
    m[3][1] = m[1][1] * y;
  }
  else if ( flagBits == ( Translation | Scale ) )
  {
    m[3][0] += m[0][0] * x;
    m[3][1] += m[1][1] * y;
  }
  else if ( flagBits < Rotation )
  {
    m[3][0] += m[0][0] * x + m[1][0] * y;
    m[3][1] += m[0][1] * x + m[1][1] * y;
  }
  else
  {
    m[3][0] += m[0][0] * x + m[1][0] * y;
    m[3][1] += m[0][1] * x + m[1][1] * y;
    m[3][2] += m[0][2] * x + m[1][2] * y;
    m[3][3] += m[0][3] * x + m[1][3] * y;
  }
  flagBits |= Translation;
}

/*!
    \overload

    Multiplies this matrix by another that translates coordinates
    by the components \a x, \a y, and \a z.

    \sa scale(), rotate()
*/
void QgsMatrix4x4::translate( double x, double y, double z )
{
  if ( flagBits == Identity )
  {
    m[3][0] = x;
    m[3][1] = y;
    m[3][2] = z;
  }
  else if ( flagBits == Translation )
  {
    m[3][0] += x;
    m[3][1] += y;
    m[3][2] += z;
  }
  else if ( flagBits == Scale )
  {
    m[3][0] = m[0][0] * x;
    m[3][1] = m[1][1] * y;
    m[3][2] = m[2][2] * z;
  }
  else if ( flagBits == ( Translation | Scale ) )
  {
    m[3][0] += m[0][0] * x;
    m[3][1] += m[1][1] * y;
    m[3][2] += m[2][2] * z;
  }
  else if ( flagBits < Rotation )
  {
    m[3][0] += m[0][0] * x + m[1][0] * y;
    m[3][1] += m[0][1] * x + m[1][1] * y;
    m[3][2] += m[2][2] * z;
  }
  else
  {
    m[3][0] += m[0][0] * x + m[1][0] * y + m[2][0] * z;
    m[3][1] += m[0][1] * x + m[1][1] * y + m[2][1] * z;
    m[3][2] += m[0][2] * x + m[1][2] * y + m[2][2] * z;
    m[3][3] += m[0][3] * x + m[1][3] * y + m[2][3] * z;
  }
  flagBits |= Translation;
}

#ifndef QT_NO_VECTOR3D
/*!
    Multiples this matrix by another that rotates coordinates through
    \a angle degrees about \a vector.

    \sa scale(), translate()
*/

void QgsMatrix4x4::rotate( double angle, const QgsVector3D &vector )
{
  rotate( angle, vector.x(), vector.y(), vector.z() );
}

#endif

/*!
    \overload

    Multiplies this matrix by another that rotates coordinates through
    \a angle degrees about the vector (\a x, \a y, \a z).

    \sa scale(), translate()
*/
void QgsMatrix4x4::rotate( double angle, double x, double y, double z )
{
  if ( angle == 0.0 )
    return;
  double c, s;
  if ( angle == 90.0 || angle == -270.0 )
  {
    s = 1.0;
    c = 0.0;
  }
  else if ( angle == -90.0 || angle == 270.0 )
  {
    s = -1.0;
    c = 0.0;
  }
  else if ( angle == 180.0 || angle == -180.0 )
  {
    s = 0.0;
    c = -1.0;
  }
  else
  {
    double a = qDegreesToRadians( angle );
    c = std::cos( a );
    s = std::sin( a );
  }
  if ( x == 0.0 )
  {
    if ( y == 0.0 )
    {
      if ( z != 0.0 )
      {
        // Rotate around the Z axis.
        if ( z < 0 )
          s = -s;
        double tmp;
        m[0][0] = ( tmp = m[0][0] ) * c + m[1][0] * s;
        m[1][0] = m[1][0] * c - tmp * s;
        m[0][1] = ( tmp = m[0][1] ) * c + m[1][1] * s;
        m[1][1] = m[1][1] * c - tmp * s;
        m[0][2] = ( tmp = m[0][2] ) * c + m[1][2] * s;
        m[1][2] = m[1][2] * c - tmp * s;
        m[0][3] = ( tmp = m[0][3] ) * c + m[1][3] * s;
        m[1][3] = m[1][3] * c - tmp * s;

        flagBits |= Rotation2D;
        return;
      }
    }
    else if ( z == 0.0 )
    {
      // Rotate around the Y axis.
      if ( y < 0 )
        s = -s;
      double tmp;
      m[2][0] = ( tmp = m[2][0] ) * c + m[0][0] * s;
      m[0][0] = m[0][0] * c - tmp * s;
      m[2][1] = ( tmp = m[2][1] ) * c + m[0][1] * s;
      m[0][1] = m[0][1] * c - tmp * s;
      m[2][2] = ( tmp = m[2][2] ) * c + m[0][2] * s;
      m[0][2] = m[0][2] * c - tmp * s;
      m[2][3] = ( tmp = m[2][3] ) * c + m[0][3] * s;
      m[0][3] = m[0][3] * c - tmp * s;

      flagBits |= Rotation;
      return;
    }
  }
  else if ( y == 0.0 && z == 0.0 )
  {
    // Rotate around the X axis.
    if ( x < 0 )
      s = -s;
    double tmp;
    m[1][0] = ( tmp = m[1][0] ) * c + m[2][0] * s;
    m[2][0] = m[2][0] * c - tmp * s;
    m[1][1] = ( tmp = m[1][1] ) * c + m[2][1] * s;
    m[2][1] = m[2][1] * c - tmp * s;
    m[1][2] = ( tmp = m[1][2] ) * c + m[2][2] * s;
    m[2][2] = m[2][2] * c - tmp * s;
    m[1][3] = ( tmp = m[1][3] ) * c + m[2][3] * s;
    m[2][3] = m[2][3] * c - tmp * s;

    flagBits |= Rotation;
    return;
  }

  double len = double( x ) * double( x ) +
               double( y ) * double( y ) +
               double( z ) * double( z );
  if ( !qFuzzyCompare( len, 1.0 ) && !qFuzzyIsNull( len ) )
  {
    len = std::sqrt( len );
    x = double( double( x ) / len );
    y = double( double( y ) / len );
    z = double( double( z ) / len );
  }
  double ic = 1.0 - c;
  QgsMatrix4x4 rot;
  rot.m[0][0] = x * x * ic + c;
  rot.m[1][0] = x * y * ic - z * s;
  rot.m[2][0] = x * z * ic + y * s;
  rot.m[3][0] = 0.0;
  rot.m[0][1] = y * x * ic + z * s;
  rot.m[1][1] = y * y * ic + c;
  rot.m[2][1] = y * z * ic - x * s;
  rot.m[3][1] = 0.0;
  rot.m[0][2] = x * z * ic - y * s;
  rot.m[1][2] = y * z * ic + x * s;
  rot.m[2][2] = z * z * ic + c;
  rot.m[3][2] = 0.0;
  rot.m[0][3] = 0.0;
  rot.m[1][3] = 0.0;
  rot.m[2][3] = 0.0;
  rot.m[3][3] = 1.0;
  rot.flagBits = Rotation;
  *this *= rot;
}

/*!
    \internal
*/
void QgsMatrix4x4::projectedRotate( double angle, double x, double y, double z )
{
  // Used by QGraphicsRotation::applyTo() to perform a rotation
  // and projection back to 2D in a single step.
  if ( angle == 0.0 )
    return;
  double c, s;
  if ( angle == 90.0 || angle == -270.0 )
  {
    s = 1.0;
    c = 0.0;
  }
  else if ( angle == -90.0 || angle == 270.0 )
  {
    s = -1.0;
    c = 0.0;
  }
  else if ( angle == 180.0 || angle == -180.0 )
  {
    s = 0.0;
    c = -1.0;
  }
  else
  {
    double a = qDegreesToRadians( angle );
    c = std::cos( a );
    s = std::sin( a );
  }
  if ( x == 0.0 )
  {
    if ( y == 0.0 )
    {
      if ( z != 0.0 )
      {
        // Rotate around the Z axis.
        if ( z < 0 )
          s = -s;
        double tmp;
        m[0][0] = ( tmp = m[0][0] ) * c + m[1][0] * s;
        m[1][0] = m[1][0] * c - tmp * s;
        m[0][1] = ( tmp = m[0][1] ) * c + m[1][1] * s;
        m[1][1] = m[1][1] * c - tmp * s;
        m[0][2] = ( tmp = m[0][2] ) * c + m[1][2] * s;
        m[1][2] = m[1][2] * c - tmp * s;
        m[0][3] = ( tmp = m[0][3] ) * c + m[1][3] * s;
        m[1][3] = m[1][3] * c - tmp * s;

        flagBits |= Rotation2D;
        return;
      }
    }
    else if ( z == 0.0 )
    {
      // Rotate around the Y axis.
      if ( y < 0 )
        s = -s;
      m[0][0] = m[0][0] * c + m[3][0] * s * inv_dist_to_plane;
      m[0][1] = m[0][1] * c + m[3][1] * s * inv_dist_to_plane;
      m[0][2] = m[0][2] * c + m[3][2] * s * inv_dist_to_plane;
      m[0][3] = m[0][3] * c + m[3][3] * s * inv_dist_to_plane;
      flagBits = General;
      return;
    }
  }
  else if ( y == 0.0 && z == 0.0 )
  {
    // Rotate around the X axis.
    if ( x < 0 )
      s = -s;
    m[1][0] = m[1][0] * c - m[3][0] * s * inv_dist_to_plane;
    m[1][1] = m[1][1] * c - m[3][1] * s * inv_dist_to_plane;
    m[1][2] = m[1][2] * c - m[3][2] * s * inv_dist_to_plane;
    m[1][3] = m[1][3] * c - m[3][3] * s * inv_dist_to_plane;
    flagBits = General;
    return;
  }
  double len = double( x ) * double( x ) +
               double( y ) * double( y ) +
               double( z ) * double( z );
  if ( !qFuzzyCompare( len, 1.0 ) && !qFuzzyIsNull( len ) )
  {
    len = std::sqrt( len );
    x = double( double( x ) / len );
    y = double( double( y ) / len );
    z = double( double( z ) / len );
  }
  const double ic = 1.0 - c;
  QgsMatrix4x4 rot;
  rot.m[0][0] = x * x * ic + c;
  rot.m[1][0] = x * y * ic - z * s;
  rot.m[2][0] = 0.0;
  rot.m[3][0] = 0.0;
  rot.m[0][1] = y * x * ic + z * s;
  rot.m[1][1] = y * y * ic + c;
  rot.m[2][1] = 0.0;
  rot.m[3][1] = 0.0;
  rot.m[0][2] = 0.0;
  rot.m[1][2] = 0.0;
  rot.m[2][2] = 1.0;
  rot.m[3][2] = 0.0;
  rot.m[0][3] = ( x * z * ic - y * s ) * -inv_dist_to_plane;
  rot.m[1][3] = ( y * z * ic + x * s ) * -inv_dist_to_plane;
  rot.m[2][3] = 0.0;
  rot.m[3][3] = 1.0;
  rot.flagBits = General;
  *this *= rot;
}

/*!
    \fn int QgsMatrix4x4::flags() const
    \internal
*/

/*!
    \enum QgsMatrix4x4::Flags
    \internal
    \omitvalue Identity
    \omitvalue Translation
    \omitvalue Scale
    \omitvalue Rotation2D
    \omitvalue Rotation
    \omitvalue Perspective
    \omitvalue General
*/

#ifndef QT_NO_QUATERNION

/*!
    Multiples this matrix by another that rotates coordinates according
    to a specified \a quaternion.  The \a quaternion is assumed to have
    been normalized.

    \sa scale(), translate(), QQuaternion
*/
void QgsMatrix4x4::rotate( const QQuaternion &quaternion )
{
  // Algorithm from:
  // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q54

  QgsMatrix4x4 m;

  const double f2x = quaternion.x() + quaternion.x();
  const double f2y = quaternion.y() + quaternion.y();
  const double f2z = quaternion.z() + quaternion.z();
  const double f2xw = f2x * quaternion.scalar();
  const double f2yw = f2y * quaternion.scalar();
  const double f2zw = f2z * quaternion.scalar();
  const double f2xx = f2x * quaternion.x();
  const double f2xy = f2x * quaternion.y();
  const double f2xz = f2x * quaternion.z();
  const double f2yy = f2y * quaternion.y();
  const double f2yz = f2y * quaternion.z();
  const double f2zz = f2z * quaternion.z();

  m.m[0][0] = 1.0 - ( f2yy + f2zz );
  m.m[1][0] =         f2xy - f2zw;
  m.m[2][0] =         f2xz + f2yw;
  m.m[3][0] = 0.0;
  m.m[0][1] =         f2xy + f2zw;
  m.m[1][1] = 1.0 - ( f2xx + f2zz );
  m.m[2][1] =         f2yz - f2xw;
  m.m[3][1] = 0.0;
  m.m[0][2] =         f2xz - f2yw;
  m.m[1][2] =         f2yz + f2xw;
  m.m[2][2] = 1.0 - ( f2xx + f2yy );
  m.m[3][2] = 0.0;
  m.m[0][3] = 0.0;
  m.m[1][3] = 0.0;
  m.m[2][3] = 0.0;
  m.m[3][3] = 1.0;
  m.flagBits = Rotation;
  *this *= m;
}

#endif

/*!
    \overload

    Multiplies this matrix by another that applies an orthographic
    projection for a window with boundaries specified by \a rect.
    The near and far clipping planes will be -1 and 1 respectively.

    \sa frustum(), perspective()
*/
void QgsMatrix4x4::ortho( const QRect &rect )
{
  // Note: rect.right() and rect.bottom() subtract 1 in QRect,
  // which gives the location of a pixel within the rectangle,
  // instead of the extent of the rectangle.  We want the extent.
  // QRectF expresses the extent properly.
  ortho( rect.x(), rect.x() + rect.width(), rect.y() + rect.height(), rect.y(), -1.0, 1.0 );
}

/*!
    \overload

    Multiplies this matrix by another that applies an orthographic
    projection for a window with boundaries specified by \a rect.
    The near and far clipping planes will be -1 and 1 respectively.

    \sa frustum(), perspective()
*/
void QgsMatrix4x4::ortho( const QRectF &rect )
{
  ortho( rect.left(), rect.right(), rect.bottom(), rect.top(), -1.0, 1.0 );
}

/*!
    Multiplies this matrix by another that applies an orthographic
    projection for a window with lower-left corner (\a left, \a bottom),
    upper-right corner (\a right, \a top), and the specified \a nearPlane
    and \a farPlane clipping planes.

    \sa frustum(), perspective()
*/
void QgsMatrix4x4::ortho( double left, double right, double bottom, double top, double nearPlane, double farPlane )
{
  // Bail out if the projection volume is zero-sized.
  if ( left == right || bottom == top || nearPlane == farPlane )
    return;

  // Construct the projection.
  const double width = right - left;
  const double invheight = top - bottom;
  const double clip = farPlane - nearPlane;
  QgsMatrix4x4 m;
  m.m[0][0] = 2.0 / width;
  m.m[1][0] = 0.0;
  m.m[2][0] = 0.0;
  m.m[3][0] = -( left + right ) / width;
  m.m[0][1] = 0.0;
  m.m[1][1] = 2.0 / invheight;
  m.m[2][1] = 0.0;
  m.m[3][1] = -( top + bottom ) / invheight;
  m.m[0][2] = 0.0;
  m.m[1][2] = 0.0;
  m.m[2][2] = -2.0 / clip;
  m.m[3][2] = -( nearPlane + farPlane ) / clip;
  m.m[0][3] = 0.0;
  m.m[1][3] = 0.0;
  m.m[2][3] = 0.0;
  m.m[3][3] = 1.0;
  m.flagBits = Translation | Scale;

  // Apply the projection.
  *this *= m;
}

/*!
    Multiplies this matrix by another that applies a perspective
    frustum projection for a window with lower-left corner (\a left, \a bottom),
    upper-right corner (\a right, \a top), and the specified \a nearPlane
    and \a farPlane clipping planes.

    \sa ortho(), perspective()
*/
void QgsMatrix4x4::frustum( double left, double right, double bottom, double top, double nearPlane, double farPlane )
{
  // Bail out if the projection volume is zero-sized.
  if ( left == right || bottom == top || nearPlane == farPlane )
    return;

  // Construct the projection.
  QgsMatrix4x4 m;
  const double width = right - left;
  const double invheight = top - bottom;
  const double clip = farPlane - nearPlane;
  m.m[0][0] = 2.0 * nearPlane / width;
  m.m[1][0] = 0.0;
  m.m[2][0] = ( left + right ) / width;
  m.m[3][0] = 0.0;
  m.m[0][1] = 0.0;
  m.m[1][1] = 2.0 * nearPlane / invheight;
  m.m[2][1] = ( top + bottom ) / invheight;
  m.m[3][1] = 0.0;
  m.m[0][2] = 0.0;
  m.m[1][2] = 0.0;
  m.m[2][2] = -( nearPlane + farPlane ) / clip;
  m.m[3][2] = -2.0 * nearPlane * farPlane / clip;
  m.m[0][3] = 0.0;
  m.m[1][3] = 0.0;
  m.m[2][3] = -1.0;
  m.m[3][3] = 0.0;
  m.flagBits = General;

  // Apply the projection.
  *this *= m;
}

/*!
    Multiplies this matrix by another that applies a perspective
    projection. The vertical field of view will be \a verticalAngle degrees
    within a window with a given \a aspectRatio that determines the horizontal
    field of view.
    The projection will have the specified \a nearPlane and \a farPlane clipping
    planes which are the distances from the viewer to the corresponding planes.

    \sa ortho(), frustum()
*/
void QgsMatrix4x4::perspective( double verticalAngle, double aspectRatio, double nearPlane, double farPlane )
{
  // Bail out if the projection volume is zero-sized.
  if ( nearPlane == farPlane || aspectRatio == 0.0 )
    return;

  // Construct the projection.
  QgsMatrix4x4 m;
  const double radians = qDegreesToRadians( verticalAngle / 2.0 );
  const double sine = std::sin( radians );
  if ( sine == 0.0 )
    return;
  double cotan = std::cos( radians ) / sine;
  double clip = farPlane - nearPlane;
  m.m[0][0] = cotan / aspectRatio;
  m.m[1][0] = 0.0;
  m.m[2][0] = 0.0;
  m.m[3][0] = 0.0;
  m.m[0][1] = 0.0;
  m.m[1][1] = cotan;
  m.m[2][1] = 0.0;
  m.m[3][1] = 0.0;
  m.m[0][2] = 0.0;
  m.m[1][2] = 0.0;
  m.m[2][2] = -( nearPlane + farPlane ) / clip;
  m.m[3][2] = -( 2.0 * nearPlane * farPlane ) / clip;
  m.m[0][3] = 0.0;
  m.m[1][3] = 0.0;
  m.m[2][3] = -1.0;
  m.m[3][3] = 0.0;
  m.flagBits = General;

  // Apply the projection.
  *this *= m;
}

#ifndef QT_NO_VECTOR3D

/*!
    Multiplies this matrix by a viewing matrix derived from an eye
    point. The \a center value indicates the center of the view that
    the \a eye is looking at.  The \a up value indicates which direction
    should be considered up with respect to the \a eye.

    \note The \a up vector must not be parallel to the line of sight
    from \a eye to \a center.
*/
void QgsMatrix4x4::lookAt( const QgsVector3D &eye, const QgsVector3D &center, const QgsVector3D &up )
{
  QgsVector3D forward = center - eye;
  if ( qFuzzyIsNull( forward.x() ) && qFuzzyIsNull( forward.y() ) && qFuzzyIsNull( forward.z() ) )
    return;

  forward.normalize();
  QgsVector3D side = QgsVector3D::crossProduct( forward, up );
  side.normalize();
  QgsVector3D upVector = QgsVector3D::crossProduct( side, forward );

  QgsMatrix4x4 m;
  m.m[0][0] = side.x();
  m.m[1][0] = side.y();
  m.m[2][0] = side.z();
  m.m[3][0] = 0.0;
  m.m[0][1] = upVector.x();
  m.m[1][1] = upVector.y();
  m.m[2][1] = upVector.z();
  m.m[3][1] = 0.0;
  m.m[0][2] = -forward.x();
  m.m[1][2] = -forward.y();
  m.m[2][2] = -forward.z();
  m.m[3][2] = 0.0;
  m.m[0][3] = 0.0;
  m.m[1][3] = 0.0;
  m.m[2][3] = 0.0;
  m.m[3][3] = 1.0;
  m.flagBits = Rotation;

  *this *= m;
  translate( eye * -1.0 );
}

#endif

/*!
    \fn void QgsMatrix4x4::viewport(const QRectF &rect)
    \overload

    Sets up viewport transform for viewport bounded by \a rect and with near and far set
    to 0 and 1 respectively.
*/

/*!
    Multiplies this matrix by another that performs the scale and bias
    transformation used by OpenGL to transform from normalized device
    coordinates (NDC) to viewport (window) coordinates. That is it maps
    points from the cube ranging over [-1, 1] in each dimension to the
    viewport with it's near-lower-left corner at (\a left, \a bottom, \a nearPlane)
    and with size (\a width, \a height, \a farPlane - \a nearPlane).

    This matches the transform used by the fixed function OpenGL viewport
    transform controlled by the functions glViewport() and glDepthRange().
 */
void QgsMatrix4x4::viewport( double left, double bottom, double width, double height, double nearPlane, double farPlane )
{
  const double w2 = width / 2.0;
  const double h2 = height / 2.0;

  QgsMatrix4x4 m;
  m.m[0][0] = w2;
  m.m[1][0] = 0.0;
  m.m[2][0] = 0.0;
  m.m[3][0] = left + w2;
  m.m[0][1] = 0.0;
  m.m[1][1] = h2;
  m.m[2][1] = 0.0;
  m.m[3][1] = bottom + h2;
  m.m[0][2] = 0.0;
  m.m[1][2] = 0.0;
  m.m[2][2] = ( farPlane - nearPlane ) / 2.0;
  m.m[3][2] = ( nearPlane + farPlane ) / 2.0;
  m.m[0][3] = 0.0;
  m.m[1][3] = 0.0;
  m.m[2][3] = 0.0;
  m.m[3][3] = 1.0;
  m.flagBits = General;

  *this *= m;
}

/*!
    \deprecated

    Flips between right-handed and left-handed coordinate systems
    by multiplying the y and z co-ordinates by -1.  This is normally
    used to create a left-handed orthographic view without scaling
    the viewport as ortho() does.

    \sa ortho()
*/
void QgsMatrix4x4::flipCoordinates()
{
  // Multiplying the y and z coordinates with -1 does NOT flip between right-handed and
  // left-handed coordinate systems, it just rotates 180 degrees around the x axis, so
  // I'm deprecating this function.
  if ( flagBits < Rotation2D )
  {
    // Translation | Scale
    m[1][1] = -m[1][1];
    m[2][2] = -m[2][2];
  }
  else
  {
    m[1][0] = -m[1][0];
    m[1][1] = -m[1][1];
    m[1][2] = -m[1][2];
    m[1][3] = -m[1][3];
    m[2][0] = -m[2][0];
    m[2][1] = -m[2][1];
    m[2][2] = -m[2][2];
    m[2][3] = -m[2][3];
  }
  flagBits |= Scale;
}

/*!
    Retrieves the 16 items in this matrix and copies them to \a values
    in row-major order.
*/
void QgsMatrix4x4::copyDataTo( double *values ) const
{
  for ( int row = 0; row < 4; ++row )
    for ( int col = 0; col < 4; ++col )
      values[row * 4 + col] = double( m[col][row] );
}

/*!
    Returns the conventional Qt 2D transformation matrix that
    corresponds to this matrix.

    The returned QTransform is formed by simply dropping the
    third row and third column of the QgsMatrix4x4.  This is suitable
    for implementing orthographic projections where the z co-ordinate
    should be dropped rather than projected.
*/
QTransform QgsMatrix4x4::toTransform() const
{
  return QTransform( m[0][0], m[0][1], m[0][3],
                     m[1][0], m[1][1], m[1][3],
                     m[3][0], m[3][1], m[3][3] );
}

/*!
    Returns the conventional Qt 2D transformation matrix that
    corresponds to this matrix.

    If \a distanceToPlane is non-zero, it indicates a projection
    factor to use to adjust for the z co-ordinate.  The value of
    1024 corresponds to the projection factor used
    by QTransform::rotate() for the x and y axes.

    If \a distanceToPlane is zero, then the returned QTransform
    is formed by simply dropping the third row and third column
    of the QgsMatrix4x4.  This is suitable for implementing
    orthographic projections where the z co-ordinate should
    be dropped rather than projected.
*/
QTransform QgsMatrix4x4::toTransform( double distanceToPlane ) const
{
  if ( distanceToPlane == 1024.0 )
  {
    // Optimize the common case with constants.
    return QTransform( m[0][0], m[0][1], m[0][3] - m[0][2] * inv_dist_to_plane,
                       m[1][0], m[1][1], m[1][3] - m[1][2] * inv_dist_to_plane,
                       m[3][0], m[3][1], m[3][3] - m[3][2] * inv_dist_to_plane );
  }
  else if ( distanceToPlane != 0.0 )
  {
    // The following projection matrix is pre-multiplied with "matrix":
    //      | 1 0 0 0 |
    //      | 0 1 0 0 |
    //      | 0 0 1 0 |
    //      | 0 0 d 1 |
    // where d = -1 / distanceToPlane.  After projection, row 3 and
    // column 3 are dropped to form the final QTransform.
    double d = 1.0 / distanceToPlane;
    return QTransform( m[0][0], m[0][1], m[0][3] - m[0][2] * d,
                       m[1][0], m[1][1], m[1][3] - m[1][2] * d,
                       m[3][0], m[3][1], m[3][3] - m[3][2] * d );
  }
  else
  {
    // Orthographic projection: drop row 3 and column 3.
    return QTransform( m[0][0], m[0][1], m[0][3],
                       m[1][0], m[1][1], m[1][3],
                       m[3][0], m[3][1], m[3][3] );
  }
}

QMatrix4x4 QgsMatrix4x4::toQMatrix4x4( const QgsCoordinateTransform *coordTrans ) const
{
  if ( coordTrans == NULL || ( m[0][3] == 0.0 && m[1][3] == 0.0 && m[2][3] == 0.0 ) )
  {
    return QMatrix4x4( m[0][0], m[0][1], m[0][2], m[0][3],
                       m[1][0], m[1][1], m[1][2], m[1][3],
                       m[2][0], m[2][1], m[2][2], m[2][3],
                       m[3][0], m[3][1], m[3][2], m[3][3] );
  }
  else
  {
    std::vector<double> x, y, z;
    x.push_back( m[0][3] );
    y.push_back( m[1][3] );
    z.push_back( m[2][3] );
    coordTrans->transformCoords( 1, x.data(), y.data(), z.data() );
    return QMatrix4x4( m[0][0], m[0][1], m[0][2], x[0],
                       m[1][0], m[1][1], m[1][2], x[1],
                       m[2][0], m[2][1], m[2][2], x[2],
                       m[3][0], m[3][1], m[3][2], m[3][3] );
  }
}

/*!
    \fn QPoint QgsMatrix4x4::map(const QPoint& point) const

    Maps \a point by multiplying this matrix by \a point.

    \sa mapRect()
*/

/*!
    \fn QPointF QgsMatrix4x4::map(const QPointF& point) const

    Maps \a point by multiplying this matrix by \a point.

    \sa mapRect()
*/

#ifndef QT_NO_VECTOR3D

/*!
    \fn QgsVector3D QgsMatrix4x4::map(const QgsVector3D& point) const

    Maps \a point by multiplying this matrix by \a point.

    \sa mapRect(), mapVector()
*/

/*!
    \fn QgsVector3D QgsMatrix4x4::mapVector(const QgsVector3D& vector) const

    Maps \a vector by multiplying the top 3x3 portion of this matrix
    by \a vector.  The translation and projection components of
    this matrix are ignored.

    \sa map()
*/

#endif

#ifndef QT_NO_VECTOR4D

/*!
    \fn QgsVector4D QgsMatrix4x4::map(const QgsVector4D& point) const;

    Maps \a point by multiplying this matrix by \a point.

    \sa mapRect()
*/

#endif

/*!
    Maps \a rect by multiplying this matrix by the corners
    of \a rect and then forming a new rectangle from the results.
    The returned rectangle will be an ordinary 2D rectangle
    with sides parallel to the horizontal and vertical axes.

    \sa map()
*/
QRect QgsMatrix4x4::mapRect( const QRect &rect ) const
{
  if ( flagBits < Scale )
  {
    // Translation
    return QRect( qRound( rect.x() + m[3][0] ),
                  qRound( rect.y() + m[3][1] ),
                  rect.width(), rect.height() );
  }
  else if ( flagBits < Rotation2D )
  {
    // Translation | Scale
    double x = rect.x() * m[0][0] + m[3][0];
    double y = rect.y() * m[1][1] + m[3][1];
    double w = rect.width() * m[0][0];
    double h = rect.height() * m[1][1];
    if ( w < 0 )
    {
      w = -w;
      x -= w;
    }
    if ( h < 0 )
    {
      h = -h;
      y -= h;
    }
    return QRect( qRound( x ), qRound( y ), qRound( w ), qRound( h ) );
  }

  QPoint tl = map( rect.topLeft() );
  QPoint tr = map( QPoint( rect.x() + rect.width(), rect.y() ) );
  QPoint bl = map( QPoint( rect.x(), rect.y() + rect.height() ) );
  QPoint br = map( QPoint( rect.x() + rect.width(),
                           rect.y() + rect.height() ) );

  int xmin = qMin( qMin( tl.x(), tr.x() ), qMin( bl.x(), br.x() ) );
  int xmax = qMax( qMax( tl.x(), tr.x() ), qMax( bl.x(), br.x() ) );
  int ymin = qMin( qMin( tl.y(), tr.y() ), qMin( bl.y(), br.y() ) );
  int ymax = qMax( qMax( tl.y(), tr.y() ), qMax( bl.y(), br.y() ) );

  return QRect( xmin, ymin, xmax - xmin, ymax - ymin );
}

/*!
    Maps \a rect by multiplying this matrix by the corners
    of \a rect and then forming a new rectangle from the results.
    The returned rectangle will be an ordinary 2D rectangle
    with sides parallel to the horizontal and vertical axes.

    \sa map()
*/
QRectF QgsMatrix4x4::mapRect( const QRectF &rect ) const
{
  if ( flagBits < Scale )
  {
    // Translation
    return rect.translated( m[3][0], m[3][1] );
  }
  else if ( flagBits < Rotation2D )
  {
    // Translation | Scale
    double x = rect.x() * m[0][0] + m[3][0];
    double y = rect.y() * m[1][1] + m[3][1];
    double w = rect.width() * m[0][0];
    double h = rect.height() * m[1][1];
    if ( w < 0 )
    {
      w = -w;
      x -= w;
    }
    if ( h < 0 )
    {
      h = -h;
      y -= h;
    }
    return QRectF( x, y, w, h );
  }

  QPointF tl = map( rect.topLeft() ); QPointF tr = map( rect.topRight() );
  QPointF bl = map( rect.bottomLeft() ); QPointF br = map( rect.bottomRight() );

  double xmin = qMin( qMin( tl.x(), tr.x() ), qMin( bl.x(), br.x() ) );
  double xmax = qMax( qMax( tl.x(), tr.x() ), qMax( bl.x(), br.x() ) );
  double ymin = qMin( qMin( tl.y(), tr.y() ), qMin( bl.y(), br.y() ) );
  double ymax = qMax( qMax( tl.y(), tr.y() ), qMax( bl.y(), br.y() ) );

  return QRectF( QPointF( xmin, ymin ), QPointF( xmax, ymax ) );
}

/*!
    \fn double *QgsMatrix4x4::data()

    Returns a pointer to the raw data of this matrix.

    \sa constData(), optimize()
*/

/*!
    \fn const double *QgsMatrix4x4::data() const

    Returns a constant pointer to the raw data of this matrix.
    This raw data is stored in column-major format.

    \sa constData()
*/

/*!
    \fn const double *QgsMatrix4x4::constData() const

    Returns a constant pointer to the raw data of this matrix.
    This raw data is stored in column-major format.

    \sa data()
*/

// Helper routine for inverting orthonormal matrices that consist
// of just rotations and translations.
QgsMatrix4x4 QgsMatrix4x4::orthonormalInverse() const
{
  QgsMatrix4x4 result;

  result.m[0][0] = m[0][0];
  result.m[1][0] = m[0][1];
  result.m[2][0] = m[0][2];

  result.m[0][1] = m[1][0];
  result.m[1][1] = m[1][1];
  result.m[2][1] = m[1][2];

  result.m[0][2] = m[2][0];
  result.m[1][2] = m[2][1];
  result.m[2][2] = m[2][2];

  result.m[0][3] = 0.0;
  result.m[1][3] = 0.0;
  result.m[2][3] = 0.0;

  result.m[3][0] = -( result.m[0][0] * m[3][0] + result.m[1][0] * m[3][1] + result.m[2][0] * m[3][2] );
  result.m[3][1] = -( result.m[0][1] * m[3][0] + result.m[1][1] * m[3][1] + result.m[2][1] * m[3][2] );
  result.m[3][2] = -( result.m[0][2] * m[3][0] + result.m[1][2] * m[3][1] + result.m[2][2] * m[3][2] );
  result.m[3][3] = 1.0;

  result.flagBits = flagBits;

  return result;
}

/*!
    Optimize the usage of this matrix from its current elements.

    Some operations such as translate(), scale(), and rotate() can be
    performed more efficiently if the matrix being modified is already
    known to be the identity, a previous translate(), a previous
    scale(), etc.

    Normally the QgsMatrix4x4 class keeps track of this special type internally
    as operations are performed.  However, if the matrix is modified
    directly with \l {QgsMatrix4x4::}{operator()()} or data(), then
    QgsMatrix4x4 will lose track of the special type and will revert to the
    safest but least efficient operations thereafter.

    By calling optimize() after directly modifying the matrix,
    the programmer can force QgsMatrix4x4 to recover the special type if
    the elements appear to conform to one of the known optimized types.

    \sa {QgsMatrix4x4::}{operator()()}, data(), translate()
*/
void QgsMatrix4x4::optimize()
{
  // If the last row is not (0, 0, 0, 1), the matrix is not a special type.
  flagBits = General;
  if ( m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0 || m[3][3] != 1 )
    return;

  flagBits &= ~Perspective;

  // If the last column is (0, 0, 0, 1), then there is no translation.
  if ( m[3][0] == 0 && m[3][1] == 0 && m[3][2] == 0 )
    flagBits &= ~Translation;

  // If the two first elements of row 3 and column 3 are 0, then any rotation must be about Z.
  if ( !m[0][2] && !m[1][2] && !m[2][0] && !m[2][1] )
  {
    flagBits &= ~Rotation;
    // If the six non-diagonal elements in the top left 3x3 matrix are 0, there is no rotation.
    if ( !m[0][1] && !m[1][0] )
    {
      flagBits &= ~Rotation2D;
      // Check for identity.
      if ( m[0][0] == 1 && m[1][1] == 1 && m[2][2] == 1 )
        flagBits &= ~Scale;
    }
    else
    {
      // If the columns are orthonormal and form a right-handed system, then there is no scale.
      double mm[4][4];
      copyToDoubles( m, mm );
      double det = matrixDet2( mm, 0, 1, 0, 1 );
      double lenX = mm[0][0] * mm[0][0] + mm[0][1] * mm[0][1];
      double lenY = mm[1][0] * mm[1][0] + mm[1][1] * mm[1][1];
      double lenZ = mm[2][2];
      if ( qFuzzyCompare( det, 1.0 ) && qFuzzyCompare( lenX, 1.0 )
           && qFuzzyCompare( lenY, 1.0 ) && qFuzzyCompare( lenZ, 1.0 ) )
      {
        flagBits &= ~Scale;
      }
    }
  }
  else
  {
    // If the columns are orthonormal and form a right-handed system, then there is no scale.
    double mm[4][4];
    copyToDoubles( m, mm );
    double det = matrixDet3( mm, 0, 1, 2, 0, 1, 2 );
    double lenX = mm[0][0] * mm[0][0] + mm[0][1] * mm[0][1] + mm[0][2] * mm[0][2];
    double lenY = mm[1][0] * mm[1][0] + mm[1][1] * mm[1][1] + mm[1][2] * mm[1][2];
    double lenZ = mm[2][0] * mm[2][0] + mm[2][1] * mm[2][1] + mm[2][2] * mm[2][2];
    if ( qFuzzyCompare( det, 1.0 ) && qFuzzyCompare( lenX, 1.0 )
         && qFuzzyCompare( lenY, 1.0 ) && qFuzzyCompare( lenZ, 1.0 ) )
    {
      flagBits &= ~Scale;
    }
  }
}

/*!
    Returns the matrix as a QVariant.
*//*
QgsMatrix4x4::operator QVariant() const
{
    return QVariant::fromValue(*this);
}*/

#ifndef QT_NO_DEBUG_STREAM

QDebug operator<<( QDebug dbg, const QgsMatrix4x4 &m )
{
  QDebugStateSaver saver( dbg );
  // Create a string that represents the matrix type.
  QByteArray bits;
  if ( m.flagBits == QgsMatrix4x4::Identity )
  {
    bits = "Identity";
  }
  else if ( m.flagBits == QgsMatrix4x4::General )
  {
    bits = "General";
  }
  else
  {
    if ( ( m.flagBits & QgsMatrix4x4::Translation ) != 0 )
      bits += "Translation,";
    if ( ( m.flagBits & QgsMatrix4x4::Scale ) != 0 )
      bits += "Scale,";
    if ( ( m.flagBits & QgsMatrix4x4::Rotation2D ) != 0 )
      bits += "Rotation2D,";
    if ( ( m.flagBits & QgsMatrix4x4::Rotation ) != 0 )
      bits += "Rotation,";
    if ( ( m.flagBits & QgsMatrix4x4::Perspective ) != 0 )
      bits += "Perspective,";
    bits.chop( 1 );
  }

  // Output in row-major order because it is more human-readable.
  dbg.nospace() << "QgsMatrix4x4(type:" << bits.constData() << Qt::endl
                << qSetFieldWidth( 10 )
                << m( 0, 0 ) << m( 0, 1 ) << m( 0, 2 ) << m( 0, 3 ) << Qt::endl
                << m( 1, 0 ) << m( 1, 1 ) << m( 1, 2 ) << m( 1, 3 ) << Qt::endl
                << m( 2, 0 ) << m( 2, 1 ) << m( 2, 2 ) << m( 2, 3 ) << Qt::endl
                << m( 3, 0 ) << m( 3, 1 ) << m( 3, 2 ) << m( 3, 3 ) << Qt::endl
                << qSetFieldWidth( 0 ) << ')';
  return dbg;
}

#endif

#ifndef QT_NO_DATASTREAM

/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QgsMatrix4x4 &matrix)
    \relates QgsMatrix4x4

    Writes the given \a matrix to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<( QDataStream &stream, const QgsMatrix4x4 &matrix )
{
  for ( int row = 0; row < 4; ++row )
    for ( int col = 0; col < 4; ++col )
      stream << matrix( row, col );
  return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QgsMatrix4x4 &matrix)
    \relates QgsMatrix4x4

    Reads a 4x4 matrix from the given \a stream into the given \a matrix
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>( QDataStream &stream, QgsMatrix4x4 &matrix )
{
  double x;
  for ( int row = 0; row < 4; ++row )
  {
    for ( int col = 0; col < 4; ++col )
    {
      stream >> x;
      matrix( row, col ) = x;
    }
  }
  matrix.optimize();
  return stream;
}

#endif // QT_NO_DATASTREAM

#endif // QT_NO_MATRIX4X4
