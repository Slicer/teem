/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define NO_STRING "."

#define INFO "Create a nrrd (or nrrd header) from scratch"
char *_unrrdu_makeInfoL =
(INFO
 ".  The data can be in one or more files, or coming from stdin. "
 "This provides an easy way of providing the bare minimum "
 "information about some data so as to wrap it in a "
 "nrrd, either to pass on for further unu processing, "
 "or to save to disk.  However, with \"-h\", this creates "
 "only a detached nrrd header file, without ever reading "
 "or writing data. \n \n "
 "When reading multiple files, each file must contain "
 "the data for one slice along the slowest axis.  Nearly all the options "
 "below refer to the finished nrrd resulting from joining all the slices "
 "together, with the exception of \"-ls\", \"-bs\", and \"-e\", which apply "
 "to every input slice file.  When reading data from many seperate files, it "
 "may be easier to put their filenames in a response file; there can be one "
 "or more filenames per line of the response file. \n \n "
 "NOTE: for the \"-l\" (labels), \"-u\" (units), and \"-spu\" (space units) "
 "options below, you can use a single unquoted period (\".\") to signify "
 "an empty string.  This creates a convenient way to convey something that "
 "the shell doesn't make it easy to convey.  Shell expansion weirdness "
 "also requires the use of quotes around the arguments to \"-orig\" (space "
 "origin) and \"-dirs\" (space directions).");

int
unrrduMakeRead(char *me, Nrrd *nrrd, NrrdIoState *nio, const char *fname,
               int lineSkip, int byteSkip, const NrrdEncoding *encoding) {
  char err[AIR_STRLEN_MED];

  nrrdIoStateInit(nio);
  nio->lineSkip = lineSkip;
  nio->byteSkip = byteSkip;
  nio->encoding = encoding;
  if (!( nio->dataFile = airFopen(fname, stdin, "rb") )) {
    sprintf(err, "%s:  couldn't fopen(\"%s\",\"rb\"): %s\n", 
            me, fname, strerror(errno));
    biffAdd(me, err); return 1;
  }
  if (nrrdLineSkip(nio)) {
    sprintf(err, "%s: couldn't skip lines", me);
    nio->dataFile = airFclose(nio->dataFile);
    biffMove(me, err, NRRD); return 1;
  }
  if (!nio->encoding->isCompression) {
    if (nrrdByteSkip(nrrd, nio)) {
      sprintf(err, "%s: couldn't skip bytes", me);
      nio->dataFile = airFclose(nio->dataFile);
      biffMove(me, err, NRRD); return 1;
    }
  }
  if (nio->encoding->read(nrrd, nio)) {
    sprintf(err, "%s: error reading data", me);
    nio->dataFile = airFclose(nio->dataFile);
    biffMove(me, err, NRRD); return 1;
  }
  nio->dataFile = airFclose(nio->dataFile);
  return 0;
}

int
unrrdu_makeMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, **dataFileNames, **kvp, *content, encInfo[AIR_STRLEN_LARGE];
  Nrrd *nrrd, **nslice, *njoin, *nsave;
  int *size, sizeLen, buflen,
    nameLen, kvpLen, ii, spacingLen, thicknessLen, labelLen, unitsLen,
    spunitsLen, headerOnly, pret, lineSkip, byteSkip, endian, slc, type,
    encodingType, gotSpacing, gotThickness, space, spaceDim, kindsLen,
    centeringsLen, spaceSet;
  double *spacing, *thickness;
  airArray *mop;
  NrrdIoState *nio;
  FILE *fileOut;
  char **label, **units, **spunits, **kinds, **centerings, *parseBuf,
    *spcStr, *_origStr, *origStr, *_dirStr, *dirStr;
  const NrrdEncoding *encoding;

  /* so that long lists of filenames can be read from file */
  hparm->respFileEnable = AIR_TRUE;
  hparm->greedySingleString = AIR_TRUE;

  mop = airMopNew();
  
  hestOptAdd(&opt, "h", NULL, airTypeBool, 0, 0, &headerOnly, NULL,
             "Generate header ONLY: don't write out the whole nrrd, "
             "don't even bother reading the input data, just output the "
             "detached nrrd header file (usually with a \".nhdr\" "
             "extension) determined by the options below. "
             "*NOTE*: The filename given with \"-i\" should probably start "
             "with \"./\" to indicate that the "
             "data file is to be found relative to the header file "
             "(as opposed to the current working directory of whomever "
             "is reading the nrrd).  Detached headers are incompatible with "
             "using stdin as the data source, or using multiple data "
             "files");
  hestOptAdd(&opt, "i", "file", airTypeString, 1, -1, &dataFileNames, "-",
             "Filename(s) of data file(s); use \"-\" for stdin. ", &nameLen);
  hestOptAdd(&opt, "t", "type", airTypeEnum, 1, 1, &type, NULL,
             "type of data (e.g. \"uchar\", \"int\", \"float\", "
             "\"double\", etc.)",
             NULL, nrrdType);
  hestOptAdd(&opt, "s", "sz0 sz1", airTypeInt, 1, -1, &size, NULL,
             "number of samples along each axis (and implicit indicator "
             "of dimension of nrrd)", &sizeLen);
  hestOptAdd(&opt, "sp", "sp0 sp1", airTypeDouble, 1, -1, &spacing, "nan",
             "spacing between samples on each axis.  Use \"nan\" for "
             "any non-spatial axes (e.g. spacing between red, green, and blue "
             "along axis 0 of interleaved RGB image data)", &spacingLen);
  hestOptAdd(&opt, "th", "th0 th1", airTypeDouble, 1, -1, &thickness, "nan",
             "thickness of region represented by one sample along each axis. "
             "  As with spacing, use \"nan\" for "
             "any non-spatial axes.", &thicknessLen);
  hestOptAdd(&opt, "k", "kind0 kind1", airTypeString, 1, -1, &kinds, "",
             "what \"kind\" is each axis, from the nrrdKind airEnum "
             "(e.g. space, time, 3-vector, 3D-masked-symmetric-matrix, "
             "or \"none\" to signify no kind)", &kindsLen);
  hestOptAdd(&opt, "cn", "cent0 cent1", airTypeString, 1, -1, &centerings, "",
             "kind of centering (node or cell) for each axis, or "
             "\"none\" to signify no centering", &centeringsLen);
  hestOptAdd(&opt, "l", "lb0 lb1", airTypeString, 1, -1, &label, "",
             "short string labels for each of the axes", &labelLen);
  hestOptAdd(&opt, "u", "un0 un1", airTypeString, 1, -1, &units, "",
             "short strings giving units for each of the axes", &unitsLen);
  hestOptAdd(&opt, "c", "content", airTypeString, 1, 1, &content, "",
             "Specifies the content string of the nrrd, which is built upon "
             "by many nrrd function to record a history of operations");
  hestOptAdd(&opt, "ls", "lineskip", airTypeInt, 1, 1, &lineSkip, "0",
             "number of ascii lines to skip before reading data");
  hestOptAdd(&opt, "bs", "byteskip", airTypeInt, 1, 1, &byteSkip, "0",
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
    strcat(encInfo, 
           "\n \b\bo \"gzip\", \"gz\": gzip compressed raw data");
  }
  if (nrrdEncodingBzip2->available()) {
    strcat(encInfo, 
           "\n \b\bo \"bzip2\", \"bz2\": bzip2 compressed raw data");
  }
  hestOptAdd(&opt, "e", "encoding", airTypeEnum, 1, 1, &encodingType, "raw",
             encInfo, NULL, nrrdEncodingType);
  hestOptAdd(&opt, "en", "endian", airTypeEnum, 1, 1, &endian,
             airEnumStr(airEndian, airMyEndian),
             "Endianness of data; relevent for any data with value "
             "representation bigger than 8 bits, with a non-ascii encoding: "
             "\"little\" for Intel and friends "
             "(least significant byte first, at lower address); "
             "\"big\" for everyone else (most significant byte first). "
             "Defaults to endianness of this machine",
             NULL, airEndian);
  hestOptAdd(&opt, "kv", "key/val", airTypeString, 0, -1, &kvp, "",
             "key/value string pairs to be stored in nrrd.  Each key/value "
             "pair must be a single string (put it in \"\"s "
             "if the key or the value contain spaces).  The format of each "
             "pair is \"<key>:=<value>\", with no spaces before or after "
             "\":=\".", &kvpLen);
  hestOptAdd(&opt, "spc", "space", airTypeString, 1, 1, &spcStr, "",
             "identify the space (e.g. \"RAS\", \"LPS\") in which the array "
             "conceptually lives, from the nrrdSpace airEnum, which in turn "
             "determines the dimension of the space.  Or, use an integer to "
             "give the dimension of a space that nrrdSpace doesn't know about "
             "By default (not using this option), the enclosing space is "
             "set as unknown.");
  hestOptAdd(&opt, "orig", "origin", airTypeString, 1, 1, &_origStr, "",
             "(NOTE: must quote vector) the origin in space of the array: "
             "the location of the center "
             "of the first sample, of the form \"(x,y,z)\" (or however "
             "many coefficients are needed for the chosen space). Quoting the "
             "vector is needed to stop interpretation from the with shell");
  hestOptAdd(&opt, "dirs", "dir0 dir1 ...", airTypeString, 1, 1, &_dirStr, "",
             "(NOTE: must quote whole vector list) The \"space directions\": "
             "the vectors in space spanned by incrementing (by one) each "
             "axis index (the column vectors of the index-to-world "
             "matrix transform), OR, \"none\" for non-spatial axes. Quoting "
             "around vector list (not individually) is needed because of "
             "limitations in the parser.");
  hestOptAdd(&opt, "spu", "spu0 spu1", airTypeString, 1, -1, &spunits, "",
             "short strings giving units with which the coefficients of the "
             "space origin and direction vectors are measured.", &spunitsLen);
  OPT_ADD_NOUT(out, "output filename");
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_makeInfoL);
  airStrtokQuoting = AIR_TRUE;
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);
  encoding = nrrdEncodingArray[encodingType];


  /********************************************************************
   ********************************************************************
      It is no secret that this code is awful and needs a re-write.
      The deficiencies have nearly everything to do with hest, though,
      and little to do with nrrd itself, except that having a
      nrrdMake() function would probably simplify the following...
   ********************************************************************
   ********************************************************************/


  /* given the information we have, we set the fields in the nrrdIoState
     so as to simulate having read the information from a header */
  if (!( AIR_IN_CL(1, sizeLen, NRRD_DIM_MAX) )) {
    fprintf(stderr, "%s: # axis sizes (%d) not in valid nrrd dimension "
            "range [1,NRRD_DIM_MAX] = [1,%d]\n", me, sizeLen, NRRD_DIM_MAX);
    airMopError(mop);
    return 1;
  }
  gotSpacing = (spacingLen > 1 ||
                (sizeLen == 1 && AIR_EXISTS(spacing[0])));
  if (gotSpacing && spacingLen != sizeLen) {
    fprintf(stderr,
            "%s: number of spacings (%d) not same as dimension (%d)\n",
            me, spacingLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  gotThickness = (thicknessLen > 1 ||
                  (sizeLen == 1 && AIR_EXISTS(thickness[0])));
  if (gotThickness && thicknessLen != sizeLen) {
    fprintf(stderr,
            "%s: number of thicknesses (%d) not same as dimension (%d)\n",
            me, thicknessLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(label[0]) && sizeLen != labelLen) {
    fprintf(stderr,
            "%s: number of labels (%d) not same as dimension (%d)\n",
            me, labelLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(units[0]) && sizeLen != unitsLen) {
    fprintf(stderr,
            "%s: number of units (%d) not same as dimension (%d)\n",
            me, unitsLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(kinds[0]) && sizeLen != kindsLen) {
    fprintf(stderr,
            "%s: number of kinds (%d) not same as dimension (%d)\n",
            me, kindsLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(centerings[0]) && sizeLen != centeringsLen) {
    fprintf(stderr,
            "%s: number of centerings (%d) not same as dimension (%d)\n",
            me, centeringsLen, sizeLen);
    airMopError(mop);
    return 1;
  }
  if (nameLen > 1 && nameLen != size[sizeLen-1]) {
    fprintf(stderr, "%s: got %d slice filenames but the last axis has %d "
            "elements\n", me, nameLen, size[sizeLen-1]);
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
  if (gotSpacing) {
    nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoSpacing, spacing);
  }
  if (gotThickness) {
    nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoThickness, thickness);
  }
  if (airStrlen(label[0])) {
    for (ii=0; ii<nrrd->dim; ii++) {
      if (!strcmp(NO_STRING, label[ii])) {
        strcpy(label[ii], "");
      }
    }
    nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoLabel, label);
  }
  if (airStrlen(units[0])) {
    for (ii=0; ii<nrrd->dim; ii++) {
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
    for (ii=0; ii<kvpLen; ii++) {
      /* a hack: have to use NrrdIoState->line as the channel to communicate
         the key/value pair, since we have to emulate it having been
         read from a NRRD header.  But because nio doesn't own the 
         memory, we must be careful to unset the pointer prior to 
         NrrdIoStateNix being called by the mop. */
      nio->line = kvp[ii];
      nio->pos = 0;
      if (nrrdFieldInfoParse[nrrdField_keyvalue](nrrd, nio, AIR_TRUE)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble with key/value %d \"%s\":\n%s",
                me, ii, kvp[ii], err);
        nio->line = NULL; airMopError(mop); return 1;
      }
      nio->line = NULL;
    }
  }
  if (airStrlen(kinds[0])) {
    /* have to allocate line then pass it to parsing */
    buflen = 0;
    for (ii=0; ii<sizeLen; ii++) {
      buflen += airStrlen(" ") + airStrlen(kinds[ii]);
    }
    buflen += 1;
    parseBuf = calloc(buflen, sizeof(char));
    airMopAdd(mop, parseBuf, airFree, airMopAlways);
    strcpy(parseBuf, "");
    for (ii=0; ii<sizeLen; ii++) {
      if (ii) {
        strcat(parseBuf, " ");
      }
      strcat(parseBuf, kinds[ii]);
    }
    nio->line = parseBuf;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_kinds](nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with kinds \"%s\":\n%s",
              me, parseBuf, err);
      nio->line = NULL; airMopError(mop); return 1;
    }
    nio->line = NULL;
  }
  if (airStrlen(centerings[0])) {
    /* have to allocate line then pass it to parsing */
    buflen = 0;
    for (ii=0; ii<sizeLen; ii++) {
      buflen += airStrlen(" ") + airStrlen(centerings[ii]);
    }
    buflen += 1;
    parseBuf = calloc(buflen, sizeof(char));
    airMopAdd(mop, parseBuf, airFree, airMopAlways);
    strcpy(parseBuf, "");
    for (ii=0; ii<sizeLen; ii++) {
      if (ii) {
        strcat(parseBuf, " ");
      }
      strcat(parseBuf, centerings[ii]);
    }
    nio->line = parseBuf;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_centers](nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with centerings \"%s\":\n%s",
              me, parseBuf, err);
      nio->line = NULL; airMopError(mop); return 1;
    }
    nio->line = NULL;
  }
  if (airStrlen(spcStr)) {
    space = airEnumVal(nrrdSpace, spcStr);
    if (!space) {
      /* couldn't parse it as space, perhaps its an int */
      if (1 != sscanf(spcStr, "%d", &spaceDim)) {
        fprintf(stderr, "%s: couldn't parse \"%s\" as a nrrdSpace "
                "or as an int", me, spcStr);
        airMopError(mop); return 1;
      }
      /* else we did parse it as an int */
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
       enclosing quotes, and the need to use to a seperate variable (isn't
       hest doing memory management of addresses, not variables?) */
    if ('\"' == _origStr[0] && '\"' == _origStr[strlen(_origStr)-1]) {
      _origStr[strlen(_origStr)-1] = 0;
      origStr = _origStr + 1;
    } else {
      origStr = _origStr;
    }
    /* same hack about using NrrdIoState->line as basis for parsing */
    nio->line = origStr;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_space_origin](nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with origin \"%s\":\n%s",
              me, origStr, err);
      nio->line = NULL; airMopError(mop); return 1;
    }
    nio->line = NULL;
  }
  if (airStrlen(_dirStr)) {
    /* same confusion as above */
    if ('\"' == _dirStr[0] && '\"' == _dirStr[strlen(_dirStr)-1]) {
      _dirStr[strlen(_dirStr)-1] = 0;
      dirStr = _dirStr + 1;
    } else {
      dirStr = _dirStr;
    }
    /* same hack about using NrrdIoState->line as basis for parsing */
    nio->line = dirStr;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_space_directions](nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with space directions \"%s\":\n%s",
              me, dirStr, err);
      nio->line = NULL; airMopError(mop); return 1;
    }
    nio->line = NULL;
  }
  if (airStrlen(spunits[0])) {
    if (!spaceSet) {
      fprintf(stderr, "%s: can't have space units with no space set\n", me);
      airMopError(mop); return 1;
    }
    if (nrrd->spaceDim != spunitsLen) {
      fprintf(stderr,
              "%s: number of space units (%d) "
              "not same as space dimension (%d)\n",
              me, spunitsLen, nrrd->spaceDim);
      airMopError(mop);
      return 1;
    }
    for (ii=0; ii<nrrd->spaceDim; ii++) {
      if (!strcmp(NO_STRING, spunits[ii])) {
        strcpy(spunits[ii], "");
      }
    }
    /* have to allocate line then pass it to parsing */
    buflen = 0;
    for (ii=0; ii<nrrd->spaceDim; ii++) {
      buflen += airStrlen(" ") + airStrlen("\"\"") + airStrlen(spunits[ii]);
    }
    buflen += 1;
    parseBuf = calloc(buflen, sizeof(char));
    airMopAdd(mop, parseBuf, airFree, airMopAlways);
    strcpy(parseBuf, "");
    for (ii=0; ii<nrrd->spaceDim; ii++) {
      if (ii) {
        strcat(parseBuf, " ");
      }
      strcat(parseBuf, "\"");
      strcat(parseBuf, spunits[ii]);
      strcat(parseBuf, "\"");
    }
    nio->line = parseBuf;
    nio->pos = 0;
    if (nrrdFieldInfoParse[nrrdField_space_units](nrrd, nio, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with space units \"%s\":\n%s",
              me, parseBuf, err);
      nio->line = NULL; airMopError(mop); return 1;
    }
    nio->line = NULL;
  }
  if (_nrrdCheck(nrrd, AIR_FALSE)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: problems with nrrd as set up:\n%s", me, err);
    airMopError(mop); return 1;
  }
  
  /* ----------------- END SETTING INFO ---------------- */
  /* -------------------- BEGIN I/O -------------------- */

  if (headerOnly) {
    /* we don't have to fopen() any input; all we care about
       is the name of the input datafile.  We disallow stdin here */
    if (1 != nameLen) {
      fprintf(stderr, "%s: can't use detached headers with multiple "
              "data files\n", me);
      airMopError(mop); return 1;
    }
    if (!strcmp("-", dataFileNames[0])) {
      fprintf(stderr, "%s: can't use detached headers with stdin as "
              "data source\n", me);
      airMopError(mop); return 1;
    }
    nio->lineSkip = lineSkip;
    nio->byteSkip = byteSkip;
    nio->encoding = encoding;
    nio->dataFN = airStrdup(dataFileNames[0]);
    nio->detachedHeader = AIR_TRUE;
    nio->skipData = AIR_TRUE;
    nio->endian = endian;
    /* we open and hand off the output FILE* to the nrrd writer, which
       will not write any data, because of nio->skipData = AIR_TRUE */
    if (!( fileOut = airFopen(out, stdout, "wb") )) {
      fprintf(stderr, "%s: couldn't fopen(\"%s\",\"wb\"): %s\n", 
              me, out, strerror(errno));
      airMopError(mop); return 1;
    }
    airMopAdd(mop, fileOut, (airMopper)airFclose, airMopAlways);
    /* whatever line and byte skipping is required will be simply
       recorded in the header, and done by the next reader */
    nrrdFormatNRRD->write(fileOut, nrrd, nio);
  } else {
    /* else not headerOnly */
    /* we're not actually using the handy unrrduHestFileCB,
       since we have to open the input data file by hand */
    if (1 == nameLen) {
      if (unrrduMakeRead(me, nrrd, nio, dataFileNames[0],
                         lineSkip, byteSkip, encoding)) {
        airMopAdd(mop, err = biffGetDone(me), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble reading from \"%s\":\n%s",
                me, dataFileNames[0], err);
        airMopError(mop); return 1;
      }
      nsave = nrrd;
    } else {
      /* create one nrrd for each slice, read them all in, then
         join them together */
      njoin = nrrdNew();
      airMopAdd(mop, njoin, (airMopper)nrrdNuke, airMopAlways);
      nslice = (Nrrd **)calloc(nameLen, sizeof(Nrrd *));
      if (!nslice) {
        fprintf(stderr, "%s: couldn't allocate nslice array!\n", me);
        airMopError(mop); return 1;
      }
      airMopAdd(mop, nslice, airFree, airMopAlways);
      for (slc=0; slc<nameLen; slc++) {
        nslice[slc] = nrrdNew();
        airMopAdd(mop, nslice[slc], (airMopper)nrrdNuke, airMopAlways);
        nslice[slc]->type = type;
        nslice[slc]->dim = sizeLen-1;
        /* the last element of size[] will be ignored */
        nrrdAxisInfoSet_nva(nslice[slc], nrrdAxisInfoSize, size);
        if (unrrduMakeRead(me, nslice[slc], nio, dataFileNames[slc],
                           lineSkip, byteSkip, encoding)) {
          airMopAdd(mop, err = biffGetDone(me), airFree, airMopAlways);
          fprintf(stderr, "%s: trouble reading from \"%s\" "
                  "(file %d of %d):\n%s",
                  me, dataFileNames[slc], slc+1, nameLen, err);
          airMopError(mop); return 1;
        }
      }
      if (nrrdJoin(njoin, (const Nrrd**)nslice,
                   nameLen, nrrd->dim-1, AIR_TRUE)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble joining slices together:\n%s",
                me, err);
        airMopError(mop); return 1;
      }
      /* copy peripheral information already set in nrrd to njoin */
      nrrdBasicInfoCopy(njoin, nrrd, (NRRD_BASIC_INFO_DATA_BIT
                                      | NRRD_BASIC_INFO_TYPE_BIT
                                      | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                                      | NRRD_BASIC_INFO_DIMENSION_BIT));
      nrrdAxisInfoCopy(njoin, nrrd, NULL, NRRD_AXIS_INFO_NONE);
      nsave = njoin;
    }
    if (1 < nrrdElementSize(nsave)
        && encoding->endianMatters
        && endian != AIR_ENDIAN) {
      /* endianness exposed in encoding, and its wrong */
      nrrdSwapEndian(nsave);
    }
    /* we are saving normally- no need to subvert nrrdSave() here;
       we just pass it the output filename */
    SAVE(out, nsave, NULL);
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(make, INFO);
