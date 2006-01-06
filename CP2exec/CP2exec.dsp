# Microsoft Developer Studio Project File - Name="CP2exec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=CP2exec - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CP2exec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CP2exec.mak" CFG="CP2exec - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CP2exec - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "CP2exec - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CP2exec - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ws2_32.lib win32lib.lib /nologo /subsystem:console /incremental:yes /machine:I386
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "CP2exec - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 PiraqIII_RevA_Driver.lib win32lib.lib ws2_32.lib winmm.lib PlxApi.lib /nologo /subsystem:console /map /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "CP2exec - Win32 Release"
# Name "CP2exec - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CP2exec.cpp
# End Source File
# Begin Source File

SOURCE=.\CP2exec.rc
# End Source File
# Begin Source File

SOURCE=.\get_julian_day.cpp
# End Source File
# Begin Source File

SOURCE=.\Julian.cpp
# End Source File
# Begin Source File

SOURCE=..\PMAC\pmac_lib.cpp
# End Source File
# Begin Source File

SOURCE=..\PMAC\SRC\STATUS_STRINGS.CPP
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\PMAC\SRC\UTILS.CPP
# End Source File
# Begin Source File

SOURCE=..\PMAC\SRC\WDU_LIB.CPP
# End Source File
# Begin Source File

SOURCE=..\PMAC\SRC\WINDRVR_EVENTS.CPP
# End Source File
# Begin Source File

SOURCE=..\PMAC\SRC\WINDRVR_INT_THREAD.CPP
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CP2exec.h
# End Source File
# Begin Source File

SOURCE=..\include\dd_types.h
# End Source File
# Begin Source File

SOURCE=.\get_julian_day.h
# End Source File
# Begin Source File

SOURCE=.\Julian.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\debug\config.dsp
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=..\win32lib\Release\win32lib.lib
# End Source File
# Begin Source File

SOURCE="..\..\..\Program Files\TVicSoft\TVicHW50\MSVC\TVicHW32.lib"
# End Source File
# Begin Source File

SOURCE=..\..\..\Plx\PciSdk\Win32\Api\Release\PlxApi.lib
# End Source File
# Begin Source File

SOURCE=..\PiraqIII_RevD_Driver\Release\PiraqIII_RevD_Driver.lib
# End Source File
# End Target
# End Project
