/***************************************************************************
  qgs3dnavigationwidget.cpp
  --------------------------------------
  Date                 : June 2019
  Copyright            : (C) 2019 by Ismail Sunni
  Email                : imajimatika at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QGridLayout>
#include <QToolButton>
#include <QObject>
#include <QHeaderView>
#include <QCheckBox>
#include "qgis.h"

Q_NOWARN_DEPRECATED_PUSH
#include "qwt_compass.h"
#include "qwt_dial_needle.h"
Q_NOWARN_DEPRECATED_POP

#include "qgsapplication.h"

#include "qgscameracontroller.h"
#include "qgs3dnavigationwidget.h"

#include <Qt3DRender/QCamera>

Qgs3DNavigationWidget::Qgs3DNavigationWidget( Qgs3DMapCanvas *parent ) : QWidget( parent )
{
  setupUi( this );

  mParent3DMapCanvas = parent;
  // Zoom in button
  zoomInButton->setToolTip( tr( "Zoom In" ) );
  zoomInButton->setAutoRepeat( true );
  zoomInButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionZoomIn.svg" ) ) );
  zoomInButton->setAutoRaise( true );

  QObject::connect(
    zoomInButton,
    &QToolButton::clicked,
    parent,
    [ = ]
  {
    parent->cameraController()->zoom( 5 );
  }
  );

  // Zoom out button
  zoomOutButton->setToolTip( tr( "Zoom Out" ) );
  zoomOutButton->setAutoRepeat( true );
  zoomOutButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionZoomOut.svg" ) ) );
  zoomOutButton->setAutoRaise( true );

  QObject::connect(
    zoomOutButton,
    &QToolButton::clicked,
    parent,
    [ = ]
  {
    parent->cameraController()->zoom( -5 );
  }
  );

  // Tilt up button
  tiltUpButton->setToolTip( tr( "Tilt Up" ) );
  tiltUpButton->setAutoRepeat( true );
  tiltUpButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionTiltUp.svg" ) ) );
  tiltUpButton->setAutoRaise( true );

  QObject::connect(
    tiltUpButton,
    &QToolButton::clicked,
    parent,
    [ = ]
  {
    parent->cameraController()->tiltUpAroundViewCenter( 1 );
  }
  );

  // Tilt down button
  tiltDownButton->setToolTip( tr( "Tilt Down" ) );
  tiltDownButton->setAutoRepeat( true );
  tiltDownButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionTiltDown.svg" ) ) );
  tiltDownButton->setAutoRaise( true );

  QObject::connect(
    tiltDownButton,
    &QToolButton::clicked,
    parent,
    [ = ]
  {
    parent->cameraController()->tiltUpAroundViewCenter( -1 );
  }
  );

  // Compas
  QwtCompassMagnetNeedle *compasNeedle = new QwtCompassMagnetNeedle();
  compass->setToolTip( tr( "Rotate view" ) );
  compass->setWrapping( true );
  compass->setNeedle( compasNeedle );

  QObject::connect(
    compass,
    &QwtDial::valueChanged,
    parent,
    [ = ]
  {
    parent->cameraController()->setCameraHeadingAngle( float( compass->value() ) );
  }
  );

  // Move up button
  moveUpButton->setToolTip( tr( "Move up" ) );
  moveUpButton->setAutoRepeat( true );
  moveUpButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionArrowUp.svg" ) ) );
  moveUpButton->setAutoRaise( true );

  QObject::connect(
    moveUpButton,
    &QToolButton::clicked,
    parent,
    [ = ]
  {
    parent->cameraController()->moveView( 0, 1 );
  }
  );

  // Move right button
  moveRightButton->setToolTip( tr( "Move right" ) );
  moveRightButton->setAutoRepeat( true );
  moveRightButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionArrowRight.svg" ) ) );
  moveRightButton->setAutoRaise( true );

  QObject::connect(
    moveRightButton,
    &QToolButton::clicked,
    parent,
    [ = ]
  {
    parent->cameraController()->moveView( 1, 0 );
  }
  );

  // Move down button
  moveDownButton->setToolTip( tr( "Move down" ) );
  moveDownButton->setAutoRepeat( true );
  moveDownButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionArrowDown.svg" ) ) );
  moveDownButton->setAutoRaise( true );

  QObject::connect(
    moveDownButton,
    &QToolButton::clicked,
    parent,
    [ = ]
  {
    parent->cameraController()->moveView( 0, -1 );
  }
  );

  // Move left button
  moveLeftButton->setToolTip( tr( "Move left" ) );
  moveLeftButton->setAutoRepeat( true );
  moveLeftButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionArrowLeft.svg" ) ) );
  moveLeftButton->setAutoRaise( true );

  QObject::connect(
    moveLeftButton,
    &QToolButton::clicked,
    parent,
    [ = ]
  {
    parent->cameraController()->moveView( -1, 0 );
  }
  );

  cameraInfo->setEditTriggers( QAbstractItemView::NoEditTriggers );

  mCameraInfoItemModel = new QStandardItemModel( this );

  mCameraInfoItemModel->appendRow( QList<QStandardItem *> { new QStandardItem( QStringLiteral( "Near plane" ) ), new QStandardItem } );
  mCameraInfoItemModel->appendRow( QList<QStandardItem *> { new QStandardItem( QStringLiteral( "Far plane" ) ), new QStandardItem } );
  mCameraInfoItemModel->appendRow( QList<QStandardItem *> { new QStandardItem( QStringLiteral( "Camera X pos" ) ), new QStandardItem } );
  mCameraInfoItemModel->appendRow( QList<QStandardItem *> { new QStandardItem( QStringLiteral( "Camera Y pos" ) ), new QStandardItem } );
  mCameraInfoItemModel->appendRow( QList<QStandardItem *> { new QStandardItem( QStringLiteral( "Camera Z pos" ) ), new QStandardItem } );
  mCameraInfoItemModel->appendRow( QList<QStandardItem *> { new QStandardItem( QStringLiteral( "Looking at X" ) ), new QStandardItem } );
  mCameraInfoItemModel->appendRow( QList<QStandardItem *> { new QStandardItem( QStringLiteral( "Looking at Y" ) ), new QStandardItem } );
  mCameraInfoItemModel->appendRow( QList<QStandardItem *> { new QStandardItem( QStringLiteral( "Looking at Z" ) ), new QStandardItem } );

  cameraInfo->setModel( mCameraInfoItemModel );
  cameraInfo->verticalHeader()->hide();
  cameraInfo->horizontalHeader()->hide();
  cameraInfo->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Stretch );

  cameraInfo->setVisible( false );
  QObject::connect( cameraInfoCheckBox, &QCheckBox::clicked, parent, [ = ]( bool enabled ) { cameraInfo->setVisible( enabled ); } );

  widgetLayout->setAlignment( Qt::AlignTop );

  QObject::connect( cbo3DAxis, &QComboBox::currentTextChanged, this, &Qgs3DNavigationWidget::updateAxisMode );
  cbo3DAxis->setCurrentIndex( 1 ); // not OFF
}

void Qgs3DNavigationWidget::updateAxisMode( const QString &modeStr )
{
  Qgs3DAxis::Mode m;
  if ( modeStr.toUpper() == "SRS" ) m = Qgs3DAxis::Mode::SRS;
  else if ( modeStr.toUpper().startsWith( "NORTH" ) ) m = Qgs3DAxis::Mode::NEU;
  else m = Qgs3DAxis::Mode::OFF;
  if ( mParent3DMapCanvas->get3DAxis() )
    mParent3DMapCanvas->get3DAxis()->setMode( m );
}

void Qgs3DNavigationWidget::updateFromCamera()
{
  // Make sure the angle is between 0 - 359
  whileBlocking( compass )->setValue( fmod( mParent3DMapCanvas->cameraController()->yaw() + 360, 360 ) );

  mCameraInfoItemModel->setData( mCameraInfoItemModel->index( 0, 1 ), QStringLiteral( "%1" ).arg( mParent3DMapCanvas->cameraController()->camera()->nearPlane() ) );
  mCameraInfoItemModel->setData( mCameraInfoItemModel->index( 1, 1 ), QStringLiteral( "%1" ).arg( mParent3DMapCanvas->cameraController()->camera()->farPlane() ) );
  mCameraInfoItemModel->setData( mCameraInfoItemModel->index( 2, 1 ), QStringLiteral( "%1" ).arg( mParent3DMapCanvas->cameraController()->camera()->position().x() ) );
  mCameraInfoItemModel->setData( mCameraInfoItemModel->index( 3, 1 ), QStringLiteral( "%1" ).arg( mParent3DMapCanvas->cameraController()->camera()->position().y() ) );
  mCameraInfoItemModel->setData( mCameraInfoItemModel->index( 4, 1 ), QStringLiteral( "%1" ).arg( mParent3DMapCanvas->cameraController()->camera()->position().z() ) );
  mCameraInfoItemModel->setData( mCameraInfoItemModel->index( 5, 1 ), QStringLiteral( "%1" ).arg( mParent3DMapCanvas->cameraController()->lookingAtPoint().x() ) );
  mCameraInfoItemModel->setData( mCameraInfoItemModel->index( 6, 1 ), QStringLiteral( "%1" ).arg( mParent3DMapCanvas->cameraController()->lookingAtPoint().y() ) );
  mCameraInfoItemModel->setData( mCameraInfoItemModel->index( 7, 1 ), QStringLiteral( "%1" ).arg( mParent3DMapCanvas->cameraController()->lookingAtPoint().z() ) );
}

void Qgs3DNavigationWidget::resizeEvent( QResizeEvent *ev )
{
  QWidget::resizeEvent( ev );

  QSize size = ev->size();
  emit sizeChanged( size );
}

void Qgs3DNavigationWidget::hideEvent( QHideEvent *ev )
{
  QWidget::hideEvent( ev );
  emit sizeChanged( QSize( 0, 0 ) );
}

void Qgs3DNavigationWidget::showEvent( QShowEvent *ev )
{
  QWidget::showEvent( ev );
  emit sizeChanged( size() );
}
