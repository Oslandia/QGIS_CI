#include <Qt3DCore/QTransform>
#include <Qt3DRender/QCamera>

#include "qgs3dbillboardlabel.h"
#include "qgscameracontroller.h"

Qgs3DBillboardLabel::Qgs3DBillboardLabel( const QgsCameraController *cameraController, Qt3DCore::QNode *parent )
  : Qt3DExtras::QText2DEntity( ),
    mSceneCamera( cameraController->camera() )
{
  this->setParent( parent );
  mTransform = new Qt3DCore::QTransform;
  addComponent( mTransform );

  connect( cameraController, &QgsCameraController::cameraChanged, this, &Qgs3DBillboardLabel::onCameraViewChanged );
}

void Qgs3DBillboardLabel::setTranslation( const QVector3D &position )
{
  mTransform->setTranslation( position );
  // mInitialDistance = mInitialCameraPosition.distanceToPoint( position );
}

void Qgs3DBillboardLabel::onCameraViewChanged()
{
  // QVector3D textPosition =  mTransform->rotation().rotatedVector( mTransform->translation() );
  QVector3D textPosition =  mTransform->translation();
  QVector3D cameraPosition = mSceneCamera->position();

  QVector3D textToCamera = cameraPosition - textPosition;
  textToCamera.normalize();
  QVector3D cameraRight = QVector3D::crossProduct( mSceneCamera->upVector(), textToCamera );
  QVector3D upAux = QVector3D::crossProduct( textToCamera, cameraRight );

  QMatrix4x4 billboardTransform;
  billboardTransform.setColumn( 0, QVector4D( cameraRight, 0.0f ) );
  billboardTransform.setColumn( 1, QVector4D( upAux, 0.0f ) );
  billboardTransform.setColumn( 2, QVector4D( textToCamera, 0.0f ) );
  billboardTransform.setColumn( 3, QVector4D( textPosition, 1.0f ) );

  mTransform->setMatrix( billboardTransform );
}
