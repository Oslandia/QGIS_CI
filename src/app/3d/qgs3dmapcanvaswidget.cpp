/***************************************************************************
  qgs3dmapcanvaswidget.cpp
  --------------------------------------
  Date                 : January 2022
  Copyright            : (C) 2022 by Belgacem Nedjima
  Email                : belgacem dot nedjima at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3dmapcanvaswidget.h"

#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QToolBar>
#include <QUrl>
#include <QAction>

#include "qgisapp.h"
#include "qgs3dboundingboxsettings.h"
#include "qgs3dmapcanvas.h"
#include "qgs3dmapconfigwidget.h"
#include "qgs3dmapscene.h"
#include "qgscameracontroller.h"
#include "qgshelp.h"
#include "qgsmapcanvas.h"
#include "qgsmessagebar.h"
#include "qgsapplication.h"
#include "qgssettings.h"
#include "qgsgui.h"
#include "qgsmapthemecollection.h"

#include "qgs3danimationsettings.h"
#include "qgs3danimationwidget.h"
#include "qgs3dmapsettings.h"
#include "qgs3dmaptoolidentify.h"
#include "qgs3dmaptoolmeasureline.h"
#include "qgs3dutils.h"

#include "qgsmap3dexportwidget.h"
#include "qgs3dmapexportsettings.h"

#include "qgsdockablewidgethelper.h"
#include "qgsrubberband.h"

#include <QWidget>
#include <QActionGroup>

Qgs3DMapCanvasWidget::Qgs3DMapCanvasWidget( const QString &name, bool isDocked )
  : QWidget( nullptr )
  , mCanvasName( name )
  , mViewFrustumHighlight( nullptr )
{
  const QgsSettings setting;

  QToolBar *toolBar = new QToolBar( this );
  toolBar->setIconSize( QgisApp::instance()->iconSize( true ) );

  QAction *actionCameraControl = toolBar->addAction( QIcon( QgsApplication::iconPath( "mActionPan.svg" ) ),
                                 tr( "Camera Control" ), this, &Qgs3DMapCanvasWidget::cameraControl );
  actionCameraControl->setCheckable( true );

  toolBar->addAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionZoomFullExtent.svg" ) ),
                      tr( "Zoom Full" ), this, &Qgs3DMapCanvasWidget::resetView );

  QAction *toggleOnScreenNavigation = toolBar->addAction(
                                        QgsApplication::getThemeIcon( QStringLiteral( "mAction3DNavigation.svg" ) ),
                                        tr( "Toggle On-Screen Navigation" ) );

  toggleOnScreenNavigation->setCheckable( true );
  toggleOnScreenNavigation->setChecked(
    setting.value( QStringLiteral( "/3D/navigationWidget/visibility" ), true, QgsSettings::Gui ).toBool()
  );
  QObject::connect( toggleOnScreenNavigation, &QAction::toggled, this, &Qgs3DMapCanvasWidget::toggleNavigationWidget );

  toolBar->addSeparator();

  QAction *actionIdentify = toolBar->addAction( QIcon( QgsApplication::iconPath( "mActionIdentify.svg" ) ),
                            tr( "Identify" ), this, &Qgs3DMapCanvasWidget::identify );
  actionIdentify->setCheckable( true );

  QAction *actionMeasurementTool = toolBar->addAction( QIcon( QgsApplication::iconPath( "mActionMeasure.svg" ) ),
                                   tr( "Measurement Line" ), this, &Qgs3DMapCanvasWidget::measureLine );
  actionMeasurementTool->setCheckable( true );

  // Create action group to make the action exclusive
  QActionGroup *actionGroup = new QActionGroup( this );
  actionGroup->addAction( actionCameraControl );
  actionGroup->addAction( actionIdentify );
  actionGroup->addAction( actionMeasurementTool );
  actionGroup->setExclusive( true );
  actionCameraControl->setChecked( true );

  QAction *actionAnim = toolBar->addAction( QIcon( QgsApplication::iconPath( "mTaskRunning.svg" ) ),
                        tr( "Animations" ), this, &Qgs3DMapCanvasWidget::toggleAnimations );
  actionAnim->setCheckable( true );

  toolBar->addAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionSaveMapAsImage.svg" ) ),
                      tr( "Save as Image…" ), this, &Qgs3DMapCanvasWidget::saveAsImage );

  toolBar->addAction( QgsApplication::getThemeIcon( QStringLiteral( "3d.svg" ) ),
                      tr( "Export 3D Scene" ), this, &Qgs3DMapCanvasWidget::exportScene );

  toolBar->addSeparator();

  // Map Theme Menu
  mMapThemeMenu = new QMenu( this );
  connect( mMapThemeMenu, &QMenu::aboutToShow, this, &Qgs3DMapCanvasWidget::mapThemeMenuAboutToShow );
  connect( QgsProject::instance()->mapThemeCollection(), &QgsMapThemeCollection::mapThemeRenamed, this, &Qgs3DMapCanvasWidget::currentMapThemeRenamed );

  mBtnMapThemes = new QToolButton();
  mBtnMapThemes->setAutoRaise( true );
  mBtnMapThemes->setToolTip( tr( "Set View Theme" ) );
  mBtnMapThemes->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionShowAllLayers.svg" ) ) );
  mBtnMapThemes->setPopupMode( QToolButton::InstantPopup );
  mBtnMapThemes->setMenu( mMapThemeMenu );

  toolBar->addWidget( mBtnMapThemes );

  toolBar->addSeparator();

  // Options Menu
  mOptionsMenu = new QMenu( this );

  mBtnOptions = new QToolButton();
  mBtnOptions->setAutoRaise( true );
  mBtnOptions->setToolTip( tr( "Options" ) );
  mBtnOptions->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "mActionOptions.svg" ) ) );
  mBtnOptions->setPopupMode( QToolButton::InstantPopup );
  mBtnOptions->setMenu( mOptionsMenu );

  toolBar->addWidget( mBtnOptions );

  mActionEnableShadows = new QAction( tr( "Show Shadows" ), this );
  mActionEnableShadows->setCheckable( true );
  connect( mActionEnableShadows, &QAction::toggled, this, [ = ]( bool enabled )
  {
    QgsShadowSettings settings = mCanvas->map()->shadowSettings();
    settings.setRenderShadows( enabled );
    mCanvas->map()->setShadowSettings( settings );
  } );
  mOptionsMenu->addAction( mActionEnableShadows );

  mActionEnableEyeDome = new QAction( tr( "Show Eye Dome Lighting" ), this );
  mActionEnableEyeDome->setCheckable( true );
  connect( mActionEnableEyeDome, &QAction::triggered, this, [ = ]( bool enabled )
  {
    mCanvas->map()->setEyeDomeLightingEnabled( enabled );
  } );
  mOptionsMenu->addAction( mActionEnableEyeDome );

  mActionEnableAmbientOcclusion = new QAction( tr( "Show Ambient Occlusion" ), this );
  mActionEnableAmbientOcclusion->setCheckable( true );
  connect( mActionEnableAmbientOcclusion, &QAction::triggered, this, [ = ]( bool enabled )
  {
    QgsAmbientOcclusionSettings ambientOcclusionSettings = mCanvas->map()->ambientOcclusionSettings();
    ambientOcclusionSettings.setEnabled( enabled );
    mCanvas->map()->setAmbientOcclusionSettings( ambientOcclusionSettings );
  } );
  mOptionsMenu->addAction( mActionEnableAmbientOcclusion );

  mOptionsMenu->addSeparator();

  mActionSync2DNavTo3D = new QAction( tr( "2D Map View Follows 3D Camera" ), this );
  mActionSync2DNavTo3D->setCheckable( true );
  connect( mActionSync2DNavTo3D, &QAction::triggered, this, [ = ]( bool enabled )
  {
    Qgis::ViewSyncModeFlags syncMode = mCanvas->map()->viewSyncMode();
    syncMode.setFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D, enabled );
    mCanvas->map()->setViewSyncMode( syncMode );
  } );
  mOptionsMenu->addAction( mActionSync2DNavTo3D );

  mActionSync3DNavTo2D = new QAction( tr( "3D Camera Follows 2D Map View" ), this );
  mActionSync3DNavTo2D->setCheckable( true );
  connect( mActionSync3DNavTo2D, &QAction::triggered, this, [ = ]( bool enabled )
  {
    Qgis::ViewSyncModeFlags syncMode = mCanvas->map()->viewSyncMode();
    syncMode.setFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D, enabled );
    mCanvas->map()->setViewSyncMode( syncMode );
  } );
  mOptionsMenu->addAction( mActionSync3DNavTo2D );

  mShowFrustumPolyogon = new QAction( tr( "Show Visible Camera Area in 2D Map View" ), this );
  mShowFrustumPolyogon->setCheckable( true );
  connect( mShowFrustumPolyogon, &QAction::triggered, this, [ = ]( bool enabled )
  {
    mCanvas->map()->setViewFrustumVisualizationEnabled( enabled );
  } );
  mOptionsMenu->addAction( mShowFrustumPolyogon );

  mOptionsMenu->addSeparator();

  QAction *configureAction = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionOptions.svg" ) ),
                                          tr( "Configure…" ), this );
  connect( configureAction, &QAction::triggered, this, &Qgs3DMapCanvasWidget::configure );
  mOptionsMenu->addAction( configureAction );

  mCanvas = new Qgs3DMapCanvas( this );
  mCanvas->setMinimumSize( QSize( 200, 200 ) );
  mCanvas->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

  connect( mCanvas, &Qgs3DMapCanvas::savedAsImage, this, [ = ]( const QString & fileName )
  {
    QgisApp::instance()->messageBar()->pushSuccess( tr( "Save as Image" ), tr( "Successfully saved the 3D map to <a href=\"%1\">%2</a>" ).arg( QUrl::fromLocalFile( fileName ).toString(), QDir::toNativeSeparators( fileName ) ) );
  } );

  connect( mCanvas, &Qgs3DMapCanvas::fpsCountChanged, this, &Qgs3DMapCanvasWidget::updateFpsCount );
  connect( mCanvas, &Qgs3DMapCanvas::fpsCounterEnabledChanged, this, &Qgs3DMapCanvasWidget::toggleFpsCounter );
  connect( mCanvas, &Qgs3DMapCanvas::cameraNavigationSpeedChanged, this, &Qgs3DMapCanvasWidget::cameraNavigationSpeedChanged );
  connect( mCanvas, &Qgs3DMapCanvas::viewed2DExtentFrom3DChanged, this, &Qgs3DMapCanvasWidget::onViewed2DExtentFrom3DChanged );

  mMapToolIdentify = new Qgs3DMapToolIdentify( mCanvas );

  mMapToolMeasureLine = new Qgs3DMapToolMeasureLine( mCanvas );

  mLabelPendingJobs = new QLabel( this );
  mProgressPendingJobs = new QProgressBar( this );
  mProgressPendingJobs->setRange( 0, 0 );
  mLabelFpsCounter = new QLabel( this );
  mLabelNavigationSpeed = new QLabel( this );

  mAnimationWidget = new Qgs3DAnimationWidget( this );
  mAnimationWidget->setVisible( false );

  QHBoxLayout *topLayout = new QHBoxLayout;
  topLayout->setContentsMargins( 0, 0, 0, 0 );
  topLayout->setSpacing( style()->pixelMetric( QStyle::PM_LayoutHorizontalSpacing ) );
  topLayout->addWidget( toolBar );
  topLayout->addStretch( 1 );
  topLayout->addWidget( mLabelPendingJobs );
  topLayout->addWidget( mProgressPendingJobs );
  topLayout->addWidget( mLabelNavigationSpeed );
  mLabelNavigationSpeed->hide();
  topLayout->addWidget( mLabelFpsCounter );

  mLabelNavSpeedHideTimeout = new QTimer( this );
  mLabelNavSpeedHideTimeout->setInterval( 1000 );
  connect( mLabelNavSpeedHideTimeout, &QTimer::timeout, this, [ = ]
  {
    mLabelNavigationSpeed->hide();
    mLabelNavSpeedHideTimeout->stop();
  } );

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->setSpacing( 0 );
  layout->addLayout( topLayout );
  layout->addWidget( mCanvas );
  layout->addWidget( mAnimationWidget );

  setLayout( layout );

  onTotalPendingJobsCountChanged();

  mDockableWidgetHelper = new QgsDockableWidgetHelper( isDocked, mCanvasName, this, QgisApp::instance() );
  QToolButton *toggleButton = mDockableWidgetHelper->createDockUndockToolButton();
  toggleButton->setToolTip( tr( "Dock 3D Map View" ) );
  toolBar->addWidget( toggleButton );
  connect( mDockableWidgetHelper, &QgsDockableWidgetHelper::closed, this, [ = ]()
  {
    QgisApp::instance()->close3DMapView( canvasName() );
  } );
}

Qgs3DMapCanvasWidget::~Qgs3DMapCanvasWidget()
{
  delete mDockableWidgetHelper;
  if ( mViewFrustumHighlight )
    delete mViewFrustumHighlight;

  if ( mViewBoundingBoxHighlight )
    delete mViewBoundingBoxHighlight;
}

void Qgs3DMapCanvasWidget::resizeEvent( QResizeEvent *event )
{
  QWidget::resizeEvent( event );
  mCanvas->resize( event->size() );
}

void Qgs3DMapCanvasWidget::saveAsImage()
{
  const QPair< QString, QString> fileNameAndFilter = QgsGuiUtils::getSaveAsImageName( this, tr( "Choose a file name to save the 3D map canvas to an image" ) );
  if ( !fileNameAndFilter.first.isEmpty() )
  {
    mCanvas->saveAsImage( fileNameAndFilter.first, fileNameAndFilter.second );
  }
}

void Qgs3DMapCanvasWidget::toggleAnimations()
{
  if ( mAnimationWidget->isVisible() )
  {
    mAnimationWidget->setVisible( false );
    return;
  }

  mAnimationWidget->setVisible( true );

  // create a dummy animation when first started - better to have something than nothing...
  if ( mAnimationWidget->animation().duration() == 0 )
  {
    mAnimationWidget->setDefaultAnimation();
  }
}

void Qgs3DMapCanvasWidget::cameraControl()
{
  QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mCanvas->setMapTool( nullptr );
}

void Qgs3DMapCanvasWidget::identify()
{
  QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mCanvas->setMapTool( action->isChecked() ? mMapToolIdentify : nullptr );
}

void Qgs3DMapCanvasWidget::measureLine()
{
  QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mCanvas->setMapTool( action->isChecked() ? mMapToolMeasureLine : nullptr );
}

void Qgs3DMapCanvasWidget::setCanvasName( const QString &name )
{
  mCanvasName = name;
  mDockableWidgetHelper->setWindowTitle( name );
}

void Qgs3DMapCanvasWidget::toggleNavigationWidget( bool visibility )
{
  mCanvas->setOnScreenNavigationVisibility( visibility );
}

void Qgs3DMapCanvasWidget::toggleFpsCounter( bool visibility )
{
  mLabelFpsCounter->setVisible( visibility );
}

void Qgs3DMapCanvasWidget::setMapSettings( Qgs3DMapSettings *map )
{
  whileBlocking( mActionEnableShadows )->setChecked( map->shadowSettings().renderShadows() );
  whileBlocking( mActionEnableEyeDome )->setChecked( map->eyeDomeLightingEnabled() );
  whileBlocking( mActionEnableAmbientOcclusion )->setChecked( map->ambientOcclusionSettings().isEnabled() );
  whileBlocking( mActionSync2DNavTo3D )->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D ) );
  whileBlocking( mActionSync3DNavTo2D )->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) );
  whileBlocking( mShowFrustumPolyogon )->setChecked( map->viewFrustumVisualizationEnabled() );

  mCanvas->setMap( map );

  connect( mCanvas->scene(), &Qgs3DMapScene::totalPendingJobsCountChanged, this, &Qgs3DMapCanvasWidget::onTotalPendingJobsCountChanged );

  mAnimationWidget->setCameraController( mCanvas->scene()->cameraController() );
  mAnimationWidget->setMap( map );

  // Disable button for switching the map theme if the terrain generator is a mesh, or if there is no terrain
  mBtnMapThemes->setDisabled( !mCanvas->map()->terrainRenderingEnabled()
                              || !mCanvas->map()->terrainGenerator()
                              || mCanvas->map()->terrainGenerator()->type() == QgsTerrainGenerator::Mesh );
  mLabelFpsCounter->setVisible( map->isFpsCounterEnabled() );

  connect( map, &Qgs3DMapSettings::viewFrustumVisualizationEnabledChanged, this, &Qgs3DMapCanvasWidget::onViewFrustumVisualizationEnabledChanged );
  connect( map, &Qgs3DMapSettings::boundingBoxSettingsChanged, this, &Qgs3DMapCanvasWidget::onViewBoundingBoxChanged );
  onViewBoundingBoxChanged();
}

void Qgs3DMapCanvasWidget::setMainCanvas( QgsMapCanvas *canvas )
{
  mMainCanvas = canvas;

  connect( mMainCanvas, &QgsMapCanvas::layersChanged, this, &Qgs3DMapCanvasWidget::onMainCanvasLayersChanged );
  connect( mMainCanvas, &QgsMapCanvas::canvasColorChanged, this, &Qgs3DMapCanvasWidget::onMainCanvasColorChanged );
  connect( mMainCanvas, &QgsMapCanvas::extentsChanged, this, &Qgs3DMapCanvasWidget::onMainMapCanvasExtentChanged );

  if ( mViewFrustumHighlight )
    delete mViewFrustumHighlight;
  mViewFrustumHighlight = new QgsRubberBand( canvas, QgsWkbTypes::PolygonGeometry );
  mViewFrustumHighlight->setColor( QColor::fromRgba( qRgba( 0, 0, 255, 50 ) ) );

  if ( mViewBoundingBoxHighlight )
    delete mViewBoundingBoxHighlight;

  mViewBoundingBoxHighlight = new QgsRubberBand( canvas, QgsWkbTypes::PolygonGeometry );
}

void Qgs3DMapCanvasWidget::resetView()
{
  mCanvas->resetView( true );
}

void Qgs3DMapCanvasWidget::configure()
{
  if ( mConfigureDialog )
  {
    mConfigureDialog->raise();
    return;
  }

  mConfigureDialog = new QDialog( this );
  mConfigureDialog->setAttribute( Qt::WA_DeleteOnClose );
  mConfigureDialog->setWindowTitle( tr( "3D Configuration" ) );
  mConfigureDialog->setObjectName( QStringLiteral( "3DConfigurationDialog" ) );
  mConfigureDialog->setMinimumSize( 600, 460 );
  QgsGui::enableAutoGeometryRestore( mConfigureDialog );

  Qgs3DMapSettings *map = mCanvas->map();
  Qgs3DMapConfigWidget *w = new Qgs3DMapConfigWidget( map, mMainCanvas, mCanvas, mConfigureDialog );
  QDialogButtonBox *buttons = new QDialogButtonBox( QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, mConfigureDialog );

  auto applyConfig = [ = ]()
  {
    const QgsVector3D oldOrigin = map->origin();
    const QgsCoordinateReferenceSystem oldCrs = map->crs();
    const QgsCameraPose oldCameraPose = mCanvas->cameraController()->cameraPose();
    const QgsVector3D oldLookingAt = oldCameraPose.centerPoint();

    // update map
    w->apply();

    const QgsVector3D p = Qgs3DUtils::transformWorldCoordinates(
                            oldLookingAt,
                            oldOrigin, oldCrs,
                            map->origin(), map->crs(), QgsProject::instance()->transformContext() );

    if ( p != oldLookingAt )
    {
      // apply() call has moved origin of the world so let's move camera so we look still at the same place
      QgsCameraPose newCameraPose = oldCameraPose;
      newCameraPose.setCenterPoint( p );
      mCanvas->cameraController()->setCameraPose( newCameraPose );
    }

    // Disable map theme button if the terrain generator is a mesh, or if there is no terrain
    mBtnMapThemes->setDisabled( !mCanvas->map()->terrainRenderingEnabled()
                                || !mCanvas->map()->terrainGenerator()
                                || map->terrainGenerator()->type() == QgsTerrainGenerator::Mesh );
  };

  connect( buttons, &QDialogButtonBox::rejected, mConfigureDialog, &QDialog::reject );
  connect( buttons, &QDialogButtonBox::clicked, mConfigureDialog, [ = ]( QAbstractButton * button )
  {
    if ( button == buttons->button( QDialogButtonBox::Apply ) || button == buttons->button( QDialogButtonBox::Ok ) )
      applyConfig();
    if ( button == buttons->button( QDialogButtonBox::Ok ) )
      mConfigureDialog->accept();
  } );
  connect( buttons, &QDialogButtonBox::helpRequested, w, []() { QgsHelp::openHelp( QStringLiteral( "introduction/qgis_gui.html#scene-configuration" ) ); } );

  connect( w, &Qgs3DMapConfigWidget::isValidChanged, this, [ = ]( bool valid )
  {
    buttons->button( QDialogButtonBox::Apply )->setEnabled( valid );
    buttons->button( QDialogButtonBox::Ok )->setEnabled( valid );
  } );

  QVBoxLayout *layout = new QVBoxLayout( mConfigureDialog );
  layout->addWidget( w, 1 );
  layout->addWidget( buttons );

  mConfigureDialog->show();

  whileBlocking( mActionEnableShadows )->setChecked( map->shadowSettings().renderShadows() );
  whileBlocking( mActionEnableEyeDome )->setChecked( map->eyeDomeLightingEnabled() );
  whileBlocking( mActionEnableAmbientOcclusion )->setChecked( map->ambientOcclusionSettings().isEnabled() );
  whileBlocking( mActionSync2DNavTo3D )->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D ) );
  whileBlocking( mActionSync3DNavTo2D )->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) );
  whileBlocking( mShowFrustumPolyogon )->setChecked( map->viewFrustumVisualizationEnabled() );
}

void Qgs3DMapCanvasWidget::exportScene()
{

  QDialog dlg;
  dlg.setWindowTitle( tr( "Export 3D Scene" ) );
  dlg.setObjectName( QStringLiteral( "3DSceneExportDialog" ) );
  QgsGui::enableAutoGeometryRestore( &dlg );

  Qgs3DMapExportSettings exportSettings;
  QgsMap3DExportWidget w( mCanvas->scene(), &exportSettings );

  QDialogButtonBox *buttons = new QDialogButtonBox( QDialogButtonBox::Cancel | QDialogButtonBox::Help | QDialogButtonBox::Ok, &dlg );

  connect( buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept );
  connect( buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject );
  connect( buttons, &QDialogButtonBox::helpRequested, &dlg, [ = ] { QgsHelp::openHelp( QStringLiteral( "introduction/qgis_gui.html#d-map-view" ) ); } );

  QVBoxLayout *layout = new QVBoxLayout( &dlg );
  layout->addWidget( &w, 1 );
  layout->addWidget( buttons );
  if ( dlg.exec() )
    w.exportScene();
}

void Qgs3DMapCanvasWidget::onMainCanvasLayersChanged()
{
  mCanvas->map()->setLayers( mMainCanvas->layers( true ) );
}

void Qgs3DMapCanvasWidget::onMainCanvasColorChanged()
{
  mCanvas->map()->setBackgroundColor( mMainCanvas->canvasColor() );
}

void Qgs3DMapCanvasWidget::onTotalPendingJobsCountChanged()
{
  const int count = mCanvas->scene() ? mCanvas->scene()->totalPendingJobsCount() : 0;
  mProgressPendingJobs->setVisible( count );
  mLabelPendingJobs->setVisible( count );
  if ( count )
    mLabelPendingJobs->setText( tr( "Loading %n tile(s)", nullptr, count ) );
}

void Qgs3DMapCanvasWidget::updateFpsCount( float fpsCount )
{
  mLabelFpsCounter->setText( QStringLiteral( "%1 fps" ).arg( fpsCount, 10, 'f', 2, QLatin1Char( ' ' ) ) );
}

void Qgs3DMapCanvasWidget::cameraNavigationSpeedChanged( double speed )
{
  mLabelNavigationSpeed->setText( QStringLiteral( "Speed: %1 ×" ).arg( QString::number( speed, 'f', 2 ) ) );
  mLabelNavigationSpeed->show();
  mLabelNavSpeedHideTimeout->start();
}

void Qgs3DMapCanvasWidget::mapThemeMenuAboutToShow()
{
  qDeleteAll( mMapThemeMenuPresetActions );
  mMapThemeMenuPresetActions.clear();

  const QString currentTheme = mCanvas->map()->terrainMapTheme();

  QAction *actionFollowMain = new QAction( tr( "(none)" ), mMapThemeMenu );
  actionFollowMain->setCheckable( true );
  if ( currentTheme.isEmpty() || !QgsProject::instance()->mapThemeCollection()->hasMapTheme( currentTheme ) )
  {
    actionFollowMain->setChecked( true );
  }
  connect( actionFollowMain, &QAction::triggered, this, [ = ]
  {
    mCanvas->map()->setTerrainMapTheme( QString() );
  } );
  mMapThemeMenuPresetActions.append( actionFollowMain );

  const auto constMapThemes = QgsProject::instance()->mapThemeCollection()->mapThemes();
  for ( const QString &grpName : constMapThemes )
  {
    QAction *a = new QAction( grpName, mMapThemeMenu );
    a->setCheckable( true );
    if ( grpName == currentTheme )
    {
      a->setChecked( true );
    }
    connect( a, &QAction::triggered, this, [a, this]
    {
      mCanvas->map()->setTerrainMapTheme( a->text() );
    } );
    mMapThemeMenuPresetActions.append( a );
  }
  mMapThemeMenu->addActions( mMapThemeMenuPresetActions );
}

void Qgs3DMapCanvasWidget::currentMapThemeRenamed( const QString &theme, const QString &newTheme )
{
  if ( theme == mCanvas->map()->terrainMapTheme() )
  {
    mCanvas->map()->setTerrainMapTheme( newTheme );
  }
}

void Qgs3DMapCanvasWidget::onMainMapCanvasExtentChanged()
{
  if ( mCanvas->map()->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) )
  {
    mCanvas->setViewFrom2DExtent( mMainCanvas->extent() );
  }
}

void Qgs3DMapCanvasWidget::onViewed2DExtentFrom3DChanged( QVector<QgsPointXY> extent )
{
  if ( mCanvas->map()->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D ) )
  {
    QgsRectangle extentRect;
    extentRect.setMinimal();
    for ( QgsPointXY &pt : extent )
    {
      extentRect.include( pt );
    }
    if ( !extentRect.isEmpty() && extentRect.isFinite() && !extentRect.isNull() )
    {
      if ( mCanvas->map()->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) )
      {
        whileBlocking( mMainCanvas )->setExtent( extentRect );
      }
      else
      {
        mMainCanvas->setExtent( extentRect );
      }
      mMainCanvas->refresh();
    }
  }

  onViewFrustumVisualizationEnabledChanged();
}

void Qgs3DMapCanvasWidget::onViewFrustumVisualizationEnabledChanged()
{
  mViewFrustumHighlight->reset( QgsWkbTypes::PolygonGeometry );
  if ( mCanvas->map()->viewFrustumVisualizationEnabled() )
  {
    for ( QgsPointXY &pt : mCanvas->viewFrustum2DExtent() )
    {
      mViewFrustumHighlight->addPoint( pt, false );
    }
    mViewFrustumHighlight->closePoints();
  }
}

void Qgs3DMapCanvasWidget::onViewBoundingBoxChanged()
{
  Qgs3DMapSettings *mapSettings = mCanvas->map();
  mViewBoundingBoxHighlight->reset( QgsWkbTypes::PolygonGeometry );
  Qgs3DBoundingBoxSettings bbSettings = mapSettings->getBoundingBoxSettings();
  if ( bbSettings.isEnabled() && bbSettings.showIn2DView() )
  {
    QColor bbColor = bbSettings.color();
    mViewBoundingBoxHighlight->setColor( QColor( bbColor.red(), bbColor.green(), bbColor.blue(), 127 ) );
    QgsAABB bbox = bbSettings.coords();
    QgsVector3D bboxWorldMin( bbox.xMin, bbox.yMin, bbox.zMin );
    QgsVector3D bboxWorldMax( bbox.xMax, bbox.yMax, bbox.zMax );
    QgsVector3D bboxMapMin = mapSettings->worldToMapCoordinates( bboxWorldMin );
    QgsVector3D bboxMapMax = mapSettings->worldToMapCoordinates( bboxWorldMax );

    mViewBoundingBoxHighlight->addPoint( QgsPointXY( bboxMapMin.x(), bboxMapMin.y() ), false );
    mViewBoundingBoxHighlight->addPoint( QgsPointXY( bboxMapMin.x(), bboxMapMax.y() ), false );
    mViewBoundingBoxHighlight->addPoint( QgsPointXY( bboxMapMax.x(), bboxMapMax.y() ), false );
    mViewBoundingBoxHighlight->addPoint( QgsPointXY( bboxMapMax.x(), bboxMapMin.y() ), false );
    mViewBoundingBoxHighlight->closePoints();

    mViewFrustumHighlight->closePoints();
  }
}
