This directory (formerly called "arch") used to contain many per-architecture
sub-directories:
   cygwin darwin.32 darwin.64 irix6.64 irix6.n32 linux.32 linux.amd64
   linux.ia64 netbsd.32 netbsd.amd64 netbsd.ia64 solaris win32
that were used as destinations for Teem's (non-CMake) GNUmake build system,
which was designed for settings where a wide variety of architectures are
simultaneously using the same networked filesystem.

For TeemV2, this was greatly simplified. Now the GNUmake build system puts
files right into this directory (creating obj/ bin/ include/ lib/ directories
as needed).  This is still used for Teem development and debugging.
