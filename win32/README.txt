=============== 

  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

=============== Building and Using Teem on Windows

On Windows all the teem libraries are built into one monolithic mega
library, available either as a DLL and an import library or as a
static library that users can link against. The teem command-line
utilities are also built together with these libraries and linked
statically, so they do not rely on the teem DLL.

The following steps are recommended for building teem on Windows:

* Go into the "build" subdirectory.

Visual Studio 6.0
-----------------
* Open the Visual Studio 6.0 workspace file "teem.dsw".
* Select the configuration you want to build using the "Build" menu's
  "Set Active Configuration" item. You have two choices: "Release" and "Debug".
* Select all projects by shift-clicking on them in the workspace file-view
  window.
* Right click on the selection and hit "Build" in the pop-up menu.

Visual Studio .NET
------------------
* Open the Visual Studio 6.0 workspace file "teem.dsw".
* Convert all project files to the .NET format.
* Select the configuration you want to build using the "Build" menu's
  "Configuration Manager" item. You have two choices: "Release" and "Debug".
* In the solution explorer window right click on "Solution teem" and hit
  "Build Solution" in the pop-up menu.

The libraries and executables are compiled in the "lib" and "bin"
subdirectories. To install them, you need to copy the DLL and the
executables manually to a directory in the Windows path. In this
case, the import library "lib/shared/teem.lib" usually goes into one
of the Visual Studio library directories, e.g. "path to visual
studio\Vc98\lib".  Likewise, the headers can be placed in one of the
Visual Studio include directories.

To uninstall teem you have to delete the installed files manually :).

Note that the precompiled binaries are linked against the
"multithreaded DLL" version of the Windows C runtime library. This
means that you have to link against the same library in your project
by setting the appropriate compiler flag (/MD), which is unfortunately
not the default setting in Visual Studio. Linking against a different
version will result in "unexpected crashes" of your program.

To use the compression encodings and png support, you'll need to copy
the provided libraries "bin/z.dll", "bin/bz2.dll", and "bin/png.dll"
to somewhere in the Windows path in case you do not have them yet. The import
libraries for these are located in "lib/shared".

When using the static version of teem, located in "lib/static", you
need to define TEEM_STATIC when you compile your source
(-DTEEM_STATIC), otherwise your program will "crash".
