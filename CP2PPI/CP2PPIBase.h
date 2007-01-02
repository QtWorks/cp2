/****************************************************************************
** Form interface generated from reading ui file 'CP2PPIBase.ui'
**
** Created: Tue Jan 2 10:17:52 2007
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
class QPushButton;
class QLabel;
class QLCDNumber;
class QTabWidget;
class QWidget;
class QGroupBox;

class CP2PPIBase : public QMainWindow
{
    Q_OBJECT

public:
    CP2PPIBase( QWidget* parent = 0, const char* name = 0, WFlags fl = WType_TopLevel );
    ~CP2PPIBase();

    PPI* _ppi;
    QFrame* frameColorBar;
    QPushButton* _pauseButton;
    QPushButton* _zoomInButton;
    QPushButton* _zoomOutButton;
    QLabel* textLabel1;
    QLCDNumber* ZoomFactor;
    QTabWidget* _typeTab;
    QWidget* tab;
    QGroupBox* groupBox1;
    QLabel* textLabel1_2;
    QLabel* textLabel3_2;
    QLabel* _chan0pulseRate;
    QLabel* _chan1pulseRate;
    QLabel* _chan2pulseRate;
    QLabel* textLabel3_3;
    QLabel* _chan0errors;
    QLabel* _chan1errors;
    QLabel* _chan2errors;
    QLabel* textLabel3;
    QLabel* _chan0pulseCount;
    QLabel* _chan1pulseCount;
    QLabel* _chan2pulseCount;
    QLabel* textLabel1_5;
    QLabel* textLabel1_5_2;
    QLabel* textLabel1_5_3;
    QLabel* textLabel4;
    QLabel* _chan0led;
    QLabel* _chan2led;
    QLabel* _chan1led;
    QLabel* m_pTextIPname;
    QLabel* m_pTextIPaddress;

public slots:
    virtual void pauseSlot(bool);
    virtual void dataSourceSlot();
    virtual void zoomInSlot();
    virtual void zoomOutSlot();

protected:
    QHBoxLayout* CP2PPIBaseLayout;
    QVBoxLayout* layout18;
    QSpacerItem* spacer3;
    QVBoxLayout* layout14;
    QHBoxLayout* layout13;
    QHBoxLayout* layout12;
    QGridLayout* groupBox1Layout;
    QHBoxLayout* layout29;
    QSpacerItem* spacer2;
    QHBoxLayout* layout40;

protected slots:
    virtual void languageChange();

};

#endif // CP2PPIBASE_H
