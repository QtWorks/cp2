/****************************************************************************
** Meta object code from reading C++ file 'CP2PPI.h'
**
** Created: Wed May 30 11:37:51 2007
**      by: The Qt Meta Object Compiler version 59 (Qt 4.2.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "CP2PPI.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CP2PPI.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.2.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_CP2PPI[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
       8,    7,    7,    7, 0x0a,
      30,   22,    7,    7, 0x0a,
      49,   47,    7,    7, 0x0a,
      78,   73,    7,    7, 0x0a,
      94,    7,    7,    7, 0x0a,
     107,    7,    7,    7, 0x0a,
     121,    7,    7,    7, 0x0a,
     151,  144,    7,    7, 0x0a,
     185,    7,    7,    7, 0x0a,
     215,    7,    7,    7, 0x0a,
     251,  245,    7,    7, 0x0a,
     273,  245,    7,    7, 0x0a,
     295,    7,    7,    7, 0x0a,
     311,    7,    7,    7, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CP2PPI[] = {
    "CP2PPI\0\0newDataSlot()\0ppiType\0ppiTypeSlot(int)\0w\0"
    "tabChangeSlot(QWidget*)\0flag\0pauseSlot(bool)\0zoomInSlot()\0"
    "zoomOutSlot()\0colorBarReleasedSlot()\0result\0"
    "colorBarSettingsFinishedSlot(int)\0backColorButtonReleasedSlot()\0"
    "ringColorButtonReleasedSlot()\0state\0ringStateChanged(int)\0"
    "gridStateChanged(int)\0saveImageSlot()\0resetButtonReleasedSlot()\0"
};

const QMetaObject CP2PPI::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_CP2PPI,
      qt_meta_data_CP2PPI, 0 }
};

const QMetaObject *CP2PPI::metaObject() const
{
    return &staticMetaObject;
}

void *CP2PPI::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CP2PPI))
	return static_cast<void*>(const_cast<CP2PPI*>(this));
    if (!strcmp(_clname, "Ui::CP2PPI"))
	return static_cast<Ui::CP2PPI*>(const_cast<CP2PPI*>(this));
    return QDialog::qt_metacast(_clname);
}

int CP2PPI::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: newDataSlot(); break;
        case 1: ppiTypeSlot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: tabChangeSlot((*reinterpret_cast< QWidget*(*)>(_a[1]))); break;
        case 3: pauseSlot((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: zoomInSlot(); break;
        case 5: zoomOutSlot(); break;
        case 6: colorBarReleasedSlot(); break;
        case 7: colorBarSettingsFinishedSlot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: backColorButtonReleasedSlot(); break;
        case 9: ringColorButtonReleasedSlot(); break;
        case 10: ringStateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: gridStateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: saveImageSlot(); break;
        case 13: resetButtonReleasedSlot(); break;
        }
        _id -= 14;
    }
    return _id;
}
