#
# The contents of this file are subject to the University of Utah Public
# License (the "License"); you may not use this file except in
# compliance with the License.
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Source Code is "teem", released March 23, 2001.
#  
# The Original Source Code was developed by the University of Utah.
# Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
# of Utah. All Rights Reserved.
#
#
BIN = unrrdu/unu
PRIV_HEADERS = private.h
BIN_OBJS = unu.o
#
# every unu command source file must define:
# char *xxxName
#      the short string used to invoke the command
# char *xxxInfo
#      approx. one line description of the function
# int xxxMain(int argc, char **argv, char *argv0)
#      the function to call to do the command
# This applies to every .o file below!
#
BIN_OBJS += flip.o slice.o convert.o crop.o
BIN_LIBS = -lnrrd -lbiff -lhest -lair -lm

include ../make/Common.mk
