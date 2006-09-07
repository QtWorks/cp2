/****************************************************************************
** Form implementation generated from reading ui file 'CP2PPIBase.ui'
**
** Created: Fri Aug 25 17:58:39 2006
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.6-snapshot-20060428   edited Apr 2 18:47 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "CP2PPIBase.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlcdnumber.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <PPI/PPI.h>
#include <qframe.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qaction.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qtoolbar.h>
#include "PPI/PPI.h"

/*
 *  Constructs a CP2PPIBase as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
CP2PPIBase::CP2PPIBase( QWidget* parent, const char* name, WFlags fl )
    : QMainWindow( parent, name, fl )
{
    (void)statusBar();
    if ( !name )
	setName( "CP2PPIBase" );
    setCentralWidget( new QWidget( this, "qt_central_widget" ) );

    groupBox1 = new QGroupBox( centralWidget(), "groupBox1" );
    groupBox1->setGeometry( QRect( 30, 440, 570, 110 ) );

    _startButton = new QPushButton( groupBox1, "_startButton" );
    _startButton->setGeometry( QRect( 11, 20, 130, 23 ) );

    _zoomOutButton = new QPushButton( groupBox1, "_zoomOutButton" );
    _zoomOutButton->setGeometry( QRect( 10, 80, 80, 23 ) );

    _zoomInButton = new QPushButton( groupBox1, "_zoomInButton" );
    _zoomInButton->setGeometry( QRect( 10, 50, 80, 23 ) );

    textLabel1 = new QLabel( groupBox1, "textLabel1" );
    textLabel1->setGeometry( QRect( 107, 69, 50, 20 ) );

    ZoomFactor = new QLCDNumber( groupBox1, "ZoomFactor" );
    ZoomFactor->setGeometry( QRect( 150, 64, 70, 30 ) );
    ZoomFactor->setSegmentStyle( QLCDNumber::Flat );

    _dataSourceButtons = new QButtonGroup( groupBox1, "_dataSourceButtons" );
    _dataSourceButtons->setGeometry( QRect( 150, 10, 130, 50 ) );
    _dataSourceButtons->setProperty( "selectedId", -1 );
    _dataSourceButtons->setColumnLayout(0, Qt::Vertical );
    _dataSourceButtons->layout()->setSpacing( 6 );
    _dataSourceButtons->layout()->setMargin( 11 );
    _dataSourceButtonsLayout = new QHBoxLayout( _dataSourceButtons->layout() );
    _dataSourceButtonsLayout->setAlignment( Qt::AlignTop );

    _dataSourceRadarButton = new QRadioButton( _dataSourceButtons, "_dataSourceRadarButton" );
    _dataSourceButtonsLayout->addWidget( _dataSourceRadarButton );

    _dataSourceTestButton = new QRadioButton( _dataSourceButtons, "_dataSourceTestButton" );
    _dataSourceButtonsLayout->addWidget( _dataSourceTestButton );

    QWidget* privateLayoutWidget = new QWidget( centralWidget(), "layout5" );
    privateLayoutWidget->setGeometry( QRect( 20, 10, 614, 410 ) );
    layout5 = new QHBoxLayout( privateLayoutWidget, 11, 6, "layout5"); 

    _ppi = new PPI( privateLayoutWidget, "_ppi" );
    _ppi->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, _ppi->sizePolicy().hasHeightForWidth() ) );
    _ppi->setMinimumSize( QSize( 400, 400 ) );
    layout5->addWidget( _ppi );

    frameColorBar = new QFrame( privateLayoutWidget, "frameColorBar" );
    frameColorBar->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, frameColorBar->sizePolicy().hasHeightForWidth() ) );
    frameColorBar->setMinimumSize( QSize( 40, 400 ) );
    frameColorBar->setMaximumSize( QSize( 40, 32767 ) );
    frameColorBar->setFrameShape( QFrame::StyledPanel );
    frameColorBar->setFrameShadow( QFrame::Raised );
    layout5->addWidget( frameColorBar );

    _productButtons = new QButtonGroup( privateLayoutWidget, "_productButtons" );
    _productButtons->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, _productButtons->sizePolicy().hasHeightForWidth() ) );
    _productButtons->setMinimumSize( QSize( 160, 400 ) );
    layout5->addWidget( _productButtons );

    // toolbars

    languageChange();
    resize( QSize(775, 562).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect( _dataSourceRadarButton, SIGNAL( toggled(bool) ), this, SLOT( dataSourceSlot() ) );
    connect( _dataSourceTestButton, SIGNAL( toggled(bool) ), this, SLOT( dataSourceSlot() ) );
    connect( _zoomInButton, SIGNAL( clicked() ), this, SLOT( zoomInSlot() ) );
    connect( _zoomOutButton, SIGNAL( clicked() ), this, SLOT( zoomOutSlot() ) );
    connect( _startButton, SIGNAL( clicked() ), this, SLOT( startStopDisplaySlot() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
CP2PPIBase::~CP2PPIBase()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void CP2PPIBase::languageChange()
{
    setCaption( tr( "CP2PPI" ) );
    groupBox1->setTitle( tr( "Control/Status" ) );
    _startButton->setText( tr( "Start/Stop Display" ) );
    _zoomOutButton->setText( tr( "Zoom out" ) );
    _zoomInButton->setText( tr( "Zoom in" ) );
    textLabel1->setText( tr( "Zoom:" ) );
    _dataSourceButtons->setTitle( tr( "Data Source" ) );
    _dataSourceRadarButton->setText( tr( "Radar" ) );
    _dataSourceTestButton->setText( tr( "Test" ) );
    _productButtons->setTitle( tr( "Product" ) );
}

void CP2PPIBase::startStopDisplaySlot()
{
    qWarning( "CP2PPIBase::startStopDisplaySlot(): Not implemented yet" );
}

void CP2PPIBase::dataSourceSlot()
{
    qWarning( "CP2PPIBase::dataSourceSlot(): Not implemented yet" );
}

void CP2PPIBase::zoomInSlot()
{
    qWarning( "CP2PPIBase::zoomInSlot(): Not implemented yet" );
}

void CP2PPIBase::zoomOutSlot()
{
    qWarning( "CP2PPIBase::zoomOutSlot(): Not implemented yet" );
}

