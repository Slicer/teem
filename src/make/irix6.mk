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

#INSTALL = /usr/local/gnu/bin/install

OTHER_CLEAN = so_locations
SHEXT = so
CFLAGS =
CPP_ERROR_DIE = -diag_error 1035
OPT_CFLAG = -O2
STATIC_CFLAG = -Bstatic
SHARED_CFLAG = -Bdynamic
SHARED_LDFLAG = -shared
ifeq ($(SUBARCH),n32)
  ARCH_CFLAG = -n32
  ARCH_LDFLAG = -n32
else
  ifeq ($(SUBARCH),64)
    ARCH_CFLAG = -64
    ARCH_LDFLAG = -64
  else 
    $(error irix6 mode $(SUBARCH) not recognized) 
  endif
endif

TEEM_ENDIAN = 4321
TEEM_QNANHIBIT = 0
TEEM_DIO = 1
