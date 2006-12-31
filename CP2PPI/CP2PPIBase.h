/****************************************************************************
** Form interface generated from reading ui file 'CP2PPIBase.ui'
**
** Created: Sat Dec 30 23:16:21 2006
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.5   edited Aug 31 12:13 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef CP2PPIBASE_H
#define CP2PPIBASE_H

#include <qvariant.h>
#include <qmainwindow.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QAction;
class QActionGroup;
class QToolBar;
class QPopupMenu;
class PPI;
class QFrame;
class QTabWidget;
class QWidget;
class QPushButton;
class QLabel;
class QLCDNumber;

class CP2PPIBase : public QMainWindow
{
    Q_OBJECT

public:
    CP2PPIBase( QWidget* parent = 0, const char* name = 0, WFlags fl = WType_TopLevel );
    ~CP2PPIBase();

    PPI* _ppi;
    QFrame* frameColorBar;
    QTabWidget* _typeTab;
    QWidget* tab;
    QPushButton* _startButton;
    QPushButton* _zoomInButton;
    QPushButton* _zoomOutButton;
    QLabel* textLabel1;
    QLCDNumber* ZoomFactor;

public slots:
    virtual void startStopDisplaySlot();
    virtual void dataSourceSlot();
    virtual void zoomInSlot();
    virtual void zoomOutSlot();

protected:
    QHBoxLayout* CP2PPIBaseLayout;
    QVBoxLayout* layout16;
    QSpacerItem* spacer3;
    QVBoxLayout* layout14;
    QHBoxLayout* layout13;
    QHBoxLayout* layout12;

protected slots:
    virtual void languageChange();

};

#endif // CP2PPIBASE_H
