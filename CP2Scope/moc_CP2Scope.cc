/****************************************************************************
** Meta object code from reading C++ file 'CP2Scope.h'
**
** Created: Wed May 30 05:02:53 2007
**      by: The Qt Meta Object Compiler version 59 (Qt 4.2.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "CP2Scope.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CP2Scope.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.2.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_CP2Scope[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      10,    9,    9,    9, 0x0a,
      25,    9,    9,    9, 0x0a,
      51,   42,    9,    9, 0x0a,
     100,   69,    9,    9, 0x0a,
     156,  154,    9,    9, 0x0a,
     180,    9,    9,    9, 0x0a,
     203,    9,    9,    9, 0x0a,
     212,    9,    9,    9, 0x0a,
     221,    9,    9,    9, 0x0a,
     237,    9,    9,    9, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CP2Scope[] = {
    "CP2Scope\0\0newPulseSlot()\0newProductSlot()\0plotType\0"
    "plotTypeSlot(int)\0pi,plotType,prodType,pulsePlot\0"
    "plotTypeChange(PlotInfo*,PLOTTYPE,PRODUCT_TYPES,bool)\0w\0"
    "tabChangeSlot(QWidget*)\0gainChangeSlot(double)\0upSlot()\0dnSlot()\0"
    "autoScaleSlot()\0saveImageSlot()\0"
};

const QMetaObject CP2Scope::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_CP2Scope,
      qt_meta_data_CP2Scope, 0 }
};

const QMetaObject *CP2Scope::metaObject() const
{
    return &staticMetaObject;
}

void *CP2Scope::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CP2Scope))
	return static_cast<void*>(const_cast<CP2Scope*>(this));
    if (!strcmp(_clname, "Ui::CP2Scope"))
	return static_cast<Ui::CP2Scope*>(const_cast<CP2Scope*>(this));
    return QDialog::qt_metacast(_clname);
}

int CP2Scope::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: newPulseSlot(); break;
        case 1: newProductSlot(); break;
        case 2: plotTypeSlot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: plotTypeChange((*reinterpret_cast< PlotInfo*(*)>(_a[1])),(*reinterpret_cast< PLOTTYPE(*)>(_a[2])),(*reinterpret_cast< PRODUCT_TYPES(*)>(_a[3])),(*reinterpret_cast< bool(*)>(_a[4]))); break;
        case 4: tabChangeSlot((*reinterpret_cast< QWidget*(*)>(_a[1]))); break;
        case 5: gainChangeSlot((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 6: upSlot(); break;
        case 7: dnSlot(); break;
        case 8: autoScaleSlot(); break;
        case 9: saveImageSlot(); break;
        }
        _id -= 10;
    }
    return _id;
}
