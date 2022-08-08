/* Biff: */ annotations (new with Teem 1.13)

teem/src/_util/scan-symbols.py -biff (the "biff auto-scan") will scrutinize
Teem source code to look at how it uses biff. This analysis creates parsable
annotations of function definitions to automate whether and how biffGet{Done}
is called in response to an error (in, say, a Python wrapper), as follows.

Even though these annotations were motivated by the needs of Python wrapping
(which is only wrapping things in the public API), GLK decided to also do these
annotations for "private" functions (which are are available for linking in the
library, but are declared in privateLib.h rather than lib.h), and even for some
static functions. The idea is that this is potentially useful information for
further analysis or for human coding, and its better to err on the side of more
info, discretely packaged, when the quality/correctness of the info is high.

Keep in mind that the Biff annotion on a function reflect a simplistic textual
analysis of that function definition: it looks like this function uses biff in
this way.  This is not based on any proper parsing of the code AST, and there
is certainly no way to know (without execution) whether any called functions
used biff. But this seems adequate for Python wrapping error handling.

Here is an example annotation from teem/src/nrrd/subset.c

  int /* Biff: 1 */
  nrrdSlice(Nrrd *nout, const Nrrd *cnin, unsigned int saxi, size_t pos) {

The annotations are a one-line comment, always on the line with the function
return type, which is above the function name (this formatting is enforced by
new use of clang-format).
** NOTE that in Teem code, the space after the function return type (with the
** function name on the next line) is reserved for these kinds of annotations.
** Human-written comments about the return type/qualifers need to be in the
** previous line.

The single-space-separated words in the comment are, in order:

--- Required:
   "Biff:" : the information in this annotation has been manually verified
or "Biff?" : this annotation automatically generated, and needs verification
(If you want to add information about biff usage which doesn't fit within the
format described here, don't start the comment with "/* Biff:" or "/* Biff?")

--- Optional:
"(private)" : this function is private (as described above). Otherwise, from
the qualifiers on the return type of the function (the same line as this
annotation), "static" will mean that the function is static, while not having
"static" (and absent "(private)"), this is declared in lib.h and intended for
external linkage. Such "private" functions probably aren't even in a python
wrapper, but the fact of being private is nice to record once known, since you
can't tell by looking at a function definition where it has been
declared. Despite the tendency, there is no iron rule that private function
names start with a single "_".

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

  static int /* Biff: nope # unlike other parsers, for reasons described below */
  _nrrdReadNrrdParse_number(FILE *file, Nrrd *nrrd, NrrdIoState *nio, int useBiff) {


                  
how are these annotations created?                  
                  
