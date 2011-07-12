#! /usr/bin/env python

##
##  teem-gen.py: automatically-generated ctypes python wrappers for Teem
##  Copyright (C) 2011 University of Chicago
##  created by Sam Quinan - samquinan@cs.uchicago.edu
##
##  Permission is hereby granted, free of charge, to any person obtaining
##  a copy of this software and associated documentation files (the
##  "Software"), to deal in the Software without restriction, including
##  without limitation the rights to use, copy, modify, merge, publish,
##  distribute, sublicense, and/or sell copies of the Software, and to
##  permit persons to whom the Software is furnished to do so, subject to
##  the following conditions:
##
##  The above copyright notice and this permission notice shall be
##  included in all copies or substantial portions of the Software.
##
##  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
##  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
##  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
##  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
##  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
##  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
##  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
##

import os, sys, shutil, platform, re, string

if len(sys.argv) != 3:
    sys.exit("program expexts arguments: 'ctypeslib-gccxml source dir' 'teem install dir' ")

## (TEEM_LIB_LIST)
libs_list = ["air", "hest", "biff", "nrrd", "ell", "unrrdu", "alan", "moss", "tijk", "gage", "dye", "bane", "limn", "echo", "hoover", "seek", "ten", "elf", "pull", "coil", "push", "mite", "meet"]

#
# validate os 
#
if os.name != "posix":
    sys.exit("program only supports posix compilant systems at this time") 

#
# validate gccxml install
#
tmp = os.path.join(os.getcwd(), "tmp")
os.system("gccxml --version &> tmp")
file = open(tmp, "r")
file.seek(0)
first_line = file.readline()
file.close()
if not first_line.startswith("GCC-XML version"):
    os.remove(tmp)
    sys.exit("error: gccxml of version >= 0.9.0 is required for this script")

if not ((int(first_line[16]) > 0) or ((int(first_line[18]) == 9) and (int(first_line[20]) >= 0))):
    os.remove(tmp)
    sys.exit("error: gccxml of version >= 0.9.0 is required for this script")
os.remove(tmp)

#
# validate ctypeslib-gccxml source dir path 
#
if not os.path.isdir(sys.argv[1]):
    sys.exit("%s does not point to a directory" % sys.argv[1])

CTYPES = os.path.abspath(sys.argv[1])

#
# validate teem install dir path
#
if not os.path.isdir(sys.argv[2]):
    sys.exit("%s does not point to a directory" % sys.argv[2])

TEEM = os.path.abspath(sys.argv[2])


#
# copy files from install dir 
#

TMP_DIR = "teem-gen-tmp"

teem_include = os.path.join(TEEM, "include") 

if not os.path.isdir(teem_include): 
    sys.exit("%s is not the teem install directory; %s is not a valid path" % (teem, teem-install))

if os.path.isdir(TMP_DIR):
    shutil.rmtree(TMP_DIR)

shutil.copytree(teem_include, TMP_DIR)


#
# generate all.h
#

all_h = os.path.join(TMP_DIR, "all.h")
f_open = open (all_h, "w")

defines = []

Files = os.listdir(os.path.join(TMP_DIR, "teem"))
for file in Files:
    if file.endswith(".h"):
        f_open.write("#include <teem/%s> %s" % (file, os.linesep))
        #
        # get #define statements from file
        #
        lines = open (os.path.join(TMP_DIR, "teem", file), "r").readlines()
        expr1 = re.compile("^#define")
        expr2 = re.compile("\\\\")
        expr3 = re.compile("HAS_BEEN_INCLUDED")
        expr4 = re.compile("##")
        expr5 = re.compile("^#define [^ ]*\(")
        expr6 = re.compile("NRRD_TYPE_BIGGEST")
        for line in lines:
            if (expr1.search(line) and not expr2.search(line) and not expr3.search(line) and not expr4.search(line) and not expr5.search(line) and not expr6.search(line)):
                firstword, restwords = string.replace(string.replace(line[8:], "/*", "#" ), "*/", "").split(None,1)
                defines.append("%s = %s" % (firstword, restwords))
f_open.close()

#
# generate teem.xml 
#
# note: these calls are unix only because windows support for command line 
#	append to python path (where the scopeis only that one call) is too
#	difficult at this time -- better support may be added to later versions
#	of python, so that something to watch for / modify in the future

pypath_append = "PYTHONPATH=%s:%s" % (CTYPES, os.getcwd())
teem_xml = os.path.join(os.getcwd(), "teem.xml")
os.system("%s python %s %s -I %s -o %s" % (pypath_append, os.path.join(CTYPES, "scripts", "h2xml.py"), os.path.abspath(all_h), TMP_DIR, teem_xml))


#
# generate pre-teem.py 
#
pre_teem_py = os.path.join(os.getcwd(), "pre-teem.py")

teem_libs = '|'.join(libs_list)

system_type = platform.system()
dll_path = ""
ext = ""
substr = ""
if system_type == "Darwin":
    dll_path = "DYLD_LIBRARY_PATH=%s" % os.path.join(TEEM, "lib")
    ext = "dylib"
    substr = "_libraries['%s']" % os.path.join(TEEM, "lib", "libteem.dylib")
else:
    dll_path = "LD_LIBRARY_PATH=%s" % os.path.join(TEEM, "lib")
    ext = "so"
    substr = "_libraries['libteem.so']"

os.system("%s %s python %s %s -l libteem.%s -o %s -m stdio -r \"(%s).*\"" % (dll_path, pypath_append, os.path.join(CTYPES, "scripts", "xml2py.py"), teem_xml, ext,  pre_teem_py, teem_libs))

#
# generate teem.py 
#

libs_destuctable = list(libs_list)

contents = open(pre_teem_py, "r").readlines()[8:]
mod_contents = []
for line in contents:
    l = line.replace(substr, "libteem")
    mod_contents.append(l)
    if "Present" in l:
        for lib in libs_destuctable:
            lib_str = lib+"Present"
            if lib_str in l:
                libs_destuctable.remove(lib)
                break

# in experimental libs not included, cleanup and fail
if libs_destuctable: # empty sequence implicity false
    shutil.rmtree(TMP_DIR)
    os.remove(teem_xml)
    #os.remove(pre_teem_py)
    sys.exit("ERROR: experimental libs: %s not turned on - please rebuild teem with BUILD_EXPERIMENTAL_LIBS turned on, then re-run teem-gen.py" % ','.join(libs_destuctable))

header = [
"##",
"##  teem.py: automatically-generated ctypes python wrappers for Teem",
"##  Copyright (C) 2011, 2010, 2009  University of Chicago",
"##",
"##  Permission is hereby granted, free of charge, to any person obtaining",
"##  a copy of this software and associated documentation files (the",
"##  \"Software\"), to deal in the Software without restriction, including",
"##  without limitation the rights to use, copy, modify, merge, publish,",
"##  distribute, sublicense, and/or sell copies of the Software, and to",
"##  permit persons to whom the Software is furnished to do so, subject to",
"##  the following conditions:",
"##",
"##  The above copyright notice and this permission notice shall be",
"##  included in all copies or substantial portions of the Software.",
"##",
"##  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,",
"##  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF",
"##  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND",
"##  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE",
"##  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION",
"##  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION",
"##  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.",
"##",
"",
"##",
"## NOTE: The contents of this file were generated by the ctypeslib utility:",
"##   http://svn.python.org/projects/ctypes/branches/ctypeslib-gccxml-0.9",
"## followed by some tweaking",
"##",
"",
"from ctypes import *",
"import ctypes.util",
"",
"## This may not work, so you may need to put the shared library",
"## in the current working directory and use simply:",
"# libteem = CDLL('./libteem.so')",
"libteem = CDLL(ctypes.util.find_library('libteem'))",
"",
"# see if this worked",
"if not libteem._name:",
"    print \"**\"",
"    print \"**  teem.py couldn\'t find and load the \\\"libteem\\\" shared library.\"",
"    print \"**\"",
"    print \"**  On Linux, try putting libteem.so in the current directory \"",
"    print \"**  and edit teem.py to replace: \"",
"    print \"**     libteem = CDLL(ctypes.util.find_library(\'libteem\')\"",
"    print \"**  with:\"",
"    print \"**     libteem = CDLL(\'./libteem.so\')\"",
"    print \"**\"",
"    raise ImportError",
"",
"STRING = c_char_p",
"",
"class FILE(Structure):",
"    pass",
"",
"# oddly, size_t is in ctypes, but not ptrdiff_t",
"# which is probably a bug",
"if sizeof(c_void_p) == 4:",
"    ptrdiff_t = c_int32",
"elif sizeof(c_void_p) == 8:",
"    ptrdiff_t = c_int64",
"",
"# NOTE: see end of this file for definitions of stderr, stdin, stdout",
]

footer = [
"# its nice to have these FILE*s around, and this is exactly the",
"# reason for these air functions",
"stderr = airStderr()",
"stdout = airStdout()",
"stdin = airStdin()",
]

teem_py = os.path.join(os.getcwd(), "teem.py")

if os.path.exists(teem_py):
    os.remove(teem_py)

out = open(teem_py, "w")

for line in header:
    out.write(line)
    out.write(os.linesep)

out.writelines(mod_contents)
out.write(os.linesep)

out.write("# define statements")
out.write(os.linesep)
out.writelines(defines)
out.write(os.linesep)

for line in footer:
    out.write(line)
    out.write(os.linesep)

out.close()


#
# cleanup
#

shutil.rmtree(TMP_DIR)
os.remove(teem_xml)
os.remove(pre_teem_py)
