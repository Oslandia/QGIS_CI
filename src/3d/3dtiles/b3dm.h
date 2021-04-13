/**
 * Copyright (c) 2019 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef threedtiles_b3dm_hpp_included_
#define threedtiles_b3dm_hpp_included_

#include <QVector3D>
#include <QUrl>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QIODevice>
#include <qgslogger.h>
//#include <tiny_gltf.h>

#define LOGTHROW(level, ex, msg) { QgsLogger::level(msg); throw new ex(msg.toStdString()); }

class FeatureTable {
public:
    uint mBatchLength;
    QVector3D mRtcCenter;

public:
  FeatureTable ();
  FeatureTable (const QJsonObject & obj, const QByteArray &binary);

};

class BatchTable {
public:
  BatchTable ();
  BatchTable (const QJsonObject & obj, const QByteArray &binary);
};

class B3dm {
public:
  struct Header {
    char magic[4];
    unsigned int version;
    unsigned int byteLength;
    unsigned int featureTableJsonByteLength;
    unsigned int featureTableBinaryByteLength;
    unsigned int batchTableJsonByteLength;
    unsigned int batchTableBinaryByteLength;
  };
  QUrl mUrl;
  Header mHeader;
  FeatureTable mFeatureTable;
  BatchTable mBatchTable;

  //tinygltf::Model mGltf;

public:
  B3dm ();

  void load (QIODevice & dev);

};


#endif // threedtiles_b3dm_hpp_included_
