#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "qgs3dboundingboxlabels.h"

namespace QgsBoundingBoxLabels
{

  float coverage( float axisMin, float axisMmax, float lmin, float lmax )
  {
    float axisRange = axisMmax - axisMin;
    return 1.0f - 0.5f * ( std::pow( axisMmax - lmax, 2.0f ) + std::pow( axisMin - lmin, 2.0f ) ) / std::pow( 0.1f * axisRange, 2.0f );
  }

  float coverageMax( float axisMin, float axisMmax, float span )
  {
    float axisRange = axisMmax - axisMin;
    if ( span > axisRange )
    {
      float half = ( span - axisRange ) / 2.0f;
      return 1.0f - 0.5f * ( 2.0f * std::pow( half, 2.0f ) ) / std::pow( 0.1f * axisRange, 2.0f );
    }
    else
      return 1.0f;
  }

  float density( int k, int nrTicks, float axisMin, float axisMmax, float lmin, float lmax )
  {
    float r = ( k - 1.0f ) / ( lmax - lmin );
    float rt = ( nrTicks - 1.0f ) / ( std::max( lmax, axisMmax ) - std::min( axisMin, lmin ) );
    return 2.0f - std::max( r / rt, rt / r );
  }

  float densityMax( int k, int nrTicks )
  {
    if ( k >= nrTicks )
      return 2.0f - ( k - 1.0f ) / ( nrTicks - 1.0f );
    else
      return 1.0f;
  }

  float simplicity( float niceNumber, std::vector<float> const &niceNumbers, int j, float lmin, float lmax, float lstep )
  {
    float epsilon = 100.0f * std::numeric_limits<float>::epsilon();
    int nrIndex = std::find( niceNumbers.begin(), niceNumbers.end(), niceNumber ) - niceNumbers.begin();

    float v = 0.0f;
    float minMod = std::fmod( lmin, lstep );
    if ( ( minMod < epsilon or ( lstep - minMod ) < epsilon ) && lmin <= 0.0f && lmax >= 0.0f )
      v = 1.0f;

    return 1.0 - float( nrIndex ) / ( niceNumbers.size() - 1.0 ) - float( j ) + v;
  }

  float simplicityMmax( float niceNumber, std::vector<float> const &niceNumbers, int j )
  {
    int nrIndex = std::find( niceNumbers.begin(), niceNumbers.end(), niceNumber ) - niceNumbers.begin();
    return 1.0 - float( nrIndex ) / ( niceNumbers.size() - 1.0 ) - float( j ) + 1.0;
  }

  void extendedWilkinsonAlgorithm( float axisMin, float axisMmax, int nrTicks, bool extend, float &rangeMin, float &rangeMax, float &rangeStep )
  {
    std::vector<float> niceNumbers = {1.0f, 5.0f, 2.0f, 2.5f, 4.0f, 3.0f};
    std::vector<float> weigths = {0.25f, 0.2f, 0.5f, 0.05f};

    float bestScore = -2.0f;
    int j = 1;

    while ( j < std::numeric_limits<int>::max() )
    {
      for ( float niceNumber : niceNumbers )
      {
        float sm = simplicityMmax( niceNumber, niceNumbers, j );
        if ( ( weigths[0]*sm + weigths[1] + weigths[2] + weigths[3] ) < bestScore )
        {
          j = std::numeric_limits<int>::max() - 1;
          break;
        }

        // loop over tick counts
        int k = 2;
        while ( k < std::numeric_limits<int>::max() )
        {
          float dm = densityMax( k, nrTicks );
          if ( ( weigths[0]*sm + weigths[1] + weigths[2]*dm + weigths[3] ) < bestScore )
            break;

          float delta = ( axisMmax - axisMin ) / ( k + 1.0f ) / float( j ) / niceNumber;
          int z = ( int ) std::ceil( std::log10( delta ) );

          while ( z < std::numeric_limits<int>::max() )
          {
            float step = float( j ) * niceNumber * std::pow( 10.0f, z );
            float cm = coverageMax( axisMin, axisMmax, step * ( k - 1.0f ) );
            if ( ( weigths[0]*sm + weigths[1]*cm + weigths[2]*dm + weigths[3] ) < bestScore )
              break;

            int minStart = ( int ) std::floor( axisMmax / ( step ) ) * float( j ) - ( k - 1.0f ) * j;
            int maxStart = ( int ) std::ceil( axisMin / ( step ) ) * float( j );
            if ( minStart > maxStart )
            {
              z++;
              continue;
            }

            for ( int start = minStart; start <= maxStart; start++ )
            {
              float lmin = start * ( step / float( j ) );
              float lmax = lmin + step * ( k - 1 );

              float s = simplicity( niceNumber, niceNumbers, j, lmin, lmax, step );
              float c = coverage( axisMin, axisMmax, lmin, lmax );
              float dens = density( k, nrTicks, axisMin, axisMmax, lmin, lmax );
              float score = weigths[0] * s + weigths[1] * c + weigths[2] * dens + weigths[3];

              if ( score > bestScore && ( extend || ( lmin >= axisMin && lmax <= axisMmax ) ) )
              {
                bestScore = score;
                rangeMin = lmin;
                rangeMax = lmax;
                rangeStep = step;
              }
            }
            z++;
          }
          k++;
        }
      }
      j++;
    }
  }
}
