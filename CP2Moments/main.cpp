#include <qapplication.h>
#include "CP2Moments.h"
#include <stdlib.h>


int main( int argc, char** argv )
{
    QApplication app( argc, argv );
	QDialog* dialog = new QDialog;

	// create our main window. It may contain a PPI sometime, and 
	// other buttons etc.
	CP2Moments cp2moments(dialog);

	// if we don't show() the  dialog, nothing appears!
	dialog->show();
	
	return app.exec();
}

