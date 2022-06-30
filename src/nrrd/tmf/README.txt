These are the files required to incorporate Torsten Moeller's
kernels into nrrd.  The tmFilters_raw.c file is directly from
an email from Torsten.  The file ../tmfKernel.c is created
as follows (in a bash-like shell)

  ./fix1.pl tmFilters_raw.c |
    ./fix2.pl |
    ./fix3.pl > tmp0.c
  echo '/* clang-format off */' |
    cat ../../preamble.c - tmp0.c > tmp1.c
  echo '/* clang-format on */' |
    cat tmp1.c - |
    sed 's/[[:space:]]*$//'>../tmfKernel.c
  rm -f tmp?.c
