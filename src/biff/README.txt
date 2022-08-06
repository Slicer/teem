/* Biff: */ annotations (new with Teem 1.13)

teem/src/_util/scan-symbols.py -biff (the "biff auto-scan") will scrutinize
Teem source code to look at how it uses biff. This analysis creates parsable
annotations of function definitions to automate whether and how biffGet{Done}
is called in response to an error (in, say, a Python wrapper), as follows.

Here is an example from teem/src/nrrd/subset.c

  int /* Biff: 1 */
  nrrdSlice(Nrrd *nout, const Nrrd *cnin, unsigned int saxi, size_t pos) {

The annotations are a one-line comment, always on the line with the function
return type, which is above the function name (this is enforced by new
clang-format). The single-space-separated words in the comment are, in order:

--- Required:
   "Biff:" : the information in this annotation has been manually verified
or "Biff?" : this annotation automatically generated, and needs verification
(If you want to add information about biff usage which doesn't fit within the
format described here, don't start the comment with "/* Biff:" or "/* Biff?")

--- Optional:
"(private)" : this function is declared in privateLib.h, not the "public"
lib.h, though still available for linking in the library. Otherwise, from the
qualifiers on the return type of the function (the same line as this
annotation), "static" will mean that the function is static, while not having
"static" (and absent "(private)"), this is declared in lib.h and intended for
external linkage. Such "private" functions probably aren't even in a python
wrapper (since the wrapper would be around the lib.h-declared API), but this
is relevant information that is nice to record once known, since you can't
tell by looking at a function definition where it has been declared. There
is no iron rule that private function names start with a single _

--- Required:
  "<val>" : The return value <val> indicates a biff-reported error, i.e., if
           the function returns <val> then someone eventually needs to
           retrieve that error message. <val> is a string (since it is in a
           comment), but it should be parsable as the function return type
           (on this same line, before the comment).
or "<v1>|<v2>" : Both values <v1> and <v2> indicate a biff-reported error
or "maybe:<N>:<val>" : This function uses something like biffMaybeAddf(), which
           may or may not set a biff error message, depending on the value of
           one of the function parameters (always called "useBiff", as enforced
           by biff auto-scan).  In *1*-based numbering, useBiff is the Nth
           function parameter.
or "nope" : This function does not use biff (no info about error-vs-nonerror
           return values is given by this)

--- Optional:
  # <comments>  : anything after a '#' is ignored by an annotation parser

These annotiations are parsed and consumed by teem/python/cffi/

Other examples:

  int /* Biff: (private) maybe:2:nrrdField_unknown */
  _nrrdReadNrrdParseField(NrrdIoState *nio, int useBiff) {

  static int /* Biff: maybe:4:1 # an oldie but goodie! */      
  _nrrdReadNrrdParse_comment(FILE *file, Nrrd *nrrd, NrrdIoState *nio,
                             int useBiff) {

                  
how are these annotations created?                  
                  
