# Microsoft Developer Studio Project File - Name="Gem" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Gem - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Gem.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Gem.mak" CFG="Gem - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Gem - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Gem - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Gem - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W4 /GX /O2 /I ".\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D CONFIG_VERSION_MAJOR=1 /D CONFIG_VERSION_MINOR=1 /Yu"gempch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Gem - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W4 /Gm /GX /ZI /Od /I ".\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D CONFIG_VERSION_MAJOR=1 /D CONFIG_VERSION_MINOR=1 /Yu"gempch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /debug /machine:I386

!ENDIF 

# Begin Target

# Name "Gem - Win32 Release"
# Name "Gem - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\AboutDlg.c
# End Source File
# Begin Source File

SOURCE=.\Draw3d.c
# End Source File
# Begin Source File

SOURCE=.\Gem.c
# End Source File
# Begin Source File

SOURCE=.\Gem.rc
# End Source File
# Begin Source File

SOURCE=.\Gemfile.c
# End Source File
# Begin Source File

SOURCE=.\gempch.c
# ADD CPP /Yc"gempch.h"
# End Source File
# Begin Source File

SOURCE=.\Gemray.c
# End Source File
# Begin Source File

SOURCE=.\Gemreg.c
# End Source File
# Begin Source File

SOURCE=.\Gemscene.c
# End Source File
# Begin Source File

SOURCE=.\Gemstat.c
# End Source File
# Begin Source File

SOURCE=.\msgwnd.c
# End Source File
# Begin Source File

SOURCE=.\Outfile.c
# End Source File
# Begin Source File

SOURCE=.\PathsDlg.c
# End Source File
# Begin Source File

SOURCE=.\RendDlg.c
# End Source File
# Begin Source File

SOURCE=.\Rendwnd.c
# End Source File
# Begin Source File

SOURCE=.\winutil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\Include\Expr.h
# End Source File
# Begin Source File

SOURCE=.\include\findfile.h
# End Source File
# Begin Source File

SOURCE=.\Gem.h
# End Source File
# Begin Source File

SOURCE=.\Gempch.h
# End Source File
# Begin Source File

SOURCE=.\include\image.h
# End Source File
# Begin Source File

SOURCE=.\include\math3d.h
# End Source File
# Begin Source File

SOURCE=.\include\nff.h
# End Source File
# Begin Source File

SOURCE=.\include\raytrace.h
# End Source File
# Begin Source File

SOURCE=.\include\rend2dc.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\include\scn10.h
# End Source File
# Begin Source File

SOURCE=.\winutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\gem.ico
# End Source File
# Begin Source File

SOURCE=.\gemsm.ico
# End Source File
# Begin Source File

SOURCE=.\split.cur
# End Source File
# End Group
# Begin Source File

SOURCE=..\DOC\GemBugs.txt
# End Source File
# Begin Source File

SOURCE=..\DOC\GemTODO.txt
# End Source File
# End Target
# End Project
