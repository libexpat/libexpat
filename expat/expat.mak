# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=xmlwf - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to xmlwf - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "xmltok - Win32 Release" && "$(CFG)" != "xmltok - Win32 Debug"\
 && "$(CFG)" != "xmlec - Win32 Release" && "$(CFG)" != "xmlec - Win32 Debug" &&\
 "$(CFG)" != "xmlwf - Win32 Release" && "$(CFG)" != "xmlwf - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "xmltok.mak" CFG="xmlwf - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xmltok - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "xmltok - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "xmlec - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "xmlec - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "xmlwf - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "xmlwf - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "xmlwf - Win32 Debug"

!IF  "$(CFG)" == "xmltok - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : ".\bin\xmltok.dll"

CLEAN : 
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(INTDIR)\xmlrole.obj"
	-@erase "$(INTDIR)\xmltok.obj"
	-@erase "$(OUTDIR)\xmltok.exp"
	-@erase "$(OUTDIR)\xmltok.lib"
	-@erase ".\bin\xmltok.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/xmltok.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xmltok.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /out:"bin/xmltok.dll"
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=/nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/xmltok.pdb" /machine:I386 /out:"bin/xmltok.dll"\
 /implib:"$(OUTDIR)/xmltok.lib" 
LINK32_OBJS= \
	"$(INTDIR)\dllmain.obj" \
	"$(INTDIR)\xmlrole.obj" \
	"$(INTDIR)\xmltok.obj"

".\bin\xmltok.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xmltok - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\xmltok.dll"

CLEAN : 
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\xmlrole.obj"
	-@erase "$(INTDIR)\xmltok.obj"
	-@erase "$(OUTDIR)\xmltok.dll"
	-@erase "$(OUTDIR)\xmltok.exp"
	-@erase "$(OUTDIR)\xmltok.ilk"
	-@erase "$(OUTDIR)\xmltok.lib"
	-@erase "$(OUTDIR)\xmltok.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/xmltok.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xmltok.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/xmltok.pdb" /debug /machine:I386 /out:"$(OUTDIR)/xmltok.dll"\
 /implib:"$(OUTDIR)/xmltok.lib" 
LINK32_OBJS= \
	"$(INTDIR)\dllmain.obj" \
	"$(INTDIR)\xmlrole.obj" \
	"$(INTDIR)\xmltok.obj"

"$(OUTDIR)\xmltok.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xmlec - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "xmlec\Release"
# PROP BASE Intermediate_Dir "xmlec\Release"
# PROP BASE Target_Dir "xmlec"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "xmlec\Release"
# PROP Intermediate_Dir "xmlec\Release"
# PROP Target_Dir "xmlec"
OUTDIR=.\xmlec\Release
INTDIR=.\xmlec\Release

ALL : "xmltok - Win32 Release" ".\bin\xmlec.exe"

CLEAN : 
	-@erase "$(INTDIR)\xmlec.obj"
	-@erase ".\bin\xmlec.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /Ob2 /I "." /D "NDEBUG" /D "WIN32" /D\
 "_CONSOLE" /Fp"$(INTDIR)/xmlec.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\xmlec\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xmlec.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 setargv.obj kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"bin/xmlec.exe"
LINK32_FLAGS=setargv.obj kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/xmlec.pdb" /machine:I386 /out:"bin/xmlec.exe" 
LINK32_OBJS= \
	"$(INTDIR)\xmlec.obj" \
	".\Release\xmltok.lib"

".\bin\xmlec.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xmlec - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "xmlec\Debug"
# PROP BASE Intermediate_Dir "xmlec\Debug"
# PROP BASE Target_Dir "xmlec"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "xmlec\Debug"
# PROP Intermediate_Dir "xmlec\Debug"
# PROP Target_Dir "xmlec"
OUTDIR=.\xmlec\Debug
INTDIR=.\xmlec\Debug

ALL : "xmltok - Win32 Debug" ".\Debug\xmlec.exe"

CLEAN : 
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\xmlec.obj"
	-@erase "$(OUTDIR)\xmlec.pdb"
	-@erase ".\Debug\xmlec.exe"
	-@erase ".\Debug\xmlec.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "." /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "." /D "_DEBUG" /D "WIN32" /D\
 "_CONSOLE" /Fp"$(INTDIR)/xmlec.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\xmlec\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xmlec.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 setargv.obj kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/xmlec.exe"
LINK32_FLAGS=setargv.obj kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/xmlec.pdb" /debug /machine:I386 /out:"Debug/xmlec.exe" 
LINK32_OBJS= \
	"$(INTDIR)\xmlec.obj" \
	".\Debug\xmltok.lib"

".\Debug\xmlec.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xmlwf - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "xmlwf\Release"
# PROP BASE Intermediate_Dir "xmlwf\Release"
# PROP BASE Target_Dir "xmlwf"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "xmlwf\Release"
# PROP Intermediate_Dir "xmlwf\Release"
# PROP Target_Dir "xmlwf"
OUTDIR=.\xmlwf\Release
INTDIR=.\xmlwf\Release

ALL : "xmltok - Win32 Release" ".\bin\xmlwf.exe"

CLEAN : 
	-@erase "$(INTDIR)\wfcheck.obj"
	-@erase "$(INTDIR)\win32filemap.obj"
	-@erase "$(INTDIR)\xmlwf.obj"
	-@erase ".\bin\xmlwf.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "." /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/xmlwf.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\xmlwf\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xmlwf.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 setargv.obj kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"bin/xmlwf.exe"
LINK32_FLAGS=setargv.obj kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/xmlwf.pdb" /machine:I386 /out:"bin/xmlwf.exe" 
LINK32_OBJS= \
	"$(INTDIR)\wfcheck.obj" \
	"$(INTDIR)\win32filemap.obj" \
	"$(INTDIR)\xmlwf.obj" \
	".\Release\xmltok.lib"

".\bin\xmlwf.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xmlwf - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "xmlwf\Debug"
# PROP BASE Intermediate_Dir "xmlwf\Debug"
# PROP BASE Target_Dir "xmlwf"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "xmlwf\Debug"
# PROP Intermediate_Dir "xmlwf\Debug"
# PROP Target_Dir "xmlwf"
OUTDIR=.\xmlwf\Debug
INTDIR=.\xmlwf\Debug

ALL : "xmltok - Win32 Debug" ".\Debug\xmlwf.exe"

CLEAN : 
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\wfcheck.obj"
	-@erase "$(INTDIR)\win32filemap.obj"
	-@erase "$(INTDIR)\xmlwf.obj"
	-@erase "$(OUTDIR)\xmlwf.pdb"
	-@erase ".\Debug\xmlwf.exe"
	-@erase ".\Debug\xmlwf.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "." /D "WIN32" /D "_DEBUG" /D\
 "_CONSOLE" /Fp"$(INTDIR)/xmlwf.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\xmlwf\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xmlwf.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 setargv.obj kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/xmlwf.exe"
LINK32_FLAGS=setargv.obj kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/xmlwf.pdb" /debug /machine:I386 /out:"Debug/xmlwf.exe" 
LINK32_OBJS= \
	"$(INTDIR)\wfcheck.obj" \
	"$(INTDIR)\win32filemap.obj" \
	"$(INTDIR)\xmlwf.obj" \
	".\Debug\xmltok.lib"

".\Debug\xmlwf.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "xmltok - Win32 Release"
# Name "xmltok - Win32 Debug"

!IF  "$(CFG)" == "xmltok - Win32 Release"

!ELSEIF  "$(CFG)" == "xmltok - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\xmltok.c

!IF  "$(CFG)" == "xmltok - Win32 Release"

DEP_CPP_XMLTO=\
	".\asciitab.h"\
	".\latin1tab.h"\
	".\nametab.h"\
	".\utf8tab.h"\
	".\xmltok.h"\
	".\xmltok_impl.c"\
	".\xmltok_impl.h"\
	
# ADD CPP /Ob2

"$(INTDIR)\xmltok.obj" : $(SOURCE) $(DEP_CPP_XMLTO) "$(INTDIR)"
   $(CPP) /nologo /MT /W3 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/xmltok.pch" /YX /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "xmltok - Win32 Debug"

DEP_CPP_XMLTO=\
	".\asciitab.h"\
	".\latin1tab.h"\
	".\nametab.h"\
	".\utf8tab.h"\
	".\xmltok.h"\
	".\xmltok_impl.c"\
	".\xmltok_impl.h"\
	

"$(INTDIR)\xmltok.obj" : $(SOURCE) $(DEP_CPP_XMLTO) "$(INTDIR)"
   $(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/xmltok.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dllmain.c

!IF  "$(CFG)" == "xmltok - Win32 Release"


"$(INTDIR)\dllmain.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "xmltok - Win32 Debug"


"$(INTDIR)\dllmain.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\xmlrole.c

!IF  "$(CFG)" == "xmltok - Win32 Release"

DEP_CPP_XMLRO=\
	".\xmlrole.h"\
	".\xmltok.h"\
	

"$(INTDIR)\xmlrole.obj" : $(SOURCE) $(DEP_CPP_XMLRO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "xmltok - Win32 Debug"

DEP_CPP_XMLRO=\
	".\xmlrole.h"\
	".\xmltok.h"\
	

"$(INTDIR)\xmlrole.obj" : $(SOURCE) $(DEP_CPP_XMLRO) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
################################################################################
# Begin Target

# Name "xmlec - Win32 Release"
# Name "xmlec - Win32 Debug"

!IF  "$(CFG)" == "xmlec - Win32 Release"

!ELSEIF  "$(CFG)" == "xmlec - Win32 Debug"

!ENDIF 

################################################################################
# Begin Project Dependency

# Project_Dep_Name "xmltok"

!IF  "$(CFG)" == "xmlec - Win32 Release"

"xmltok - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\xmltok.mak" CFG="xmltok - Win32 Release" 

!ELSEIF  "$(CFG)" == "xmlec - Win32 Debug"

"xmltok - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\xmltok.mak" CFG="xmltok - Win32 Debug" 

!ENDIF 

# End Project Dependency
################################################################################
# Begin Source File

SOURCE=.\xmlec\xmlec.c
DEP_CPP_XMLEC=\
	".\xmltok.h"\
	

"$(INTDIR)\xmlec.obj" : $(SOURCE) $(DEP_CPP_XMLEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
################################################################################
# Begin Target

# Name "xmlwf - Win32 Release"
# Name "xmlwf - Win32 Debug"

!IF  "$(CFG)" == "xmlwf - Win32 Release"

!ELSEIF  "$(CFG)" == "xmlwf - Win32 Debug"

!ENDIF 

################################################################################
# Begin Project Dependency

# Project_Dep_Name "xmltok"

!IF  "$(CFG)" == "xmlwf - Win32 Release"

"xmltok - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\xmltok.mak" CFG="xmltok - Win32 Release" 

!ELSEIF  "$(CFG)" == "xmlwf - Win32 Debug"

"xmltok - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\xmltok.mak" CFG="xmltok - Win32 Debug" 

!ENDIF 

# End Project Dependency
################################################################################
# Begin Source File

SOURCE=.\xmlwf\wfcheck.c
DEP_CPP_WFCHE=\
	".\xmlrole.h"\
	".\xmltok.h"\
	".\xmlwf\wfcheck.h"\
	

"$(INTDIR)\wfcheck.obj" : $(SOURCE) $(DEP_CPP_WFCHE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\xmlwf\xmlwf.c
DEP_CPP_XMLWF=\
	".\xmlwf\filemap.h"\
	".\xmlwf\wfcheck.h"\
	

"$(INTDIR)\xmlwf.obj" : $(SOURCE) $(DEP_CPP_XMLWF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\xmlwf\win32filemap.c
DEP_CPP_WIN32=\
	".\xmlwf\filemap.h"\
	

"$(INTDIR)\win32filemap.obj" : $(SOURCE) $(DEP_CPP_WIN32) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
