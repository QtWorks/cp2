/****************************************************************************
** Form interface generated from reading ui file 'CP2PPIBase.ui'
**
** Created: Thu Sep 28 16:09:48 2006
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.6-snapshot-20060428   edited Apr 2 18:47 $)
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
class QGroupBox;
class QPushButton;
class QLabel;
class QLCDNumber;
class QButtonGroup;
class QRadioButton;
class PPI;
class QFrame;

class CP2PPIBase : public QMainWindow
{
    Q_OBJECT

public:
    CP2PPIBase( QWidget* parent = 0, const char* name = 0, WFlags fl = WType_TopLevel );
    ~CP2PPIBase();

    QGroupBox* groupBox1;
    QPushButton* _startButton;
    QPushButton* _zoomOutButton;
    QPushButton* _zoomInButton;
    QLabel* textLabel1;
    QLCDNumber* ZoomFactor;
    QButtonGroup* _dataSourceButtons;
    QRadioButton* _dataSourceRadarButton;
    QRadioButton* _dataSourceTestButton;
    PPI* _ppi;
    QFrame* frameColorBar;
    QButtonGroup* _productButtons;

public slots:
    virtual void startStopDisplaySlot();
    virtual void dataSourceSlot();
    virtual void zoomInSlot();
    virtual void zoomOutSlot();

protected:
    QHBoxLayout* _dataSourceButtonsLayout;
    QHBoxLayout* layout5;

protected slots:
    virtual void languageChange();

};

#endif // CP2PPIBASE_H
