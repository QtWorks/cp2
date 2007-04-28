REM
REM Create a header file named svnInfo.h, which contains defines for svn related info
REM
REM #ifndef SVNINFOINC                 
REM #define SVNINFOINC    
REM #define SVNREVISION "Revision: 1270" 
REM #define SVNLASTCHANGEDDATE "Last Changed Date: 2005-02-24 12:04:09 -0700 (Thu, 24 Feb 2005)" 
REM #endif                                
REM
REM Lines from the svn info command results are extracted, and 
REM assigned to #defines. Other information (e.g. working directory) 
REM are also aded
REM
REM Note the use of a temporary file in order to get
REM the output of the svn info command into a DOS environment variable.
REM
REM Also note the use of the parenthesis construct; this was the only 
REM way that I could find to get DOS to write quites to the outout file
REM
REM set the subersion command
set SVNCMD="c:\Program Files\Subversion\bin\svn"
REM
REM Determine the top level directory
dir .. | find " Directory of " > svnInfo.tmp
set /P topDir= < svnInfo.tmp
REM
REM strip leading text before the path name (Note no blank between % and >)
echo %topDir:*C:=C:%> svnInfo.tmp
set /P topDir= < svnInfo.tmp
REM
REM change backslashes to forward slashes
echo %topDir:\=/%> svnInfo.tmp
set /P topDir= < svnInfo.tmp
REM
%SVNCMD% info .. | find "Revision:" > svnInfo.tmp
set /p svnRevision= < svnInfo.tmp
%SVNCMD% info .. | find "Last Changed Date:" > svnInfo.tmp
set /p svnLastChangedDate= < svnInfo.tmp
%SVNCMD% info .. | find "URL:" > svnInfo.tmp
set /p svnURL= < svnInfo.tmp
REM
echo #ifndef SVNINFOINC                 > svnInfo.h
echo #define SVNINFOINC                >> svnInfo.h
(
echo #define SVNREVISION "%svnRevision%" 
echo #define SVNLASTCHANGEDDATE "%svnLastChangedDate%" 
echo #define SVNURL "%svnURL%"
echo #define SVNWORKDIRSPEC "Working Directory: %topDir%"
echo #define SVNWORKDIR "%topDir%"
) >> svnInfo.h
echo #endif                                >> svnInfo.h
