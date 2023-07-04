This was written in 2005:

   This is an initial implementation of a matlab reader and writer
   for NRRD files, using the mex compiler to link against the Teem
   library, written by Steven Haker and Gordon Kindlmann.

   The .c files are compiled by mex according to the commands in
   compilethis.m, except that you have to edit the -I and -L lines
   according to wherever your Teem got built.  Suggestions welcome.

   Once these are compiled, you should be able to (in matlab)

   >> x = nrrdLoad('blah.nrrd');
   >> nrrdSave('out.nrrd', x);

   Work continues on a matlab-idiomatic way of representing all the other
   meta-information that makes a nrrd something more than a matlab array.

   Gordon Kindlmann

Work did not in fact continue, in part because GLK does not use Matlab in
day-to-day work.  Still, for whatever value this code may still have,
these files are still here.  The python bindings in ../python have more.
