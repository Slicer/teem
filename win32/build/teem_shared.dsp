# Microsoft Developer Studio Project File - Name="teem_shared" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=teem_shared - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "teem_shared.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "teem_shared.mak" CFG="teem_shared - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "teem_shared - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "teem_shared - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "teem_shared - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../lib/shared"
# PROP Intermediate_Dir "shared/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /Zi /Od /D "WIN32" /YX /FD /GZ /c
# ADD CPP /nologo /MD /Zi /Od /I "../../include" /I "../include" /I "..\..\src\air" /I "..\..\src\hest" /I "..\..\src\biff" /I "..\..\src\nrrd" /I "..\..\src\ell" /I "..\..\src\unrrdu" /I "..\..\src\dye" /I "..\..\src\moss" /I "..\..\src\gage" /I "..\..\src\bane" /I "..\..\src\limn" /I "..\..\src\hoover" /I "..\..\src\mite" /I "..\..\src\echo" /I "..\..\src\ten" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D TEEM_ENDIAN=1234 /D TEEM_QNANHIBIT=1 /D TEEM_DIO=0 /D TEEM_32BIT=1 /D TEEM_ZLIB=1 /D TEEM_BZIP2=1 /D TEEM_PNG=1 /D TEEM_BIGBITFIELD=1 /D "TEEM_STATIC" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /incremental:no /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 /libpath:"../lib/shared" png.lib bz2.lib z.lib /nologo /dll /incremental:no /debug /machine:I386 /def:"teem.def" /out:"../bin/teem_d.dll" /implib:"../lib/shared/teem_d.lib /pdb:"../lib/shared/teem_d.pdb" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetName=teem_d
SOURCE="$(InputPath)"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "teem_shared - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../lib/shared"
# PROP Intermediate_Dir "shared/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /O2 /D "WIN32" /YX /FD /c
# ADD CPP /nologo /MD /O2 /I "../../include" /I "../include" /I "..\..\src\air" /I "..\..\src\hest" /I "..\..\src\biff" /I "..\..\src\nrrd" /I "..\..\src\ell" /I "..\..\src\unrrdu" /I "..\..\src\dye" /I "..\..\src\moss" /I "..\..\src\gage" /I "..\..\src\bane" /I "..\..\src\limn" /I "..\..\src\hoover" /I "..\..\src\mite" /I "..\..\src\echo" /I "..\..\src\ten" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D TEEM_ENDIAN=1234 /D TEEM_QNANHIBIT=1 /D TEEM_DIO=0 /D TEEM_32BIT=1 /D TEEM_ZLIB=1 /D TEEM_BZIP2=1 /D TEEM_PNG=1 /D TEEM_BIGBITFIELD=1 /D "TEEM_STATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32zlib-1.1.4/msvc6/lib/shared
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /pdb:none /machine:I386
# ADD LINK32 /libpath:"../lib/shared" png.lib bz2.lib z.lib /nologo /dll /pdb:none /machine:I386 /out:"../bin/teem.dll" /implib:"../lib/shared/teem.lib" /def:"teem.def"
# Begin Special Build Tool
TargetName=teem
SOURCE="$(InputPath)"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "teem_shared - Win32 Debug"
# Name "teem_shared - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\unrrdu\1op.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\2op.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\3op.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\754.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\about.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\accessors.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\aniso.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\apply1D.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\arith.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\array.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\arraysNrrd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\axdelete.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\axinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\axinsert.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\axis.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\axmerge.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\axsplit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\biff\biff.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\block.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\bounds.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\cam.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\cc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\ccadj.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\ccfind.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\ccmerge.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\ccmethods.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\ccsettle.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\chan.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\clip.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\cmedian.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\color.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\comment.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\convert.c
# End Source File
# Begin Source File

SOURCE=..\..\src\dye\convertDye.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\convertNrrd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\crop.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\ctx.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ell\cubicEll.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\data.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\defaultsBane.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\defaultsGage.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hest\defaultsHest.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hoover\defaultsHoover.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\defaultsLimn.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mite\defaultsMite.c
# End Source File
# Begin Source File

SOURCE=..\..\src\moss\defaultsMoss.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\defaultsNrrd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\defaultsTen.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\dhisto.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\dice.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\dio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ell\eigen.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\endianAir.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\endianNrrd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\enum.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\enumsEcho.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\enumsNrrd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\enumsTen.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\env.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\epireg.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\fiber.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\fiberMethods.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\filt.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\filter.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\flip.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\flotsam.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\gamma.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ell\genmat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\gkmsFlotsam.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\gkmsHvol.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\gkmsInfo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\gkmsMite.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\gkmsOpac.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\gkmsPvg.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\gkmsScat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\gkmsTxf.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\glyph.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\gzio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\head.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\heq.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\hestLimn.c
# End Source File
# Begin Source File

SOURCE=..\..\src\moss\hestMoss.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\hestNrrd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\histax.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\histo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\histogram.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\hvol.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\imap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\inc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\inset.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\intx.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\io.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\iter.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\jhisto.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\join.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\kernel.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\light.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\lightEcho.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\list.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\lut.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\make.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\map.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ell\mat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\matter.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\measr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\measure.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\methodsBane.c
# End Source File
# Begin Source File

SOURCE=..\..\src\dye\methodsDye.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\methodsEcho.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hest\methodsHest.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hoover\methodsHoover.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\methodsLimn.c
# End Source File
# Begin Source File

SOURCE=..\..\src\moss\methodsMoss.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\methodsNrrd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\minmax.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\miscAir.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ell\miscEll.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\miscGage.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\miscTen.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\model.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\mop.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\obj.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\objmethods.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\pad.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\parseAir.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hest\parseHest.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\parseNrrd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\permute.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\print.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\project.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\pvl.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\qn.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\quantize.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\range.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mite\ray.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hoover\rays.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\read.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\renderEcho.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\renderLimn.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mite\renderMite.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\reorder.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\resample.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\resampleNrrd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\reshape.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\rmap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\moss\sampler.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\sane.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\save.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\scat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\scl.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\sclanswer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\sclfilter.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\sclprint.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\set.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\shape.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\shapes.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\shuffle.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\simple.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\slice.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\splice.c
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\sqd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\st.c
# End Source File
# Begin Source File

SOURCE=..\..\src\air\string.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hoover\stub.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\subset.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\superset.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\swap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tenGage.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendAnhist.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendAnplot.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendAnvol.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendBmat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendEpireg.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendEstim.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendEval.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendEvec.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendEvq.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendExpand.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendFiber.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendFlotsam.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendGlyph.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendMake.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendPoint.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendSatin.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendShrink.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendSim.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tendSten.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tensor.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mite\thread.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\tmfKernel.c
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\transform.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\trex.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\trnsf.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mite\txf.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\unblock.c
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\unquantize.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\update.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hest\usage.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mite\user.c
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\valid.c
# End Source File
# Begin Source File

SOURCE=..\..\src\ell\vecEll.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\vecGage.c
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\vecprint.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\winKernel.c
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\write.c
# End Source File
# Begin Source File

SOURCE=..\..\src\moss\xform.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\air\air.h
# End Source File
# Begin Source File

SOURCE=..\..\src\hest\hest.h
# End Source File
# Begin Source File

SOURCE=..\..\src\hest\privateHest.h
# End Source File
# Begin Source File

SOURCE=..\..\src\biff\biff.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\nrrd.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\nrrdDefines.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\nrrdMacros.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\nrrdEnums.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nrrd\privateNrrd.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ell\ell.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ell\ellMacros.h
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\unrrdu.h
# End Source File
# Begin Source File

SOURCE=..\..\src\unrrdu\privateUnrrdu.h
# End Source File
# Begin Source File

SOURCE=..\..\src\dye\dye.h
# End Source File
# Begin Source File

SOURCE=..\..\src\moss\moss.h
# End Source File
# Begin Source File

SOURCE=..\..\src\moss\privateMoss.h
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\gage.h
# End Source File
# Begin Source File

SOURCE=..\..\src\gage\privateGage.h
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\bane.h
# End Source File
# Begin Source File

SOURCE=..\..\src\bane\privateBane.h
# End Source File
# Begin Source File

SOURCE=..\..\src\limn\limn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\hoover\hoover.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mite\mite.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mite\privateMite.h
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\echo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\echo\privateEcho.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\ten.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tenMacros.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ten\tenPrivate.h
# End Source File
# End Group
# End Target
# End Project
