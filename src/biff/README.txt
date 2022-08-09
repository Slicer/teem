/* Biff: */ annotations (new with Teem 1.13)

teem/src/_util/scan-symbols.py -biff (the "biff auto-scan") will scrutinize
Teem source code to look at how it uses biff. This analysis creates parsable
annotations of function definitions to automate whether and how biffGet{Done}
is called in response to an error (in, say, a Python wrapper), as follows.

Even though these annotations were motivated by the needs of Python wrapping
(which is only wrapping things in the public API), GLK decided to also do these
annotations for "private" functions (which are are available for linking in the
library, but are declared in privateLib.h rather than lib.h), and even for
static functions that do use biff. The idea is that this is potentially useful
information for further analysis or for human coding, and its better to err on
the side of more info, discretely packaged, when the quality/correctness of the
info is high.

NOTE that the Biff annotation on a function reflects a simplistic textual
analysis of that function code: it looks like this function uses biff in this
way.  This is not based on any proper parsing of the code AST, so calls to biff
could be hidden behind a #define, and there is certainly no way to know
(without execution) whether any other functions called from this function used
biff. The formatting of the newly adopted clang-format is a big help. In any
case this seems adequate for Python wrapping error handling.

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

Some notes on how GLK creates the annotations, for example for gage:
GLK has his teem source checkout in ~/teem.
From the ~/teem/src/_util directory:

  python3 scan-symbols.py ~/teem -biff 3 gage

why -biff 3: because -biff 1 is just for observing biff usage;
-biff 2 is for doing annotations where none have been done before
and -biff 3 will over-write old comments and wrong annotations.
But nothing is actually over-written, new file are written, eg:

  wrote 2 annotations in miscGage-annote.c
  wrote 5 annotations in kind-annote.c
  wrote 6 annotations in shape-annote.c

Then to process these (in ~/teem/src/gage)

  diff miscGage{-annote,}.c  # to inspect what biff auto-scan wrote
  mv miscGage{-annote,}.c    # to start editing
  # edit miscGage.c, changing Biff? to Biff: when to confirm annotation
  svn diff miscGage.c        # to check what was changed
  svn commit ...
