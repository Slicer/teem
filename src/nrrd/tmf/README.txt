This are the files required to incorporate Torsten Moeller's
kernels into nrrd.  Actually, only tmFilters_raw.c is needed,
and is used to create ../tmfKernel.c as follows:

  tar xzvf gk.tgz
  ./fix1.pl tmFilters_raw.c \
    | ./fix2.pl \
    | cat ../../preamble.c - \
    > ../tmfKernel.c
  rm -f *.c *.cpp *.h

The all the C++ source and header files came from email from
Torsten Moeller, the perl scripts were by Gordon.
