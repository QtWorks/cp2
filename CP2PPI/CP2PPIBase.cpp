/****************************************************************************
** Form implementation generated from reading ui file 'CP2PPIBase.ui'
**
** Created: Sat Dec 30 23:16:21 2006
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.5   edited Aug 31 12:13 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "CP2PPIBase.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <PPI/PPI.h>
#include <qframe.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qlcdnumber.h>
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
    _ppi->setMinimumSize( QSize( 400, 400 ) );
    CP2PPIBaseLayout->addWidget( _ppi );

    frameColorBar = new QFrame( centralWidget(), "frameColorBar" );
    frameColorBar->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, frameColorBar->sizePolicy().hasHeightForWidth() ) );
    frameColorBar->setMinimumSize( QSize( 40, 400 ) );
    frameColorBar->setMaximumSize( QSize( 40, 32767 ) );
    frameColorBar->setFrameShape( QFrame::StyledPanel );
    frameColorBar->setFrameShadow( QFrame::Raised );
    CP2PPIBaseLayout->addWidget( frameColorBar );

    layout16 = new QVBoxLayout( 0, 0, 6, "layout16"); 

    _typeTab = new QTabWidget( centralWidget(), "_typeTab" );
    _typeTab->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, _typeTab->sizePolicy().hasHeightForWidth() ) );

    tab = new QWidget( _typeTab, "tab" );
    _typeTab->insertTab( tab, QString::fromLatin1("") );
    layout16->addWidget( _typeTab );
    spacer3 = new QSpacerItem( 20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding );
    layout16->addItem( spacer3 );

    layout14 = new QVBoxLayout( 0, 0, 6, "layout14"); 

    _startButton = new QPushButton( centralWidget(), "_startButton" );
    _startButton->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, _startButton->sizePolicy().hasHeightForWidth() ) );
    layout14->addWidget( _startButton );

    layout13 = new QHBoxLayout( 0, 0, 6, "layout13"); 

    _zoomInButton = new QPushButton( centralWidget(), "_zoomInButton" );
    _zoomInButton->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, _zoomInButton->sizePolicy().hasHeightForWidth() ) );
    layout13->addWidget( _zoomInButton );

    _zoomOutButton = new QPushButton( centralWidget(), "_zoomOutButton" );
    _zoomOutButton->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, _zoomOutButton->sizePolicy().hasHeightForWidth() ) );
    layout13->addWidget( _zoomOutButton );
    layout14->addLayout( layout13 );

    layout12 = new QHBoxLayout( 0, 0, 6, "layout12"); 

    textLabel1 = new QLabel( centralWidget(), "textLabel1" );
    textLabel1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, textLabel1->sizePolicy().hasHeightForWidth() ) );
    layout12->addWidget( textLabel1 );

    ZoomFactor = new QLCDNumber( centralWidget(), "ZoomFactor" );
    ZoomFactor->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, ZoomFactor->sizePolicy().hasHeightForWidth() ) );
    ZoomFactor->setSegmentStyle( QLCDNumber::Flat );
    layout12->addWidget( ZoomFactor );
    layout14->addLayout( layout12 );
    layout16->addLayout( layout14 );
    CP2PPIBaseLayout->addLayout( layout16 );

    // toolbars

    languageChange();
    resize( QSize(646, 443).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
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
    _typeTab->changeTab( tab, tr( "Tab 1" ) );
    _startButton->setText( tr( "Start/Stop Display" ) );
    _zoomInButton->setText( tr( "Zoom in" ) );
    _zoomOutButton->setText( tr( "Zoom out" ) );
    textLabel1->setText( tr( "Zoom:" ) );
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

