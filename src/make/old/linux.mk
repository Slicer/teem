CC = /usr/bin/gcc
LD = /usr/bin/ld 
AR = /usr/bin/ar
RM = /bin/rm -f
INSTALL = /usr/bin/install

SHEXT = so
#PIC_MATTERS = true
#PIC_CFLAG = -fpic
#NONPIC_CFLAG = -fno-pic
PIC_MATTERS = false
PIC_CFLAG = 
NONPIC_CFLAG = 
OPT_CFLAG = -O2
STATIC_CFLAG = -Wl,-Bstatic
SHARED_CFLAG = -Wl,-Bdynamic
SHARED_LDFLAG = -shared
ARCH_CFLAG = 
