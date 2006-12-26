#include "qtdsp.h"

#include <qapplication.h>
#include <qstring.h>
#include <iostream>
#include <qtimer.h>
#include <qcheckbox.h>
#include <qcheckbox.h>

int main( int argc, char ** argv )
{
    QApplication app( argc, argv );

//    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
	// create our test dialog. It may contain a ScopePlot sometime, and 
	// other buttons etc.
	QtDSP dsp;

	// if we don't show() the test dialog, nothing appears!
	dsp.show();

 	// This tells qt to stop running when scopeTestDialog
	// closes.
	app.setMainWidget(&dsp);
	
	return app.exec();
}
