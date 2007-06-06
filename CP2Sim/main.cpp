#include <qapplication.h>
#include "CP2Sim.h"
#include <stdlib.h>


int main( int argc, char** argv )
{
    QApplication app( argc, argv );

	QDialog* dialog = new QDialog(0, Qt::WindowMinimizeButtonHint);

	// create our main window. 
	CP2Sim cp2sim(dialog);

	// if we don't show() the  dialog, nothing appears!
	dialog->show();
	
	return app.exec();
}

