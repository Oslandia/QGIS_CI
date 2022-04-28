#ifndef QGS3DBILLBOARDLABEL_H
#define QGS3DBILLBOARDLABEL_H

#include <Qt3DExtras/QText2DEntity>
#include <QVector3D>

namespace Qt3DCore
{
  class QNode;
  class QTransform;
}

namespace Qt3DRender
{
  class QCamera;
}

class QgsCameraController;

class Qgs3DBillboardLabel: public Qt3DExtras::QText2DEntity
{
    Q_OBJECT

  public:
    Qgs3DBillboardLabel( const QgsCameraController *cameraController, Qt3DCore::QNode *parent = nullptr );
    void setTranslation( const QVector3D &position );

  private slots:
    void onCameraViewChanged();

  private:
    Qt3DRender::QCamera *mSceneCamera = nullptr;
    Qt3DCore::QTransform *mTransform = nullptr;
    QVector3D mInitialCameraPosition;
};

#endif // QGS3DBILLBOARDLABEL_H
