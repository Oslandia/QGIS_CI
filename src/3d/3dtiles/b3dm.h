/***************************************************************************
  b3dm.h
  --------------------------------------
  Date                 : Mars 2021
  Copyright            : (C) 2021 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
#include <tiny_gltf.h>

#define LOGTHROW(level, ex, msg) { QgsLogger::level(msg); throw new ex(msg.toStdString()); }

class FeatureTable
{
  public:
    uint mBatchLength;
    QVector3D mRtcCenter;

  public:
    FeatureTable();
    FeatureTable( const QJsonObject &obj, const QByteArray &binary );

};

class BatchTable
{
  public:
    BatchTable();
    BatchTable( const QJsonObject &obj, const QByteArray &binary );
};

class B3dm
{
  public:
    struct Header
    {
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

    QByteArray mGltfData;
    tinygltf::Model *mGltf;

  public:
    B3dm();
    virtual ~B3dm();

    void load( QIODevice &dev );
    tinygltf::Model *getGltfModel();

};


#endif // threedtiles_b3dm_hpp_included_
