/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

/* learned: some header file must declare private functions
** within extern "C" so that others can link with them
*/

#define NO_STRING "."

#define INFO "Create a nrrd (or nrrd header) from scratch"
static const char *_unrrdu_makeInfoL
  = (INFO ".  The data can be in one or more files, or coming from stdin. "
          "This provides an easy way of specifying the information about some "
          "data as to wrap it in a NRRD file, either to pass on for further "
          "unu processing, or to save to disk.  Note that with \"-h\", this creates "
          "a detached nrrd header file, without ever reading or writing data files. "
          "\n \n "
          "When using multiple datafiles, the data from each is simply "
          "concatenated in memory (as opposed to interleaving along a faster axis). "
          "Keep in mind that all the options below refer to the finished data segment "
          "resulting from joining all the data pieces together, "
          "except for \"-ls\", \"-bs\", and \"-e\", which apply (uniformly) to the "
          "individual data files. Use the \"-fd\" option when the things being joined "
          "together are not slices of the final result, but slabs or scanlines. "
          "It may be easier to put multiple filenames in a response file; "
          "there can be one or more filenames per line of the response file. "
          "You can also use a sprintf-style format to identify a numbered "
          "range of files, so for example \"-i I.%03d 1 90 1\" "
          "refers to I.001, I.002, ... I.090, using the inclusive range from the first "
          "to the second integer (following the sprintf-style format), in steps of "
          "the third.  Can optionally give a fourth integer to serve same role as "
          "\"-fd\"."
          "\n \n "
          "NOTE: for the \"-l\" (labels), \"-u\" (units), and \"-spu\" (space units) "
          "options below, you can use a single unquoted period (.) to signify "
          "an empty string.  This creates a convenient way to convey something that "
          "the shell doesn't make it easy to convey.  Shell expansion weirdness "
          "also requires the use of quotes around the arguments to \"-orig\" (space "
          "origin), \"-dirs\" (space directions), and \"-mf\" (measurement frame).\n "
          "\n "
          "* Uses various components of file and data IO, but currently there is no "
          "library function that encapsulates the functionality here.");

static int
unrrdu_makeMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *outData, *err, **dataFileNames, **kvp, *content,
    encInfo[AIR_STRLEN_LARGE + 1];
  Nrrd *nrrd;
  size_t *size, bufLen;
  int headerOnly, pret, endian, type, encodingType, gotSpacing, gotThickness, gotMin,
    gotMax, space, spaceSet;
  long int byteSkip; /* NOT unsigned: -1 means "from the end" */
  unsigned int ii, kindsLen, thicknessLen, spacingLen, sizeLen, nameLen, centeringsLen,
    unitsLen, labelLen, kvpLen, spunitsLen, dataFileDim, spaceDim, minLen, maxLen,
    thicknessIdx, spacingIdx, minIdx, maxIdx, lineSkip;
  double *spacing, *axmin, *axmax, *thickness;
  airArray *mop;
  NrrdIoState *nio;
  FILE *fileOut;
  char **label, **units, **spunits, **kinds, **centerings, *parseBuf, *spcStr, *_origStr,
    *origStr, *_dirStr, *dirStr, *_mframeStr, *mframeStr;
  const NrrdEncoding *encoding;

  /* so that long lists of filenames can be read from file */
  hparm->respFileEnable = AIR_TRUE;
  hparm->greedySingleString = AIR_TRUE;

  mop = airMopNew();

  hestOptAdd_Flag(&opt, "h", &headerOnly,
                  "Generate header ONLY: don't write out the whole nrrd, "
                  "don't even bother reading the input data, just output the "
                  "detached nrrd header file (usually with a \".nhdr\" "
                  "extension) determined by the options below. The single "
                  "constraint is that detached headers are incompatible with "
                  "using stdin as the data source.");
  hestOptAdd_Nv_String(&opt, "i,input", "file", 1, -1, &dataFileNames, "-",
                       "Filename(s) of data file(s); use \"-\" for stdin. *OR*, can "
                       "use sprintf-style format for identifying a range of numbered "
                       "files, see above for details.",
                       &nameLen);
  hestOptAdd_1_Enum(&opt, "t,type", "type", &type, NULL,
                    "type of data (e.g. \"uchar\", \"int\", \"float\", "
                    "\"double\", etc.)",
                    nrrdType);
  hestOptAdd_Nv_Size_t(&opt, "s,size", "sz0 sz1", 1, -1, &size, NULL,
                       "number of samples along each axis (and implicit indicator "
                       "of dimension of nrrd)",
                       &sizeLen);
  hestOptAdd_1_UInt(&opt, "fd,filedim", "dim", &dataFileDim, "0",
                    "When using *multiple* input data files (to \"-i\"), what is "
                    "the dimension of the array data in each individual file. By "
                    "default (not using this option), this dimension is assumed "
                    "to be one less than the whole data dimension. ");
  spacingIdx = hestOptAdd_Nv_Double(
    &opt, "sp,spacing", "sp0 sp1", 1, -1, &spacing, "nan",
    "spacing between samples on each axis.  Use \"nan\" for "
    "any non-spatial axes (e.g. spacing between red, green, and blue "
    "along axis 0 of interleaved RGB image data)",
    &spacingLen);
  /* NB: these are like unu jhisto's -min -max, not like unu crop's */
  minIdx = hestOptAdd_Nv_Double(
    &opt, "min,axismin", "min0 min1", 1, -1, &axmin, "nan",
    "When each axis has a distinct meaning (as in a joint "
    "histogram), the per-axis min is the smallest \"position\" "
    "associated with the first sample on the axis. Use \"nan\" for "
    "\"no value to set\" when other axes do have axis min",
    &minLen);
  maxIdx = hestOptAdd_Nv_Double(
    &opt, "max,axismax", "max0 max1", 1, -1, &axmax, "nan",
    "Goes with -min: the per-axis maximum \"position\". "
    "-max and -min should probably be used together, and having "
    "this information logically supersedes the -sp spacing on those "
    "axes.",
    &maxLen);
  thicknessIdx = hestOptAdd_Nv_Double(
    &opt, "th,thickness", "th0 th1", 1, -1, &thickness, "nan",
    "thickness of region represented by one sample along each axis. "
    "  As with -sp spacing, use \"nan\" for "
    "any non-spatial axes.",
    &thicknessLen);
  hestOptAdd_Nv_String(&opt, "k,kind", "k0 k1", 1, -1, &kinds, "",
                       "what \"kind\" is each axis, from the nrrdKind airEnum "
                       "(e.g. space, time, 3-vector, 3D-masked-symmetric-matrix, "
                       "or \"none\" to signify no kind)",
                       &kindsLen);
  hestOptAdd_Nv_String(&opt, "cn,centering", "c0 c1", 1, -1, &centerings, "",
                       "kind of centering (node or cell) for each axis, or "
                       "\"none\" to signify no centering",
                       &centeringsLen);
  hestOptAdd_Nv_String(&opt, "l,label", "lb0 lb1", 1, -1, &label, "",
                       "short string labels for each of the axes", &labelLen);
  hestOptAdd_Nv_String(&opt, "u,unit", "un0 un1", 1, -1, &units, "",
                       "short strings giving units for each of the axes", &unitsLen);
  hestOptAdd_1_String(&opt, "c,content", "content", &content, "",
                      "Specifies the content string of the nrrd, which is built upon "
                      "by many nrrd function to record a history of operations");
  hestOptAdd_1_UInt(&opt, "ls,lineskip", "num", &lineSkip, "0",
                    "number of ascii lines to skip before reading data");
  hestOptAdd_1_LongInt(&opt, "bs,byteskip", "num", &byteSkip, "0",
                       "number of bytes to skip (after skipping ascii lines, if any) "
                       "before reading data.  Can use \"-bs -1\" to skip a binary "
                       "header of unknown length in raw-encoded data");
  strcpy(encInfo,
         "encoding of input data. Possibilities include:"
         "\n \b\bo \"raw\": raw encoding"
         "\n \b\bo \"ascii\": ascii values, one scanline per line of text, "
         "values within line are delimited by space, tab, or comma"
         "\n \b\bo \"hex\": two hex digits per byte");
  if (nrrdEncodingGzip->available()) {
    strcat(encInfo, "\n \b\bo \"gzip\", \"gz\": gzip compressed raw data");
  }
  if (nrrdEncodingBzip2->available()) {
    strcat(encInfo, "\n \b\bo \"bzip2\", \"bz2\": bzip2 compressed raw data");
  }
  hestOptAdd_1_Enum(&opt, "e,encoding", "enc", &encodingType, "raw", encInfo,
                    nrrdEncodingType);
  hestOptAdd_1_Enum(&opt, "en,endian", "end", &endian,
                    airEnumStr(airEndian, airMyEndian()),
                    "Endianness of data; relevent for any data with value "
                    "representation bigger than 8 bits, with a non-ascii encoding: "
                    "\"little\" for Intel and friends "
                    "(least significant byte first, at lower address); "
                    "\"big\" for everyone else (most significant byte first). "
                    "Defaults to endianness of this machine",
                    airEndian);
  hestOptAdd_Nv_String(&opt, "kv,keyvalue", "key/val", 1, -1, &kvp, "",
                       "key/value string pairs to be stored in nrrd.  Each key/value "
                       "pair must be a single string (put it in \"\"s "
                       "if the key or the value contain spaces).  The format of each "
                       "pair is \"<key>:=<value>\", with no spaces before or after "
                       "\":=\".",
                       &kvpLen);
  hestOptAdd_1_String(&opt, "spc,space", "space", &spcStr, "",
                      "identify the space (e.g. \"RAS\", \"LPS\") in which the array "
                      "conceptually lives, from the nrrdSpace airEnum, which in turn "
                      "determines the dimension of the space.  Or, use an integer>0 to"
                      "give the dimension of a space that nrrdSpace doesn't know "
                      "about. "
                      "By default (not using this option), the enclosing space is "
                      "set as unknown.");
  hestOptAdd_1_String(&opt, "orig,origin", "origin", &_origStr, "",
                      "(NOTE: must quote vector) the origin in space of the array: "
                      "the location of the center "
                      "of the first sample, of the form \"(x,y,z)\" (or however "
                      "many coefficients are needed for the chosen space). Quoting the "
                      "vector is needed to stop interpretation from the shell");
  hestOptAdd_1_String(&opt, "dirs,directions", "v0 v1 ...", &_dirStr, "",
                      "(NOTE: must quote whole vector list) The \"space directions\": "
                      "the vectors in space spanned by incrementing (by one) each "
                      "axis index (the column vectors of the index-to-world "
                      "matrix transform), OR, \"none\" for non-spatial axes. Give "
                      "one vector per axis. Using a space direction logically "
                      "supersedes both per-axis -sp spacing and -min,-max. "
                      "(Quoting around whole vector list, not "
                      "individually, prevents the shell from interpreting parentheses)");
  hestOptAdd_1_String(&opt, "mf,measurementframe", "v0 v1 ...", &_mframeStr, "",
                      "(NOTE: must quote whole vector list). Each vector is a *column* "
                      "vector of the matrix which transforms from coordinates in "
                      "measurement frame (in which the coefficients of vectors and "
                      "tensors are given) to coordinates of world space (given with "
                      "\"-spc\"). This is not a per-axis field: the column vectors "
                      "comprise a D-by-D square matrix, where D is the dimension of "
                      "world space.");
  hestOptAdd_Nv_String(&opt, "spu,spaceunit", "su0 su1", 1, -1, &spunits, "",
                       "short strings giving units with which the coefficients of the "
                       "space origin and direction vectors are measured.",
                       &spunitsLen);
  hestOptAdd_1_String(&opt, "o,output", "nout", &out, "-",
                      "output filename.  If \"-h\" has been used, the output file is "
                      "always a detached header.  Otherwise, use extension "
                      "\".nrrd\" to signal creation of self-contained nrrd, and "
                      "\".nhdr\" to signal creating of a detached header with "
                      "(single) data file.");
  hestOptAdd_1_String(&opt, "od,outputdata", "name", &outData, "",
                      "when *not* using \"-h\" and saving to a \".nhdr\" file, using "
                      "this option allows you to explicitly name the data file, "
                      "instead of (by default, not using this option) having it be "
                      "the same filename base as the header file.");
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  airStrtokQuoting = AIR_TRUE;
  USAGE_OR_PARSE(_unrrdu_makeInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);
  encoding = nrrdEncodingArray[encodingType];

  /* ---------------- BEGIN ERROR CHECKING --------------- */

  if (headerOnly) {
    for (ii = 0; ii < nameLen; ii++) {
      if (!strcmp("-", dataFileNames[ii])) {
        fprintf(stderr,
                "%s: can't use detached headers (\"-h\") "
                "with stdin (\"-\") as data source "
                "(filename %d of %d)\n",
                me, ii + 1, nameLen);
        airMopError(mop);
        return 1;
      }
    }
  }
  /* given the information we have, we set the fields in the nrrdIoState
     so as to simulate having read the information from a header */
  if (!(AIR_IN_CL(1, sizeLen, NRRD_DIM_MAX))) {
    fprintf(stderr,
            "%s: # axis sizes (%d) not in valid nrrd dimension "
            "range [1,NRRD_DIM_MAX] = [1,%d]\n",
            me, sizeLen, NRRD_DIM_MAX);
    airMopError(mop);
    return 1;
  }
  gotSpacing = (opt[spacingIdx].source == hestSourceUser);
  if (gotSpacing && spacingLen != sizeLen) {
    fprintf(stderr, "%s: number of spacings (%u) not same as dimension (%u)\n", me,
            spacingLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  gotThickness = (opt[thicknessIdx].source == hestSourceUser);
  if (gotThickness && thicknessLen != sizeLen) {
    fprintf(stderr, "%s: number of thicknesses (%u) not same as dimension (%u)\n", me,
            thicknessLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  gotMin = (opt[minIdx].source == hestSourceUser);
  if (gotMin && minLen != sizeLen) {
    fprintf(stderr, "%s: number of mins (%u) not same as dimension (%u)\n", me, minLen,
            sizeLen);
    airMopError(mop);
    return 1;
  }
  gotMax = (opt[maxIdx].source == hestSourceUser);
  if (gotMax && maxLen != sizeLen) {
    fprintf(stderr, "%s: number of maxs (%u) not same as dimension (%u)\n", me, maxLen,
            sizeLen);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(label[0]) && sizeLen != labelLen) {
    fprintf(stderr, "%s: number of labels (%u) not same as dimension (%u)\n", me,
            labelLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(units[0]) && sizeLen != unitsLen) {
    fprintf(stderr, "%s: number of units (%u) not same as dimension (%u)\n", me,
            unitsLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(kinds[0]) && sizeLen != kindsLen) {
    fprintf(stderr, "%s: number of kinds (%u) not same as dimension (%u)\n", me,
            kindsLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(centerings[0]) && sizeLen != centeringsLen) {
    fprintf(stderr, "%s: number of centerings (%u) not same as dimension (%u)\n", me,
            centeringsLen, sizeLen);
    airMopError(mop);
    return 1;
  }

  /* ----------------- END ERROR CHECKING ---------------- */
  /* ----------------- BEGIN SETTING INFO ---------------- */

  nio = nrrdIoStateNew();
  airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);
  nrrd = nrrdNew();
  airMopAdd(mop, nrrd, (airMopper)nrrdNuke, airMopAlways);

  nrrd->type = type;
  nrrd->dim = sizeLen;
  nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoSize, size);
  /* have to simulate having parsed this line for error checking in
     _nrrdDataFNCheck() to not cause problems */
  nio->seen[nrrdField_sizes] = AIR_TRUE;
  if (nrrdContainsPercentThisAndMore(dataFileNames[0], 'd')) {
    /* trying to do a formatted filename list */
    if (nameLen < 4 || nameLen > 5) {
      fprintf(stderr,
              "%s: formatted list of filenames needs between "
              "3 and 4 ints after the format (not %d)\n",
              me, nameLen - 1);
      airMopError(mop);
      return 1;
    }
    bufLen = 0;
    for (ii = 0; ii < nameLen; ii++) {
      bufLen += strlen(dataFileNames[ii]) + 1;
    }
    parseBuf = AIR_CALLOC(bufLen + 1, char);
    airMopAdd(mop, parseBuf, airFree, airMopAlways);
    strcpy(parseBuf, "");
    for (ii = 0; ii < nameLen; ii++) {
      if (ii) {
        strcat(parseBuf, " ");
      }
      strcat(parseBuf, dataFileNames[ii]);
    }
    nio->line = parseBuf;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_data_file](NULL, nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with formatted filename list \"%s\":\n%s", me,
              parseBuf, err);
      nio->line = NULL;
      airMopError(mop);
      return 1;
    }
    nio->line = NULL;
  } else {
    /* single or regular LIST of files */
    if (nameLen > 1) {
      nio->dataFileDim = dataFileDim ? dataFileDim : nrrd->dim - 1;
    } else {
      nio->dataFileDim = nrrd->dim;
    }
    airArrayLenSet(nio->dataFNArr, nameLen);
    for (ii = 0; ii < nameLen; ii++) {
      nio->dataFN[ii] = airStrdup(dataFileNames[ii]);
    }
  }
  if (_nrrdDataFNCheck(nio, nrrd, AIR_TRUE)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble with input datafiles:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (gotSpacing) {
    nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoSpacing, spacing);
  }
  if (gotThickness) {
    nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoThickness, thickness);
  }
  if (gotMin) {
    nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoMin, axmin);
  }
  if (gotMax) {
    nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoMax, axmax);
  }
  if (airStrlen(label[0])) {
    for (ii = 0; ii < nrrd->dim; ii++) {
      if (!strcmp(NO_STRING, label[ii])) {
        strcpy(label[ii], "");
      }
    }
    nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoLabel, label);
  }
  if (airStrlen(units[0])) {
    for (ii = 0; ii < nrrd->dim; ii++) {
      if (!strcmp(NO_STRING, units[ii])) {
        strcpy(units[ii], "");
      }
    }
    nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoUnits, units);
  }
  if (airStrlen(content)) {
    nrrd->content = airStrdup(content);
  }
  if (kvpLen) {
    for (ii = 0; ii < kvpLen; ii++) {
      /* a hack: have to use NrrdIoState->line as the channel to communicate
         the key/value pair, since we have to emulate it having been
         read from a NRRD header.  But because nio doesn't own the
         memory, we must be careful to unset the pointer prior to
         NrrdIoStateNix being called by the mop. */
      nio->line = kvp[ii];
      nio->pos = 0;
      if (nrrdFieldInfoParse[nrrdField_keyvalue](NULL, nrrd, nio, AIR_TRUE)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble with key/value %d \"%s\":\n%s", me, ii, kvp[ii],
                err);
        nio->line = NULL;
        airMopError(mop);
        return 1;
      }
      nio->line = NULL;
    }
  }
  if (airStrlen(kinds[0])) {
    /* have to allocate line then pass it to parsing */
    bufLen = 0;
    for (ii = 0; ii < sizeLen; ii++) {
      bufLen += airStrlen(" ") + airStrlen(kinds[ii]);
    }
    parseBuf = AIR_CALLOC(bufLen + 1, char);
    airMopAdd(mop, parseBuf, airFree, airMopAlways);
    strcpy(parseBuf, "");
    for (ii = 0; ii < sizeLen; ii++) {
      if (ii) {
        strcat(parseBuf, " ");
      }
      strcat(parseBuf, kinds[ii]);
    }
    nio->line = parseBuf;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_kinds](NULL, nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with kinds \"%s\":\n%s", me, parseBuf, err);
      nio->line = NULL;
      airMopError(mop);
      return 1;
    }
    nio->line = NULL;
  }
  if (airStrlen(centerings[0])) {
    /* have to allocate line then pass it to parsing */
    bufLen = 0;
    for (ii = 0; ii < sizeLen; ii++) {
      bufLen += airStrlen(" ") + airStrlen(centerings[ii]);
    }
    parseBuf = AIR_CALLOC(bufLen + 1, char);
    airMopAdd(mop, parseBuf, airFree, airMopAlways);
    strcpy(parseBuf, "");
    for (ii = 0; ii < sizeLen; ii++) {
      if (ii) {
        strcat(parseBuf, " ");
      }
      strcat(parseBuf, centerings[ii]);
    }
    nio->line = parseBuf;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_centers](NULL, nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with centerings \"%s\":\n%s", me, parseBuf, err);
      nio->line = NULL;
      airMopError(mop);
      return 1;
    }
    nio->line = NULL;
  }
  if (airStrlen(spcStr)) {
    space = airEnumVal(nrrdSpace, spcStr);
    if (!space) {
      /* couldn't parse it as space, perhaps its a uint */
      if (1 != sscanf(spcStr, "%u", &spaceDim)) {
        fprintf(stderr,
                "%s: couldn't parse \"%s\" as a nrrdSpace "
                "or as a uint",
                me, spcStr);
        airMopError(mop);
        return 1;
      }
      /* else we did parse it as a uint */
      nrrd->space = nrrdSpaceUnknown;
      nrrd->spaceDim = spaceDim;
    } else {
      /* we did parse a known space */
      nrrdSpaceSet(nrrd, space);
    }
    spaceSet = AIR_TRUE;
  } else {
    /* we got no space information at all */
    nrrdSpaceSet(nrrd, nrrdSpaceUnknown);
    spaceSet = AIR_FALSE;
  }
  if (airStrlen(_origStr)) {
    /* why this is necessary is a bit confusing to me, both the check for
       enclosing quotes, and the need to use to a separate variable (isn't
       hest doing memory management of addresses, not variables?) */
    if ('\"' == _origStr[0] && '\"' == _origStr[strlen(_origStr) - 1]) {
      _origStr[strlen(_origStr) - 1] = 0;
      origStr = _origStr + 1;
    } else {
      origStr = _origStr;
    }
    /* same hack about using NrrdIoState->line as basis for parsing */
    nio->line = origStr;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_space_origin](NULL, nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with origin \"%s\":\n%s", me, origStr, err);
      nio->line = NULL;
      airMopError(mop);
      return 1;
    }
    nio->line = NULL;
  }
  if (airStrlen(_dirStr)) {
    /* same confusion as above */
    if ('\"' == _dirStr[0] && '\"' == _dirStr[strlen(_dirStr) - 1]) {
      _dirStr[strlen(_dirStr) - 1] = 0;
      dirStr = _dirStr + 1;
    } else {
      dirStr = _dirStr;
    }
    /* same hack about using NrrdIoState->line as basis for parsing */
    nio->line = dirStr;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_space_directions](NULL, nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with space directions \"%s\":\n%s", me, dirStr, err);
      nio->line = NULL;
      airMopError(mop);
      return 1;
    }
    nio->line = NULL;
  }
  if (airStrlen(_mframeStr)) {
    /* same confusion as above */
    if ('\"' == _mframeStr[0] && '\"' == _mframeStr[strlen(_mframeStr) - 1]) {
      _mframeStr[strlen(_mframeStr) - 1] = 0;
      mframeStr = _mframeStr + 1;
    } else {
      mframeStr = _mframeStr;
    }
    /* same hack about using NrrdIoState->line as basis for parsing */
    nio->line = mframeStr;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_measurement_frame](NULL, nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with measurement frame \"%s\":\n%s", me, mframeStr,
              err);
      nio->line = NULL;
      airMopError(mop);
      return 1;
    }
    nio->line = NULL;
  }
  if (airStrlen(spunits[0])) {
    if (!spaceSet) {
      fprintf(stderr, "%s: can't have space units with no space set\n", me);
      airMopError(mop);
      return 1;
    }
    if (nrrd->spaceDim != spunitsLen) {
      fprintf(stderr,
              "%s: number of space units (%d) "
              "not same as space dimension (%d)\n",
              me, spunitsLen, nrrd->spaceDim);
      airMopError(mop);
      return 1;
    }
    for (ii = 0; ii < nrrd->spaceDim; ii++) {
      if (!strcmp(NO_STRING, spunits[ii])) {
        strcpy(spunits[ii], "");
      }
    }
    /* have to allocate line then pass it to parsing */
    bufLen = 0;
    for (ii = 0; ii < nrrd->spaceDim; ii++) {
      bufLen += airStrlen(" ") + airStrlen("\"\"") + airStrlen(spunits[ii]);
    }
    parseBuf = AIR_CALLOC(bufLen + 1, char);
    airMopAdd(mop, parseBuf, airFree, airMopAlways);
    strcpy(parseBuf, "");
    for (ii = 0; ii < nrrd->spaceDim; ii++) {
      if (ii) {
        strcat(parseBuf, " ");
      }
      strcat(parseBuf, "\"");
      strcat(parseBuf, spunits[ii]);
      strcat(parseBuf, "\"");
    }
    nio->line = parseBuf;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_space_units](NULL, nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with space units \"%s\":\n%s", me, parseBuf, err);
      nio->line = NULL;
      airMopError(mop);
      return 1;
    }
    nio->line = NULL;
  }
  if (_nrrdCheck(nrrd, AIR_FALSE, AIR_TRUE)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: problems with nrrd as set up:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  /* ----------------- END SETTING INFO ---------------- */
  /* -------------------- BEGIN I/O -------------------- */

  nio->lineSkip = lineSkip;
  nio->byteSkip = byteSkip;
  nio->encoding = encoding;
  nio->endian = endian;
  /* for the sake of reading in data files, this is as good a guess
     as any as to what the header-relative path to them is.  This
     assuages concerns that come up even with headerOnly */
  nio->path = airStrdup(".");
  if (headerOnly) {
    /* we open and hand off the output FILE* to the nrrd writer, which
       will not write any data, because of nio->skipData = AIR_TRUE */
    if (!(fileOut = airFopen(out, stdout, "wb"))) {
      fprintf(stderr, "%s: couldn't fopen(\"%s\",\"wb\"): %s\n", me, out,
              strerror(errno));
      airMopError(mop);
      return 1;
    }
    airMopAdd(mop, fileOut, (airMopper)airFclose, airMopAlways);
    /* whatever line and byte skipping is required will be simply
       recorded in the header, and done by the next reader */
    nio->detachedHeader = AIR_TRUE;
    nio->skipData = AIR_TRUE;
    if (nrrdFormatNRRD->write(fileOut, nrrd, nio)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble writing header:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
  } else {
    /* all this does is read the data from the files.  We up the verbosity
       because of all places this is probably where we really want it */
    nrrdStateVerboseIO++;
    if (nrrdFormatNRRD->read(NULL, nrrd, nio)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble reading data files:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    nrrdStateVerboseIO--;
    /* then save normally */
    nrrdIoStateInit(nio);
    if (strlen(outData)) {
      airArrayLenSet(nio->dataFNArr, 1);
      nio->dataFN[0] = airStrdup(outData);
    }
    SAVE(out, nrrd, nio);
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(make, INFO);
