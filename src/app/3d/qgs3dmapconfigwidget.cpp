/***************************************************************************
  qgs3dmapconfigwidget.cpp
  --------------------------------------
  Date                 : July 2017
  Copyright            : (C) 2017 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3dmapconfigwidget.h"

#include "qgs3dmapsettings.h"
#include "qgsdemterraingenerator.h"
#include "qgsflatterraingenerator.h"
#include "qgsonlineterraingenerator.h"
#include "qgsmeshterraingenerator.h"
#include "qgs3dutils.h"

#include "qgsguiutils.h"
#include "qgsmapcanvas.h"
#include "qgsmapthemecollection.h"
#include "qgsrasterlayer.h"
#include "qgsmeshlayer.h"
#include "qgsproject.h"
#include "qgsprojectviewsettings.h"
#include "qgsmesh3dsymbolwidget.h"
#include "qgssettings.h"
#include "qgsskyboxrenderingsettingswidget.h"
#include "qgsshadowrenderingsettingswidget.h"
#include "qgs3dmapcanvas.h"
#include "qgs3dmapscene.h"

Qgs3DMapConfigWidget::Qgs3DMapConfigWidget( Qgs3DMapSettings *mapSettings, QgsMapCanvas *mainCanvas, Qgs3DMapCanvas *mapCanvas3D, QWidget *parent )
  : QWidget( parent )
  , mMapSettings( mapSettings )
  , mMainCanvas( mainCanvas )
  , m3DMapCanvas( mapCanvas3D )
{
  setupUi( this );

  Q_ASSERT( mapSettings );
  Q_ASSERT( mainCanvas );

  const QgsSettings settings;

  const int iconSize = QgsGuiUtils::scaleIconSize( 20 );
  m3DOptionsListWidget->setIconSize( QSize( iconSize, iconSize ) ) ;

  mCameraNavigationModeCombo->addItem( tr( "Terrain Based" ), QgsCameraController::TerrainBasedNavigation );
  mCameraNavigationModeCombo->addItem( tr( "Walk Mode (First Person)" ), QgsCameraController::WalkNavigation );

  // get rid of annoying outer focus rect on Mac
  m3DOptionsListWidget->setAttribute( Qt::WA_MacShowFocusRect, false );
  m3DOptionsListWidget->setCurrentRow( settings.value( QStringLiteral( "Windows/3DMapConfig/Tab" ), 0 ).toInt() );
  connect( m3DOptionsListWidget, &QListWidget::currentRowChanged, this, [ = ]( int index ) { m3DOptionsStackedWidget->setCurrentIndex( index ); } );
  m3DOptionsStackedWidget->setCurrentIndex( m3DOptionsListWidget->currentRow() );

  if ( !settings.contains( QStringLiteral( "Windows/3DMapConfig/OptionsSplitState" ) ) )
  {
    // set left list widget width on initial showing
    QList<int> splitsizes;
    splitsizes << 115;
    m3DOptionsSplitter->setSizes( splitsizes );
  }
  m3DOptionsSplitter->restoreState( settings.value( QStringLiteral( "Windows/3DMapConfig/OptionsSplitState" ) ).toByteArray() );

  mMeshSymbolWidget = new QgsMesh3dSymbolWidget( nullptr, groupMeshTerrainShading );
  mMeshSymbolWidget->configureForTerrain();

  cboCameraProjectionType->addItem( tr( "Perspective Projection" ), Qt3DRender::QCameraLens::PerspectiveProjection );
  cboCameraProjectionType->addItem( tr( "Orthogonal Projection" ), Qt3DRender::QCameraLens::OrthographicProjection );
  connect( cboCameraProjectionType, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, [ = ]()
  {
    spinCameraFieldOfView->setEnabled( cboCameraProjectionType->currentIndex() == cboCameraProjectionType->findData( Qt3DRender::QCameraLens::PerspectiveProjection ) );
  } );

  mCameraMovementSpeed->setClearValue( 4 );
  spinCameraFieldOfView->setClearValue( 45.0 );
  spinTerrainScale->setClearValue( 1.0 );
  spinTerrainResolution->setClearValue( 16 );
  spinTerrainSkirtHeight->setClearValue( 10 );
  spinMapResolution->setClearValue( 512 );
  spinScreenError->setClearValue( 3 );
  spinGroundError->setClearValue( 1 );
  terrainElevationOffsetSpinBox->setClearValue( 0.0 );
  edlStrengthSpinBox->setClearValue( 1000 );
  edlDistanceSpinBox->setClearValue( 1 );
  mDebugShadowMapSizeSpinBox->setClearValue( 0.1 );
  mDebugDepthMapSizeSpinBox->setClearValue( 0.1 );

  cboTerrainLayer->setAllowEmptyLayer( true );
  cboTerrainLayer->setFilters( QgsMapLayerProxyModel::RasterLayer );

  cboTerrainType->addItem( tr( "Flat Terrain" ), QgsTerrainGenerator::Flat );
  cboTerrainType->addItem( tr( "DEM (Raster Layer)" ), QgsTerrainGenerator::Dem );
  cboTerrainType->addItem( tr( "Online" ), QgsTerrainGenerator::Online );
  cboTerrainType->addItem( tr( "Mesh" ), QgsTerrainGenerator::Mesh );

  groupTerrain->setChecked( mMapSettings->terrainRenderingEnabled() );

  QgsTerrainGenerator *terrainGen = mMapSettings->terrainGenerator();
  if ( terrainGen && terrainGen->type() == QgsTerrainGenerator::Dem )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Dem ) );
    QgsDemTerrainGenerator *demTerrainGen = static_cast<QgsDemTerrainGenerator *>( terrainGen );
    spinTerrainResolution->setValue( demTerrainGen->resolution() );
    spinTerrainSkirtHeight->setValue( demTerrainGen->skirtHeight() );
    cboTerrainLayer->setLayer( demTerrainGen->layer() );
    cboTerrainLayer->setFilters( QgsMapLayerProxyModel::RasterLayer );
  }
  else if ( terrainGen && terrainGen->type() == QgsTerrainGenerator::Online )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Online ) );
    QgsOnlineTerrainGenerator *onlineTerrainGen = static_cast<QgsOnlineTerrainGenerator *>( terrainGen );
    spinTerrainResolution->setValue( onlineTerrainGen->resolution() );
    spinTerrainSkirtHeight->setValue( onlineTerrainGen->skirtHeight() );
  }
  else if ( terrainGen && terrainGen->type() == QgsTerrainGenerator::Mesh )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Mesh ) );
    QgsMeshTerrainGenerator *meshTerrain = static_cast<QgsMeshTerrainGenerator *>( terrainGen );
    cboTerrainLayer->setFilters( QgsMapLayerProxyModel::MeshLayer );
    cboTerrainLayer->setLayer( meshTerrain->meshLayer() );
    mMeshSymbolWidget->setLayer( meshTerrain->meshLayer(), false );
    mMeshSymbolWidget->setSymbol( meshTerrain->symbol() );
    spinTerrainScale->setValue( meshTerrain->symbol()->verticalScale() );
  }
  else if ( terrainGen )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Flat ) );
    cboTerrainLayer->setLayer( nullptr );
    spinTerrainResolution->setValue( 16 );
    spinTerrainSkirtHeight->setValue( 10 );
  }

  spinCameraFieldOfView->setValue( mMapSettings->fieldOfView() );
  cboCameraProjectionType->setCurrentIndex( cboCameraProjectionType->findData( mMapSettings->projectionType() ) );
  mCameraNavigationModeCombo->setCurrentIndex( mCameraNavigationModeCombo->findData( mMapSettings->cameraNavigationMode() ) );
  mCameraMovementSpeed->setValue( mMapSettings->cameraMovementSpeed() );
  spinTerrainScale->setValue( mMapSettings->terrainVerticalScale() );
  spinMapResolution->setValue( mMapSettings->mapTileResolution() );
  spinScreenError->setValue( mMapSettings->maxTerrainScreenError() );
  spinGroundError->setValue( mMapSettings->maxTerrainGroundError() );
  terrainElevationOffsetSpinBox->setValue( mMapSettings->terrainElevationOffset() );
  chkShowLabels->setChecked( mMapSettings->showLabels() );
  chkShowTileInfo->setChecked( mMapSettings->showTerrainTilesInfo() );
  chkShowBoundingBoxes->setChecked( mMapSettings->showTerrainBoundingBoxes() );
  chkShowCameraViewCenter->setChecked( mMapSettings->showCameraViewCenter() );
  chkShowCameraRotationCenter->setChecked( mMapSettings->showCameraRotationCenter() );
  chkShowLightSourceOrigins->setChecked( mMapSettings->showLightSourceOrigins() );
  mFpsCounterCheckBox->setChecked( mMapSettings->isFpsCounterEnabled() );

  groupTerrainShading->setChecked( mMapSettings->isTerrainShadingEnabled() );
  widgetTerrainMaterial->setTechnique( QgsMaterialSettingsRenderingTechnique::TrianglesWithFixedTexture );
  QgsPhongMaterialSettings terrainShadingMaterial = mMapSettings->terrainShadingMaterial();
  widgetTerrainMaterial->setSettings( &terrainShadingMaterial, nullptr );

  widgetLights->setLights( mMapSettings->lightSources() );

  connect( cboTerrainType, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, &Qgs3DMapConfigWidget::onTerrainTypeChanged );
  connect( cboTerrainLayer, static_cast<void ( QComboBox::* )( int )>( &QgsMapLayerComboBox::currentIndexChanged ), this, &Qgs3DMapConfigWidget::onTerrainLayerChanged );
  connect( spinMapResolution, static_cast<void ( QSpinBox::* )( int )>( &QSpinBox::valueChanged ), this, &Qgs3DMapConfigWidget::updateMaxZoomLevel );
  connect( spinGroundError, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), this, &Qgs3DMapConfigWidget::updateMaxZoomLevel );

  groupMeshTerrainShading->layout()->addWidget( mMeshSymbolWidget );

  onTerrainTypeChanged();

  mSkyboxSettingsWidget = new QgsSkyboxRenderingSettingsWidget( this );
  mSkyboxSettingsWidget->setSkyboxSettings( mapSettings->skyboxSettings() );
  groupSkyboxSettings->layout()->addWidget( mSkyboxSettingsWidget );
  groupSkyboxSettings->setChecked( mMapSettings->isSkyboxEnabled() );

  mShadowSettingsWidget = new QgsShadowRenderingSettingsWidget( this );
  mShadowSettingsWidget->onDirectionalLightsCountChanged( widgetLights->directionalLightCount() );
  mShadowSettingsWidget->setShadowSettings( mapSettings->shadowSettings() );
  groupShadowRendering->layout()->addWidget( mShadowSettingsWidget );
  connect( widgetLights, &QgsLightsWidget::directionalLightsCountChanged, mShadowSettingsWidget, &QgsShadowRenderingSettingsWidget::onDirectionalLightsCountChanged );

  connect( widgetLights, &QgsLightsWidget::lightsAdded, this, &Qgs3DMapConfigWidget::validate );
  connect( widgetLights, &QgsLightsWidget::lightsRemoved, this, &Qgs3DMapConfigWidget::validate );

  groupShadowRendering->setChecked( mapSettings->shadowSettings().renderShadows() );

  edlGroupBox->setChecked( mapSettings->eyeDomeLightingEnabled() );
  edlStrengthSpinBox->setValue( mapSettings->eyeDomeLightingStrength() );
  edlDistanceSpinBox->setValue( mapSettings->eyeDomeLightingDistance() );


  mSync2DTo3DCheckbox->setChecked( mapSettings->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D ) );
  mSync3DTo2DCheckbox->setChecked( mapSettings->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) );
  mVisualizeExtentCheckBox->setChecked( mapSettings->viewFrustumVisualizationEnabled() );

  mDebugShadowMapCornerComboBox->addItem( tr( "Top Left" ) );
  mDebugShadowMapCornerComboBox->addItem( tr( "Top Right" ) );
  mDebugShadowMapCornerComboBox->addItem( tr( "Bottom Left" ) );
  mDebugShadowMapCornerComboBox->addItem( tr( "Bottom Right" ) );

  mDebugDepthMapCornerComboBox->addItem( tr( "Top Left" ) );
  mDebugDepthMapCornerComboBox->addItem( tr( "Top Right" ) );
  mDebugDepthMapCornerComboBox->addItem( tr( "Bottom Left" ) );
  mDebugDepthMapCornerComboBox->addItem( tr( "Bottom Right" ) );

  mDebugShadowMapGroupBox->setChecked( mapSettings->debugShadowMapEnabled() );

  mDebugShadowMapCornerComboBox->setCurrentIndex( static_cast<int>( mapSettings->debugShadowMapCorner() ) );
  mDebugShadowMapSizeSpinBox->setValue( mapSettings->debugShadowMapSize() );

  mDebugDepthMapGroupBox->setChecked( mapSettings->debugDepthMapEnabled() );
  mDebugDepthMapCornerComboBox->setCurrentIndex( static_cast<int>( mapSettings->debugDepthMapCorner() ) );
  mDebugDepthMapSizeSpinBox->setValue( mapSettings->debugDepthMapSize() );
}

Qgs3DMapConfigWidget::~Qgs3DMapConfigWidget()
{
  QgsSettings settings;
  settings.setValue( QStringLiteral( "Windows/3DMapConfig/OptionsSplitState" ), m3DOptionsSplitter->saveState() );
  settings.setValue( QStringLiteral( "Windows/3DMapConfig/Tab" ), m3DOptionsListWidget->currentRow() );
}

void Qgs3DMapConfigWidget::apply()
{
  bool needsUpdateOrigin = false;

  const QgsReferencedRectangle extent = QgsProject::instance()->viewSettings()->fullExtent();
  QgsCoordinateTransform ct( extent.crs(), mMapSettings->crs(), QgsProject::instance()->transformContext() );
  ct.setBallparkTransformsAreAppropriate( true );
  QgsRectangle rect;
  try
  {
    rect = ct.transformBoundingBox( extent );
  }
  catch ( QgsCsException & )
  {
    rect = extent;
  }

  const QgsTerrainGenerator::Type terrainType = static_cast<QgsTerrainGenerator::Type>( cboTerrainType->currentData().toInt() );

  mMapSettings->setTerrainRenderingEnabled( groupTerrain->isChecked() );
  switch ( terrainType )
  {
    case QgsTerrainGenerator::Flat:
    {
      QgsFlatTerrainGenerator *flatTerrainGen = new QgsFlatTerrainGenerator;
      flatTerrainGen->setCrs( mMapSettings->crs() );
      flatTerrainGen->setExtent( rect );
      mMapSettings->setTerrainGenerator( flatTerrainGen );
      needsUpdateOrigin = true;
    }
    break;
    case QgsTerrainGenerator::Dem:
    {
      QgsRasterLayer *demLayer = qobject_cast<QgsRasterLayer *>( cboTerrainLayer->currentLayer() );

      bool tGenNeedsUpdate = true;
      if ( mMapSettings->terrainGenerator()->type() == QgsTerrainGenerator::Dem )
      {
        // if we already have a DEM terrain generator, check whether there was actually any change
        QgsDemTerrainGenerator *oldDemTerrainGen = static_cast<QgsDemTerrainGenerator *>( mMapSettings->terrainGenerator() );
        if ( oldDemTerrainGen->layer() == demLayer &&
             oldDemTerrainGen->resolution() == spinTerrainResolution->value() &&
             oldDemTerrainGen->skirtHeight() == spinTerrainSkirtHeight->value() )
          tGenNeedsUpdate = false;
      }

      if ( tGenNeedsUpdate )
      {
        QgsDemTerrainGenerator *demTerrainGen = new QgsDemTerrainGenerator;
        demTerrainGen->setCrs( mMapSettings->crs(), QgsProject::instance()->transformContext() );
        demTerrainGen->setLayer( demLayer );
        demTerrainGen->setResolution( spinTerrainResolution->value() );
        demTerrainGen->setSkirtHeight( spinTerrainSkirtHeight->value() );
        mMapSettings->setTerrainGenerator( demTerrainGen );
        needsUpdateOrigin = true;
      }
    }
    break;
    case QgsTerrainGenerator::Online:
    {
      bool tGenNeedsUpdate = true;
      if ( mMapSettings->terrainGenerator()->type() == QgsTerrainGenerator::Online )
      {
        QgsOnlineTerrainGenerator *oldOnlineTerrainGen = static_cast<QgsOnlineTerrainGenerator *>( mMapSettings->terrainGenerator() );
        if ( oldOnlineTerrainGen->resolution() == spinTerrainResolution->value() &&
             oldOnlineTerrainGen->skirtHeight() == spinTerrainSkirtHeight->value() )
          tGenNeedsUpdate = false;
      }

      if ( tGenNeedsUpdate )
      {
        QgsOnlineTerrainGenerator *onlineTerrainGen = new QgsOnlineTerrainGenerator;
        onlineTerrainGen->setCrs( mMapSettings->crs(), QgsProject::instance()->transformContext() );
        onlineTerrainGen->setExtent( rect );
        onlineTerrainGen->setResolution( spinTerrainResolution->value() );
        onlineTerrainGen->setSkirtHeight( spinTerrainSkirtHeight->value() );
        mMapSettings->setTerrainGenerator( onlineTerrainGen );
        needsUpdateOrigin = true;
      }
    }
    break;
    case QgsTerrainGenerator::Mesh:
    {
      QgsMeshLayer *meshLayer = qobject_cast<QgsMeshLayer *>( cboTerrainLayer->currentLayer() );
      QgsMeshTerrainGenerator *newTerrainGenerator = new QgsMeshTerrainGenerator;
      newTerrainGenerator->setCrs( mMapSettings->crs(), QgsProject::instance()->transformContext() );
      newTerrainGenerator->setLayer( meshLayer );
      std::unique_ptr< QgsMesh3DSymbol > symbol = mMeshSymbolWidget->symbol();
      symbol->setVerticalScale( spinTerrainScale->value() );
      newTerrainGenerator->setSymbol( symbol.release() );
      mMapSettings->setTerrainGenerator( newTerrainGenerator );
      needsUpdateOrigin = true;
    }
    break;
  }

  if ( needsUpdateOrigin && mMapSettings->terrainRenderingEnabled() && mMapSettings->terrainGenerator() )
  {
    const QgsRectangle te = m3DMapCanvas->scene()->sceneExtent();

    const QgsPointXY center = te.center();
    mMapSettings->setOrigin( QgsVector3D( center.x(), center.y(), 0 ) );
  }

  mMapSettings->setFieldOfView( spinCameraFieldOfView->value() );
  mMapSettings->setProjectionType( cboCameraProjectionType->currentData().value< Qt3DRender::QCameraLens::ProjectionType >() );
  mMapSettings->setCameraNavigationMode( static_cast<QgsCameraController::NavigationMode>( mCameraNavigationModeCombo->currentData().toInt() ) );
  mMapSettings->setCameraMovementSpeed( mCameraMovementSpeed->value() );
  mMapSettings->setTerrainVerticalScale( spinTerrainScale->value() );
  mMapSettings->setMapTileResolution( spinMapResolution->value() );
  mMapSettings->setMaxTerrainScreenError( spinScreenError->value() );
  mMapSettings->setMaxTerrainGroundError( spinGroundError->value() );
  mMapSettings->setTerrainElevationOffset( terrainElevationOffsetSpinBox->value() );
  mMapSettings->setShowLabels( chkShowLabels->isChecked() );
  mMapSettings->setShowTerrainTilesInfo( chkShowTileInfo->isChecked() );
  mMapSettings->setShowTerrainBoundingBoxes( chkShowBoundingBoxes->isChecked() );
  mMapSettings->setShowCameraViewCenter( chkShowCameraViewCenter->isChecked() );
  mMapSettings->setShowCameraRotationCenter( chkShowCameraRotationCenter->isChecked() );
  mMapSettings->setShowLightSourceOrigins( chkShowLightSourceOrigins->isChecked() );
  mMapSettings->setIsFpsCounterEnabled( mFpsCounterCheckBox->isChecked() );
  mMapSettings->setTerrainShadingEnabled( groupTerrainShading->isChecked() );

  const std::unique_ptr< QgsAbstractMaterialSettings > terrainMaterial( widgetTerrainMaterial->settings() );
  if ( QgsPhongMaterialSettings *phongMaterial = dynamic_cast< QgsPhongMaterialSettings * >( terrainMaterial.get() ) )
    mMapSettings->setTerrainShadingMaterial( *phongMaterial );

  mMapSettings->setLightSources( widgetLights->lightSources() );
  mMapSettings->setIsSkyboxEnabled( groupSkyboxSettings->isChecked() );
  mMapSettings->setSkyboxSettings( mSkyboxSettingsWidget->toSkyboxSettings() );
  QgsShadowSettings shadowSettings = mShadowSettingsWidget->toShadowSettings();
  shadowSettings.setRenderShadows( groupShadowRendering->isChecked() );
  mMapSettings->setShadowSettings( shadowSettings );

  mMapSettings->setEyeDomeLightingEnabled( edlGroupBox->isChecked() );
  mMapSettings->setEyeDomeLightingStrength( edlStrengthSpinBox->value() );
  mMapSettings->setEyeDomeLightingDistance( edlDistanceSpinBox->value() );

  Qgis::ViewSyncModeFlags viewSyncMode;
  viewSyncMode.setFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D, mSync2DTo3DCheckbox->isChecked() );
  viewSyncMode.setFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D, mSync3DTo2DCheckbox->isChecked() );
  mMapSettings->setViewSyncMode( viewSyncMode );
  mMapSettings->setViewFrustumVisualizationEnabled( mVisualizeExtentCheckBox->isChecked() );

  mMapSettings->setDebugDepthMapSettings( mDebugDepthMapGroupBox->isChecked(), static_cast<Qt::Corner>( mDebugDepthMapCornerComboBox->currentIndex() ), mDebugDepthMapSizeSpinBox->value() );
  mMapSettings->setDebugShadowMapSettings( mDebugShadowMapGroupBox->isChecked(), static_cast<Qt::Corner>( mDebugShadowMapCornerComboBox->currentIndex() ), mDebugShadowMapSizeSpinBox->value() );
}

void Qgs3DMapConfigWidget::onTerrainTypeChanged()
{
  const QgsTerrainGenerator::Type genType = static_cast<QgsTerrainGenerator::Type>( cboTerrainType->currentData().toInt() );

  labelTerrainResolution->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::Mesh ) );
  spinTerrainResolution->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::Mesh ) );
  labelTerrainSkirtHeight->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::Mesh ) );
  spinTerrainSkirtHeight->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::Mesh ) );
  labelTerrainLayer->setVisible( genType == QgsTerrainGenerator::Dem || genType == QgsTerrainGenerator::Mesh );
  cboTerrainLayer->setVisible( genType == QgsTerrainGenerator::Dem || genType == QgsTerrainGenerator::Mesh );
  groupMeshTerrainShading->setVisible( genType == QgsTerrainGenerator::Mesh );
  groupTerrainShading->setVisible( genType != QgsTerrainGenerator::Mesh );

  QgsMapLayer *oldTerrainLayer = cboTerrainLayer->currentLayer();
  if ( cboTerrainType->currentData() == QgsTerrainGenerator::Dem )
  {
    cboTerrainLayer->setFilters( QgsMapLayerProxyModel::RasterLayer );
  }
  else if ( cboTerrainType->currentData() == QgsTerrainGenerator::Mesh )
  {
    cboTerrainLayer->setFilters( QgsMapLayerProxyModel::MeshLayer );
  }

  if ( cboTerrainLayer->currentLayer() != oldTerrainLayer )
    onTerrainLayerChanged();

  updateMaxZoomLevel();
  validate();
}

void Qgs3DMapConfigWidget::onTerrainLayerChanged()
{
  updateMaxZoomLevel();

  if ( cboTerrainType->currentData() == QgsTerrainGenerator::Mesh )
  {
    QgsMeshLayer *meshLayer = qobject_cast<QgsMeshLayer *>( cboTerrainLayer->currentLayer() );
    if ( meshLayer )
    {
      QgsMeshLayer *oldLayer = mMeshSymbolWidget->meshLayer();

      mMeshSymbolWidget->setLayer( meshLayer, false );
      if ( oldLayer != meshLayer )
        mMeshSymbolWidget->reloadColorRampShaderMinMax();
    }
  }
  validate();
}

void Qgs3DMapConfigWidget::updateMaxZoomLevel()
{
  QgsRectangle te;
  const QgsTerrainGenerator::Type terrainType = static_cast<QgsTerrainGenerator::Type>( cboTerrainType->currentData().toInt() );
  if ( terrainType == QgsTerrainGenerator::Dem )
  {
    if ( QgsRasterLayer *demLayer = qobject_cast<QgsRasterLayer *>( cboTerrainLayer->currentLayer() ) )
    {
      te = demLayer->extent();
      QgsCoordinateTransform terrainToMapTransform( demLayer->crs(), mMapSettings->crs(), QgsProject::instance()->transformContext() );
      terrainToMapTransform.setBallparkTransformsAreAppropriate( true );
      te = terrainToMapTransform.transformBoundingBox( te );
    }
  }
  else if ( terrainType == QgsTerrainGenerator::Mesh )
  {
    if ( QgsMeshLayer *meshLayer = qobject_cast<QgsMeshLayer *>( cboTerrainLayer->currentLayer() ) )
    {
      te = meshLayer->extent();
      QgsCoordinateTransform terrainToMapTransform( meshLayer->crs(), mMapSettings->crs(), QgsProject::instance()->transformContext() );
      terrainToMapTransform.setBallparkTransformsAreAppropriate( true );
      te = terrainToMapTransform.transformBoundingBox( te );
    }
  }
  else  // flat or online
  {
    te = mMainCanvas->projectExtent();
  }

  const double tile0width = std::max( te.width(), te.height() );
  const int zoomLevel = Qgs3DUtils::maxZoomLevel( tile0width, spinMapResolution->value(), spinGroundError->value() );
  labelZoomLevels->setText( QStringLiteral( "0 - %1" ).arg( zoomLevel ) );
}

void Qgs3DMapConfigWidget::validate()
{
  mMessageBar->clearWidgets();

  bool valid = true;
  switch ( static_cast<QgsTerrainGenerator::Type>( cboTerrainType->currentData().toInt() ) )
  {
    case QgsTerrainGenerator::Dem:
      if ( ! cboTerrainLayer->currentLayer() )
      {
        valid = false;
        mMessageBar->pushMessage( tr( "An elevation layer must be selected for a DEM terrain" ), Qgis::MessageLevel::Critical );
      }
      break;

    case QgsTerrainGenerator::Mesh:
      if ( ! cboTerrainLayer->currentLayer() )
      {
        valid = false;
        mMessageBar->pushMessage( tr( "An elevation layer must be selected for a mesh terrain" ), Qgis::MessageLevel::Critical );
      }
      break;

    case QgsTerrainGenerator::Online:
    case QgsTerrainGenerator::Flat:
      break;
  }

  if ( valid && widgetLights->lightSourceCount() == 0 )
  {
    mMessageBar->pushMessage( tr( "No lights exist in the scene" ), Qgis::MessageLevel::Warning );
  }

  emit isValidChanged( valid );
}
