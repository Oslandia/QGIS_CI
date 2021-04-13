#include "b3dm.h"

B3dm::B3dm (): mUrl(), mHeader(), mFeatureTable(), mBatchTable() {
}

void B3dm::load (QIODevice & dev) {
  dev.read((char*)&(mHeader), sizeof (mHeader));
  if (QByteArray(mHeader.magic, 4).compare("b3dm") != 0) {
    LOGTHROW (critical, std::runtime_error, QString("Not a b3dm (" + QString(mHeader.magic) + ") file " + mUrl.toString()));
  }

  if (mHeader.featureTableJsonByteLength > 0) {
    QByteArray data = dev.read(mHeader.featureTableJsonByteLength);
    if (data.size() != (int)mHeader.featureTableJsonByteLength) {
      LOGTHROW (critical, std::runtime_error, QString("binary badly read for featureTableJsonByteLength: read: %d / expected: %d").arg(data.size(), mHeader.featureTableJsonByteLength));
    }
    QJsonDocument doc(QJsonDocument::fromJson(data));

    //QgsLogger::debug (QString ("feature json: ") + doc.toJson());
    QByteArray bin = dev.read(mHeader.featureTableBinaryByteLength);
    if (bin.size() != (int)mHeader.featureTableBinaryByteLength) {
      LOGTHROW (critical, std::runtime_error, QString("binary badly read for featureTableBinaryByteLength: read: %d / expected: %d").arg(bin.size(), mHeader.featureTableBinaryByteLength));
    }

    mFeatureTable = FeatureTable(doc.object(), bin);
  }

  if (mHeader.batchTableJsonByteLength > 0 && mFeatureTable.mBatchLength > 0) {
    QByteArray data = dev.read(mHeader.batchTableJsonByteLength);
    if (data.size() != (int)mHeader.batchTableJsonByteLength) {
      LOGTHROW (critical, std::runtime_error, QString("binary badly read for batchTableJsonByteLength: read: %d / expected: %d").arg(data.size(), mHeader.batchTableJsonByteLength));    }
    QJsonDocument doc(QJsonDocument::fromJson(data));

    //QgsLogger::debug (QString ("batch json: ") + doc.toJson());
    QByteArray bin = dev.read(mHeader.batchTableBinaryByteLength);
    if (bin.size() != (int)mHeader.batchTableBinaryByteLength) {
      LOGTHROW (critical, std::runtime_error, QString("binary badly read for batchTableBinaryByteLength: read: %d / expected: %d").arg(bin.size(), mHeader.batchTableBinaryByteLength));
    }

    mBatchTable = BatchTable(doc.object(), bin);
  }

  //tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;
  QByteArray gltfData = dev.read(mHeader.byteLength // all
                                 - sizeof(Header) // head size
                                 - mHeader.batchTableJsonByteLength - mHeader.featureTableBinaryByteLength
                                 - mHeader.featureTableBinaryByteLength - mHeader.featureTableJsonByteLength);
  /*
  bool res = loader.LoadBinaryFromMemory(&mGltf, &err, &warn, (const unsigned char*) gltfData.data(), gltfData.size(), mUrl.path().toStdString());*/

  if (!warn.empty()) {
    QgsLogger::warning(warn.c_str());
  }

  if (!err.empty()) {
    QgsLogger::critical(err.c_str());
  }

/*  if (!res) {
    QgsLogger::warning(QStringLiteral("Failed to load glTF: %1").arg(mUrl.toString()));
  }*/
  /*else {
    QgsLogger::debug(QStringLiteral("Loaded glTF: %1").arg(url.toString()));
  }*/

  dev.close();
}

BatchTable::BatchTable () {
  // TODO
}

BatchTable::BatchTable (const QJsonObject &, const QByteArray &)  {
  // TODO
}

FeatureTable::FeatureTable () : mBatchLength(0), mRtcCenter() {
}

FeatureTable::FeatureTable (const QJsonObject & obj, const QByteArray &binary) : mBatchLength(0), mRtcCenter() {
  if (obj.find("RTC_CENTER") != obj.end()) {
    if (obj["RTC_CENTER"].isArray()) {
      QJsonArray a = obj["RTC_CENTER"].toArray();
      mRtcCenter = QVector3D (a.takeAt(0).toDouble(), a.takeAt(1).toDouble(), a.takeAt(2).toDouble());

    } else if (obj["RTC_CENTER"].isObject()) {
      if (binary.size() == 0) {
        LOGTHROW (critical, std::runtime_error, QString("binary size is 0 but RTC_CENTER is referencing it!"));
      }
      QJsonObject a = obj["RTC_CENTER"].toObject();
      uint offset = a["byteOffset"].toInt();
      mRtcCenter = QVector3D (*(float*)(binary.data()+offset),
                                   *(float*)(binary.data()+offset+sizeof (float)),
                                   *(float*)(binary.data()+offset+2*sizeof (float)));

    }

  }
  if (obj.find("BATCH_LENGTH") != obj.end()) {
    if (obj["BATCH_LENGTH"].isObject()) {
      if (binary.size() == 0) {
        LOGTHROW (critical, std::runtime_error, QString("binary size is 0 but BATCH_LENGTH is referencing it!"));
      }
      QJsonObject a = obj["BATCH_LENGTH"].toObject();
      uint offset = a["byteOffset"].toInt();
      mBatchLength = *(uint*)(binary.data()+offset);

    } else {
      mBatchLength = obj["BATCH_LENGTH"].toInt();
    }
  }
}
