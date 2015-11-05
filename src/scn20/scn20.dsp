# Microsoft Developer Studio Project File - Name="scn20" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=scn20 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "scn20.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "scn20.mak" CFG="scn20 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "scn20 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "scn20 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "scn20 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W4 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "scn20 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W4 /Gm /GX /ZI /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "scn20 - Win32 Release"
# Name "scn20 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\define.c
# End Source File
# Begin Source File

SOURCE=.\error.c
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

SOURCE=.\global.c
# End Source File
# Begin Source File

SOURCE=.\noise.c
# End Source File
# Begin Source File

SOURCE=.\param.c
# End Source File
# Begin Source File

SOURCE=.\parse.c
# End Source File
# Begin Source File

SOURCE=.\pcontext.c
# End Source File
# Begin Source File

SOURCE=.\pexpr.c
# End Source File
# Begin Source File

SOURCE=.\scn20.c
# End Source File
# Begin Source File

SOURCE=.\shadebg.c
# End Source File
# Begin Source File

SOURCE=.\shadesrf.c
# End Source File
# Begin Source File

SOURCE=.\symtab.c
# End Source File
# Begin Source File

SOURCE=.\vm.c
# End Source File
# Begin Source File

SOURCE=.\vmbkgrnd.c
# End Source File
# Begin Source File

SOURCE=.\vmblob.c
# End Source File
# Begin Source File

SOURCE=.\vmcsg.c
# End Source File
# Begin Source File

SOURCE=.\vmexpr.c
# End Source File
# Begin Source File

SOURCE=.\vmfnxyz.c
# End Source File
# Begin Source File

SOURCE=.\vmfunc.c
# End Source File
# Begin Source File

SOURCE=.\vmlight.c
# End Source File
# Begin Source File

SOURCE=.\vmlval.c
# End Source File
# Begin Source File

SOURCE=.\vmobject.c
# End Source File
# Begin Source File

SOURCE=.\vmpoly.c
# End Source File
# Begin Source File

SOURCE=.\vmproc.c
# End Source File
# Begin Source File

SOURCE=.\vmsrface.c
# End Source File
# Begin Source File

SOURCE=.\vmstack.c
# End Source File
# Begin Source File

SOURCE=.\vmviewpt.c
# End Source File
# Begin Source File

SOURCE=.\vmxform.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\local.h
# End Source File
# Begin Source File

SOURCE=..\Include\scn20.h
# End Source File
# Begin Source File

SOURCE=..\Include\vm.h
# End Source File
# End Group
# End Target
# End Project
