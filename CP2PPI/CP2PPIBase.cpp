/****************************************************************************
** Form implementation generated from reading ui file 'CP2PPIBase.ui'
**
** Created: Tue Jan 2 10:17:52 2007
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.5   edited Aug 31 12:13 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "CP2PPIBase.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <PPI/PPI.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlcdnumber.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qgroupbox.h>
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
    CP2PPIBaseLayout = new QHBoxLayout( centralWidget(), 11, 6, "CP2PPIBaseLayout"); 

    _ppi = new PPI( centralWidget(), "_ppi" );
    _ppi->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, _ppi->sizePolicy().hasHeightForWidth() ) );
    _ppi->setMinimumSize( QSize( 600, 600 ) );
    CP2PPIBaseLayout->addWidget( _ppi );

    frameColorBar = new QFrame( centralWidget(), "frameColorBar" );
    frameColorBar->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, frameColorBar->sizePolicy().hasHeightForWidth() ) );
    frameColorBar->setMinimumSize( QSize( 40, 400 ) );
    frameColorBar->setMaximumSize( QSize( 40, 32767 ) );
    frameColorBar->setFrameShape( QFrame::StyledPanel );
    frameColorBar->setFrameShadow( QFrame::Raised );
    CP2PPIBaseLayout->addWidget( frameColorBar );

    layout18 = new QVBoxLayout( 0, 0, 6, "layout18"); 

    layout14 = new QVBoxLayout( 0, 0, 6, "layout14"); 

    _pauseButton = new QPushButton( centralWidget(), "_pauseButton" );
    _pauseButton->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, _pauseButton->sizePolicy().hasHeightForWidth() ) );
    _pauseButton->setToggleButton( TRUE );
    layout14->addWidget( _pauseButton );

    layout13 = new QHBoxLayout( 0, 0, 6, "layout13"); 

    _zoomInButton = new QPushButton( centralWidget(), "_zoomInButton" );
    _zoomInButton->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, _zoomInButton->sizePolicy().hasHeightForWidth() ) );
    layout13->addWidget( _zoomInButton );

    _zoomOutButton = new QPushButton( centralWidget(), "_zoomOutButton" );
    _zoomOutButton->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, _zoomOutButton->sizePolicy().hasHeightForWidth() ) );
    layout13->addWidget( _zoomOutButton );
    layout14->addLayout( layout13 );

    layout12 = new QHBoxLayout( 0, 0, 6, "layout12"); 

    textLabel1 = new QLabel( centralWidget(), "textLabel1" );
    textLabel1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, textLabel1->sizePolicy().hasHeightForWidth() ) );
    layout12->addWidget( textLabel1 );

    ZoomFactor = new QLCDNumber( centralWidget(), "ZoomFactor" );
    ZoomFactor->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, ZoomFactor->sizePolicy().hasHeightForWidth() ) );
    ZoomFactor->setSegmentStyle( QLCDNumber::Flat );
    layout12->addWidget( ZoomFactor );
    layout14->addLayout( layout12 );
    layout18->addLayout( layout14 );

    _typeTab = new QTabWidget( centralWidget(), "_typeTab" );
    _typeTab->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)3, 0, 0, _typeTab->sizePolicy().hasHeightForWidth() ) );

    tab = new QWidget( _typeTab, "tab" );
    _typeTab->insertTab( tab, QString::fromLatin1("") );
    layout18->addWidget( _typeTab );
    spacer3 = new QSpacerItem( 20, 156, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding );
    layout18->addItem( spacer3 );

    groupBox1 = new QGroupBox( centralWidget(), "groupBox1" );
    groupBox1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, groupBox1->sizePolicy().hasHeightForWidth() ) );
    groupBox1->setColumnLayout(0, Qt::Vertical );
    groupBox1->layout()->setSpacing( 6 );
    groupBox1->layout()->setMargin( 11 );
    groupBox1Layout = new QGridLayout( groupBox1->layout() );
    groupBox1Layout->setAlignment( Qt::AlignTop );

    textLabel1_2 = new QLabel( groupBox1, "textLabel1_2" );
    textLabel1_2->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, textLabel1_2->sizePolicy().hasHeightForWidth() ) );

    groupBox1Layout->addWidget( textLabel1_2, 0, 4 );

    textLabel3_2 = new QLabel( groupBox1, "textLabel3_2" );
    textLabel3_2->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, textLabel3_2->sizePolicy().hasHeightForWidth() ) );
    textLabel3_2->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( textLabel3_2, 0, 2 );

    _chan0pulseRate = new QLabel( groupBox1, "_chan0pulseRate" );
    _chan0pulseRate->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, _chan0pulseRate->sizePolicy().hasHeightForWidth() ) );
    _chan0pulseRate->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( _chan0pulseRate, 1, 2 );

    _chan1pulseRate = new QLabel( groupBox1, "_chan1pulseRate" );
    _chan1pulseRate->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, _chan1pulseRate->sizePolicy().hasHeightForWidth() ) );
    _chan1pulseRate->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( _chan1pulseRate, 2, 2 );

    _chan2pulseRate = new QLabel( groupBox1, "_chan2pulseRate" );
    _chan2pulseRate->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, _chan2pulseRate->sizePolicy().hasHeightForWidth() ) );
    _chan2pulseRate->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( _chan2pulseRate, 3, 2 );

    textLabel3_3 = new QLabel( groupBox1, "textLabel3_3" );
    textLabel3_3->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, textLabel3_3->sizePolicy().hasHeightForWidth() ) );
    textLabel3_3->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( textLabel3_3, 0, 3 );

    _chan0errors = new QLabel( groupBox1, "_chan0errors" );
    _chan0errors->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, _chan0errors->sizePolicy().hasHeightForWidth() ) );
    _chan0errors->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( _chan0errors, 1, 3 );

    _chan1errors = new QLabel( groupBox1, "_chan1errors" );
    _chan1errors->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, _chan1errors->sizePolicy().hasHeightForWidth() ) );
    _chan1errors->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( _chan1errors, 2, 3 );

    _chan2errors = new QLabel( groupBox1, "_chan2errors" );
    _chan2errors->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, _chan2errors->sizePolicy().hasHeightForWidth() ) );
    _chan2errors->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( _chan2errors, 3, 3 );

    textLabel3 = new QLabel( groupBox1, "textLabel3" );
    textLabel3->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, textLabel3->sizePolicy().hasHeightForWidth() ) );
    textLabel3->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( textLabel3, 0, 1 );

    _chan0pulseCount = new QLabel( groupBox1, "_chan0pulseCount" );
    _chan0pulseCount->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, _chan0pulseCount->sizePolicy().hasHeightForWidth() ) );
    _chan0pulseCount->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( _chan0pulseCount, 1, 1 );

    _chan1pulseCount = new QLabel( groupBox1, "_chan1pulseCount" );
    _chan1pulseCount->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, _chan1pulseCount->sizePolicy().hasHeightForWidth() ) );
    _chan1pulseCount->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( _chan1pulseCount, 2, 1 );

    _chan2pulseCount = new QLabel( groupBox1, "_chan2pulseCount" );
    _chan2pulseCount->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, _chan2pulseCount->sizePolicy().hasHeightForWidth() ) );
    _chan2pulseCount->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( _chan2pulseCount, 3, 1 );

    textLabel1_5 = new QLabel( groupBox1, "textLabel1_5" );
    textLabel1_5->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, textLabel1_5->sizePolicy().hasHeightForWidth() ) );
    textLabel1_5->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( textLabel1_5, 1, 0 );

    textLabel1_5_2 = new QLabel( groupBox1, "textLabel1_5_2" );
    textLabel1_5_2->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, textLabel1_5_2->sizePolicy().hasHeightForWidth() ) );
    textLabel1_5_2->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( textLabel1_5_2, 2, 0 );

    textLabel1_5_3 = new QLabel( groupBox1, "textLabel1_5_3" );
    textLabel1_5_3->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, textLabel1_5_3->sizePolicy().hasHeightForWidth() ) );
    textLabel1_5_3->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( textLabel1_5_3, 3, 0 );

    textLabel4 = new QLabel( groupBox1, "textLabel4" );
    textLabel4->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, textLabel4->sizePolicy().hasHeightForWidth() ) );
    textLabel4->setAlignment( int( QLabel::AlignCenter ) );

    groupBox1Layout->addWidget( textLabel4, 0, 0 );

    _chan0led = new QLabel( groupBox1, "_chan0led" );
    _chan0led->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, 0, 0, _chan0led->sizePolicy().hasHeightForWidth() ) );
    _chan0led->setMinimumSize( QSize( 16, 16 ) );
    _chan0led->setMaximumSize( QSize( 16, 16 ) );
    _chan0led->setFocusPolicy( QLabel::NoFocus );
    _chan0led->setFrameShape( QLabel::Panel );
    _chan0led->setFrameShadow( QLabel::Raised );

    groupBox1Layout->addWidget( _chan0led, 1, 4 );

    _chan2led = new QLabel( groupBox1, "_chan2led" );
    _chan2led->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, 0, 0, _chan2led->sizePolicy().hasHeightForWidth() ) );
    _chan2led->setMinimumSize( QSize( 16, 16 ) );
    _chan2led->setMaximumSize( QSize( 16, 16 ) );
    _chan2led->setFocusPolicy( QLabel::NoFocus );
    _chan2led->setFrameShape( QLabel::Panel );
    _chan2led->setFrameShadow( QLabel::Raised );

    groupBox1Layout->addWidget( _chan2led, 3, 4 );

    _chan1led = new QLabel( groupBox1, "_chan1led" );
    _chan1led->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, 0, 0, _chan1led->sizePolicy().hasHeightForWidth() ) );
    _chan1led->setMinimumSize( QSize( 16, 16 ) );
    _chan1led->setMaximumSize( QSize( 16, 16 ) );
    _chan1led->setFocusPolicy( QLabel::NoFocus );
    _chan1led->setFrameShape( QLabel::Panel );
    _chan1led->setFrameShadow( QLabel::Raised );

    groupBox1Layout->addWidget( _chan1led, 2, 4 );
    layout18->addWidget( groupBox1 );

    layout29 = new QHBoxLayout( 0, 0, 6, "layout29"); 
    spacer2 = new QSpacerItem( 61, 20, QSizePolicy::Minimum, QSizePolicy::Minimum );
    layout29->addItem( spacer2 );

    layout40 = new QHBoxLayout( 0, 0, 6, "layout40"); 

    m_pTextIPname = new QLabel( centralWidget(), "m_pTextIPname" );
    layout40->addWidget( m_pTextIPname );

    m_pTextIPaddress = new QLabel( centralWidget(), "m_pTextIPaddress" );
    m_pTextIPaddress->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)5, 0, 0, m_pTextIPaddress->sizePolicy().hasHeightForWidth() ) );
    layout40->addWidget( m_pTextIPaddress );
    layout29->addLayout( layout40 );
    layout18->addLayout( layout29 );
    CP2PPIBaseLayout->addLayout( layout18 );

    // toolbars

    languageChange();
    resize( QSize(904, 643).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect( _zoomInButton, SIGNAL( clicked() ), this, SLOT( zoomInSlot() ) );
    connect( _zoomOutButton, SIGNAL( clicked() ), this, SLOT( zoomOutSlot() ) );
    connect( _pauseButton, SIGNAL( toggled(bool) ), this, SLOT( pauseSlot(bool) ) );
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
    _pauseButton->setText( tr( "Pause" ) );
    _zoomInButton->setText( tr( "Zoom in" ) );
    _zoomOutButton->setText( tr( "Zoom out" ) );
    textLabel1->setText( tr( "Zoom:" ) );
    _typeTab->changeTab( tab, tr( "Tab 1" ) );
    groupBox1->setTitle( tr( "Pulse Statistics" ) );
    textLabel1_2->setText( tr( "EOF" ) );
    textLabel3_2->setText( tr( "Rate (/s)" ) );
    _chan0pulseRate->setText( tr( "0" ) );
    _chan1pulseRate->setText( tr( "0" ) );
    _chan2pulseRate->setText( tr( "0" ) );
    textLabel3_3->setText( tr( "Errors" ) );
    _chan0errors->setText( tr( "0" ) );
    _chan1errors->setText( tr( "0" ) );
    _chan2errors->setText( tr( "0" ) );
    textLabel3->setText( tr( "Number (K)" ) );
    _chan0pulseCount->setText( tr( "0" ) );
    _chan1pulseCount->setText( tr( "0" ) );
    _chan2pulseCount->setText( tr( "0" ) );
    textLabel1_5->setText( tr( "S" ) );
    textLabel1_5_2->setText( tr( "Kh" ) );
    textLabel1_5_3->setText( tr( "Kv" ) );
    textLabel4->setText( tr( "Channel" ) );
    _chan0led->setText( QString::null );
    _chan2led->setText( QString::null );
    _chan1led->setText( QString::null );
    m_pTextIPname->setText( tr( "IP Name" ) );
    m_pTextIPaddress->setText( tr( "xxx.xxx.xxx.xxx" ) );
}

void CP2PPIBase::pauseSlot(bool)
{
    qWarning( "CP2PPIBase::pauseSlot(bool): Not implemented yet" );
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

