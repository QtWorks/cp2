#include <qapplication.h>
#include <iostream>

#include "CP2Exec.h"
#include "CP2ExecThread.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
int 
main(int argc, char* argv[], char* envp[])
{
	if (argc != 3) {
		std::cout << "usage: CP2Exec dspObjFile configFile\n";
		exit(1);
	}

	// create the Qt application
	QApplication app( argc, argv );

	// create our main window. It wants to know about the piraq executin
	// thread so that it can query the thread for status information.
	CP2Exec cp2exec(argv[1], argv[2]);

	// if we don't show() the  dialog, nothing appears!
	cp2exec.show();

	app.setMainWidget(&cp2exec);
	
	return app.exec();
}


