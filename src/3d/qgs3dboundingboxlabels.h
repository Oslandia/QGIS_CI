#ifndef QGS3DBOUNDINGBOXLABELS_H
#define QGS3DBOUNDINGBOXLABELS_H

#define SIP_NO_FILE

#include <vector>

namespace QgsBoundingBoxLabels
{
  float coverage( float axisMin, float axisMax, float lmin, float lmax );

  float coverageMax( float axisMin, float axisMax, float span );

  float density( int k, int nrTicks, float axisMin, float axisMax, float lmin, float lmax );

  float densityMax( int k, int nrTicks );

  float simplicity( float niceNumber, std::vector<float> const &niceNumbers, int j, float lmin, float lmax, float lstep );

  float simplicityMax( float niceNumber, std::vector<float> const &niceNumbers, int j );

  void extendedWilkinsonAlgorithm( float axisMin, float axisMax, int nrTicks, bool extend, float &rangeMin, float &rangeMax, float &rangeStep );

}

#endif // QGS3DBOUNDINGBOXLABELS_H
