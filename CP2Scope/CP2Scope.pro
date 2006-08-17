TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

FORMS	= .\CP2ScopeBase.ui

IMAGES	= images/editcopy \
	images/editcut \
	images/editpaste \
	images/filenew \
	images/fileopen \
	images/filesave \
	images/print \
	images/redo \
	images/searchfind \
	images/undo

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}


