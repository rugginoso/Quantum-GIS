/***************************************************************************
                         testqgscomposermapatlas.cpp
                         ---------------------------
    begin                : Sept 2012
    copyright            : (C) 2012 by Hugo Mercier
    email                : hugo dot mercier at oslandia dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsapplication.h"
#include "qgscomposition.h"
#include "qgscompositionchecker.h"
#include "qgscomposermap.h"
#include "qgscomposerlabel.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaprenderer.h"
#include "qgsvectorlayer.h"
#include "qgsvectordataprovider.h"
#include "qgssymbolv2.h"
#include "qgssinglesymbolrendererv2.h"
#include <QObject>
#include <QtTest>

class TestQgsComposerMapAtlas: public QObject
{
    Q_OBJECT;
  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init();// will be called before each testfunction is executed.
    void cleanup();// will be called after every testfunction.

    // test filename pattern evaluation
    void filename();
    // test rendering with an autoscale atlas
    void autoscale_render();
    // test rendering with a fixed scale atlas
    void fixedscale_render();
    // test rendering with a hidden coverage
    void hiding_render();
  private:
    QgsComposition* mComposition;
    QgsComposerLabel* mLabel1;
    QgsComposerLabel* mLabel2;
    QgsComposerMap* mAtlasMap;
    QgsComposerMap* mOverview;
    QgsMapRenderer* mMapRenderer;
    QgsVectorLayer* mVectorLayer;
};

void TestQgsComposerMapAtlas::initTestCase()
{
  QgsApplication::init();
  QgsApplication::initQgis();

  //create maplayers from testdata and add to layer registry
  QFileInfo vectorFileInfo( QString( TEST_DATA_DIR ) + QDir::separator() +  "france_parts.shp" );
  mVectorLayer = new QgsVectorLayer( vectorFileInfo.filePath(),
                                     vectorFileInfo.completeBaseName(),
                                     "ogr" );

  QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer*>() << mVectorLayer );

  //create composition with composer map
  mMapRenderer = new QgsMapRenderer();
  mMapRenderer->setLayerSet( QStringList() << mVectorLayer->id() );
  mMapRenderer->setProjectionsEnabled( true );

  // select epsg:2154
  QgsCoordinateReferenceSystem crs;
  crs.createFromSrid( 2154 );
  mMapRenderer->setDestinationCrs( crs );
  mComposition = new QgsComposition( mMapRenderer );
  mComposition->setPaperSize( 297, 210 ); //A4 landscape

  // fix the renderer, fill with green
  QgsStringMap props;
  props.insert( "color", "0,127,0" );
  QgsFillSymbolV2* fillSymbol = QgsFillSymbolV2::createSimple( props );
  QgsSingleSymbolRendererV2* renderer = new QgsSingleSymbolRendererV2( fillSymbol );
  mVectorLayer->setRendererV2( renderer );

  // the atlas map
  mAtlasMap = new QgsComposerMap( mComposition, 20, 20, 130, 130 );
  mAtlasMap->setFrameEnabled( true );
  mAtlasMap->setAtlasCoverageLayer( mVectorLayer );
  mComposition->addComposerMap( mAtlasMap );
  mComposition->setAtlasMap( mAtlasMap );

  // an overview
  mOverview = new QgsComposerMap( mComposition, 180, 20, 50, 50 );
  mOverview->setFrameEnabled( true );
  mOverview->setOverviewFrameMap( mAtlasMap->id() );
  mComposition->addComposerMap( mOverview );
  mOverview->setNewExtent( QgsRectangle( 49670.718, 6415139.086, 699672.519, 7065140.887 ) );

  // header label
  mLabel1 = new QgsComposerLabel( mComposition );
  mComposition->addComposerLabel( mLabel1 );
  mLabel1->setText( "[% \"NAME_1\" %] area" );
  mLabel1->adjustSizeToText();
  mLabel1->setItemPosition( 150, 5 );

  // feature number label
  mLabel2 = new QgsComposerLabel( mComposition );
  mComposition->addComposerLabel( mLabel2 );
  mLabel2->setText( "# [%$feature || ' / ' || $numfeatures%]" );
  mLabel2->adjustSizeToText();
  mLabel2->setItemPosition( 150, 200 );
}

void TestQgsComposerMapAtlas::cleanupTestCase()
{
  delete mComposition;
  delete mMapRenderer;
  delete mVectorLayer;
}

void TestQgsComposerMapAtlas::init()
{

}

void TestQgsComposerMapAtlas::cleanup()
{

}

void TestQgsComposerMapAtlas::filename()
{
  QgsAtlasRendering atlasRender( mComposition );
  atlasRender.begin( "'output_' || $feature" );
  for ( size_t fi = 0; fi < atlasRender.numFeatures(); ++fi )
  {
    atlasRender.prepareForFeature( fi );
    QString expected = QString( "output_%1" ).arg(( int )( fi + 1 ) );
    QCOMPARE( atlasRender.currentFilename(), expected );
  }
  atlasRender.end();
}


void TestQgsComposerMapAtlas::autoscale_render()
{
  mAtlasMap->setAtlasFixedScale( false );
  mAtlasMap->setAtlasMargin( 0.10f );

  QgsAtlasRendering atlasRender( mComposition );

  atlasRender.begin();

  for ( size_t fit = 0; fit < 2; ++fit )
  {
    atlasRender.prepareForFeature( fit );
    mLabel1->adjustSizeToText();

    QgsCompositionChecker checker( "Atlas autoscale test", mComposition,
                                   QString( TEST_DATA_DIR ) + QDir::separator() + "control_images" + QDir::separator() +
                                   "expected_composermapatlas" + QDir::separator() +
                                   QString( "autoscale_%1.png" ).arg(( int )fit ) );
    QVERIFY( checker.testComposition( 0 ) );
  }
  atlasRender.end();
}

void TestQgsComposerMapAtlas::fixedscale_render()
{
  mAtlasMap->setNewExtent( QgsRectangle( 209838.166, 6528781.020, 610491.166, 6920530.620 ) );
  mAtlasMap->setAtlasFixedScale( true );

  QgsAtlasRendering atlasRender( mComposition );

  atlasRender.begin();

  for ( size_t fit = 0; fit < 2; ++fit )
  {
    atlasRender.prepareForFeature( fit );
    mLabel1->adjustSizeToText();

    QgsCompositionChecker checker( "Atlas fixedscale test", mComposition,
                                   QString( TEST_DATA_DIR ) + QDir::separator() + "control_images" + QDir::separator() +
                                   "expected_composermapatlas" + QDir::separator() +
                                   QString( "fixedscale_%1.png" ).arg(( int )fit ) );
    QVERIFY( checker.testComposition( 0 ) );
  }
  atlasRender.end();

}

void TestQgsComposerMapAtlas::hiding_render()
{
  mAtlasMap->setNewExtent( QgsRectangle( 209838.166, 6528781.020, 610491.166, 6920530.620 ) );
  mAtlasMap->setAtlasFixedScale( true );
  mAtlasMap->setAtlasHideCoverage( true );

  QgsAtlasRendering atlasRender( mComposition );

  atlasRender.begin();

  for ( size_t fit = 0; fit < 2; ++fit )
  {
    atlasRender.prepareForFeature( fit );
    mLabel1->adjustSizeToText();

    QgsCompositionChecker checker( "Atlas hidden test", mComposition,
                                   QString( TEST_DATA_DIR ) + QDir::separator() + "control_images" + QDir::separator() +
                                   "expected_composermapatlas" + QDir::separator() +
                                   QString( "hiding_%1.png" ).arg(( int )fit ) );
    QVERIFY( checker.testComposition( 0 ) );
  }
  atlasRender.end();

}

QTEST_MAIN( TestQgsComposerMapAtlas )
#include "moc_testqgscomposermapatlas.cxx"
