/***************************************************************************
  qgs3daxis.h
  --------------------------------------
  Date                 : March 2022
  Copyright            : (C) 2022 by Jean Felder
  Email                : jean dot felder at oslandia dot com
  ***************************************************************************
  *                                                                         *
  *   This program is free software; you can redistribute it and/or modify  *
  *   it under the terms of the GNU General Public License as published by  *
  *   the Free Software Foundation; either version 2 of the License, or     *
  *   (at your option) any later version.                                   *
  *                                                                         *
  ***************************************************************************/

#ifndef QGS3DAXIS_H
#define QGS3DAXIS_H

#include "qgis_3d.h"

#include <QVector3D>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DRender/QViewport>
#include <Qt3DExtras/QText2DEntity>
#include "qgs3dmapsettings.h"

#define SIP_NO_FILE

class Qgs3DAxis : public QObject
{
    Q_OBJECT
  public:
    Qgs3DAxis( Qt3DExtras::Qt3DWindow *mainWindow,  Qt3DCore::QEntity *main3DScene, QgsCameraController *camera, const Qgs3DMapSettings *map );

    enum Axis
    {
      X = 1,
      Y = 2,
      Z = 3
    };

    enum Mode
    {
      OFF = 1,
      SRS = 2,
      NEU = 3 // North-East-Up
    };

    void updateMode( Qgs3DAxis::Mode axisMode );

  private:
    void createScene();
    void createAxis( const Axis &axis );
    void updateCamera( /*const QVector3D &viewVector*/ );
    void updateViewportSize( int val );

    Qt3DExtras::Qt3DWindow *mMainWindow = nullptr;
    Qt3DRender::QCamera *mMainCamera = nullptr;
    Qt3DCore::QEntity *mSceneEntity = nullptr;
    Qt3DRender::QCamera *mCamera = nullptr;
    Qt3DRender::QViewport *mViewport = nullptr;
    float mCylinderLength = 4.0f;
    Qgs3DAxis::Mode mMode = Qgs3DAxis::Mode::SRS;
    Qt3DCore::QEntity *mAxisRoot = nullptr;
    Qt3DExtras::QText2DEntity *mTextWhite_X, *mTextWhite_Y, *mTextWhite_Z;

    Qt3DCore::QTransform *mTextTransform_X, *mTextTransform_Y, *mTextTransform_Z;
    QgsCoordinateReferenceSystem mCrs;
    QVector3D mPreviousVector;

};

#endif // QGS3DAXIS_H
