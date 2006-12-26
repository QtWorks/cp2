// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently


// #pragma warning( disable: 4311 4312 )

//#using <mscorlib.dll>
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__878ADD49_92B5_4646_84CE_DC569D24070F__INCLUDED_)
#define AFX_STDAFX_H__878ADD49_92B5_4646_84CE_DC569D24070F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#ifndef LITTLE_ENDIAN
	#define LITTLE_ENDIAN
#endif

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include "pcitypes.h"
#include "PciApi.h"
		
// TODO: reference additional headers your program requires here


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__878ADD49_92B5_4646_84CE_DC569D24070F__INCLUDED_)

