#include <qapplication.h>
#include "CP2PPI.h"
#include <stdlib.h>


int main( int argc, char** argv )
{
    QApplication app( argc, argv );

	// create our main window. It may contain a PPI sometime, and 
	// other buttons etc.
	CP2PPI cp2ppi;

	// if we don't show() the  dialog, nothing appears!
	cp2ppi.show();

 	// This tells cp2ppi to stop running when the main window
	// closes.
	app.setMainWidget(&cp2ppi);
	
	return app.exec();
}

