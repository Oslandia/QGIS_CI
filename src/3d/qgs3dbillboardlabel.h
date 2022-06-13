#ifndef QGS3DBILLBOARDLABEL_H
#define QGS3DBILLBOARDLABEL_H

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#include <Qt3DCore/QComponent>
#endif

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

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    template<class T>
    QVector<T *> componentsOfType() const
    {
      QVector<T *> matchComponents;
      const QVector<Qt3DCore::QComponent *> comps = this->components();
      for ( Qt3DCore::QComponent *component : comps )
      {
        T *typedComponent = qobject_cast<T *>( component );
        if ( typedComponent != nullptr )
          matchComponents.append( typedComponent );
      }
      return matchComponents;
    }
#endif

  private slots:
    void onCameraViewChanged();

  private:
    Qt3DRender::QCamera *mSceneCamera = nullptr;
    Qt3DCore::QTransform *mTransform = nullptr;
    QVector3D mInitialCameraPosition;
};

#endif // QGS3DBILLBOARDLABEL_H
