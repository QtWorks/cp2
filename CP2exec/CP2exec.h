#ifndef CP2EXECH_
#define CP2EXECH_

#include "CP2PIRAQ.h"
#include "CP2ExecBase.h"
#include "CP2ExecThread.h"

class CP2Exec: public CP2ExecBase 
{
	Q_OBJECT
public:
	CP2Exec(CP2ExecThread* pThread);
	virtual ~CP2Exec();

public slots:
	void restartSlot();

protected:
	CP2ExecThread* _pThread;
	// The builtin timer will be used to display statistics.
	void timerEvent(QTimerEvent*);
	int _statsUpdateInterval;

};

#endif
