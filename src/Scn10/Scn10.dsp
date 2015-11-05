# Microsoft Developer Studio Project File - Name="Scn10" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Scn10 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Scn10.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Scn10.mak" CFG="Scn10 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Scn10 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Scn10 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Scn10 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir "."
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W4 /GX /O2 /I "..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D CONFIG_VERSION_MAJOR=1 /D CONFIG_VERSION_MINOR=1 /YX"local.h" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Scn10 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir "."
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W4 /GX /Z7 /Od /I "..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D CONFIG_VERSION_MAJOR=1 /D CONFIG_VERSION_MINOR=1 /YX"local.h" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "Scn10 - Win32 Release"
# Name "Scn10 - Win32 Debug"
# Begin Source File

SOURCE=.\backgrnd.c
# End Source File
# Begin Source File

SOURCE=.\Blob.c
# End Source File
# Begin Source File

SOURCE=.\box.c
# End Source File
# Begin Source File

SOURCE=.\branch.c
# End Source File
# Begin Source File

SOURCE=.\colormap.c
# End Source File
# Begin Source File

SOURCE=.\cone.c
# End Source File
# Begin Source File

SOURCE=.\csg.c
# End Source File
# Begin Source File

SOURCE=.\define.c
# End Source File
# Begin Source File

SOURCE=.\disc.c
# End Source File
# Begin Source File

SOURCE=.\error.c
# End Source File
# Begin Source File

SOURCE=.\exeval.c
# End Source File
# Begin Source File

SOURCE=.\exparse.c
# End Source File
# Begin Source File

SOURCE=.\file.c
# End Source File
# Begin Source File

SOURCE=.\Findfile.c
# End Source File
# Begin Source File

SOURCE=.\gettoken.c
# End Source File
# Begin Source File

SOURCE=.\heightfield.c
# End Source File
# Begin Source File

SOURCE=.\implicit.c
# End Source File
# Begin Source File

SOURCE=.\light.c
# End Source File
# Begin Source File

SOURCE=.\local.h
# End Source File
# Begin Source File

SOURCE=.\loop.c
# End Source File
# Begin Source File

SOURCE=.\lvalue.c
# End Source File
# Begin Source File

SOURCE=.\message.c
# End Source File
# Begin Source File

SOURCE=.\misc.c
# End Source File
# Begin Source File

SOURCE=.\noise.c
# End Source File
# Begin Source File

SOURCE=.\Nurbs.c
# End Source File
# Begin Source File

SOURCE=.\object.c
# End Source File
# Begin Source File

SOURCE=.\param.c
# End Source File
# Begin Source File

SOURCE=.\parse.c
# End Source File
# Begin Source File

SOURCE=.\polygon.c
# End Source File
# Begin Source File

SOURCE=.\polymesh.c
# End Source File
# Begin Source File

SOURCE=.\scn10.c
# End Source File
# Begin Source File

SOURCE=.\scnbuild.c
# End Source File
# Begin Source File

SOURCE=.\sphere.c
# End Source File
# Begin Source File

SOURCE=.\stmt.c
# End Source File
# Begin Source File

SOURCE=.\surface.c
# End Source File
# Begin Source File

SOURCE=.\symbol.c
# End Source File
# Begin Source File

SOURCE=.\symtab.c
# End Source File
# Begin Source File

SOURCE=.\torus.c
# End Source File
# Begin Source File

SOURCE=.\triangle.c
# End Source File
# Begin Source File

SOURCE=.\viewport.c
# End Source File
# End Target
# End Project
