/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (c) 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "teem/air.h"

/*
** the purpose of this test is to make sure that the airRandMT
** RNG generates exactly the numbers we expect it to, for a variety
** of seeds, so that anything using the RNG is reproducible 
*/

#define NUM 10
int
main(int argc, const char *argv[]) {
  airArray *mop;
  airRandMTState *rng;
  const char *me;
  unsigned int rval[NUM][NUM] = {
    {2357136044, 2546248239, 3071714933, 3626093760, 2588848963,
     3684848379, 2340255427, 3638918503, 1819583497, 2678185683},
    {1608637542, 3421126067, 4083286876, 787846414, 3143890026,
     3348747335, 2571218620, 2563451924, 670094950, 1914837113},
    {197744554, 2405527281, 1590178649, 2055114040, 1040749045,
     1355459964, 2699301070, 1591340141, 4252490304, 3121394926},
    {451710822, 4140706415, 550374602, 880776961, 375407235,
     576831824, 495976644, 1350392909, 3211465673, 1227650870},
    {2567526101, 397661439, 2237017401, 316000557, 1060138423,
     2802111455, 1449535759, 751581949, 3635455645, 658021748},
    {429171210, 2009581671, 1300722668, 3858470021, 3363216262,
     1963629412, 2166299591, 229689286, 484002369, 2062223911},
    {23250075, 3670330222, 1860540774, 4216169317, 1062279565,
     2886996639, 2197431119, 3112004045, 3229777453, 1632140913},
    {2869147098, 1558248213, 585501645, 3600180646, 2654279825,
     3658135664, 287832047, 912891514, 2926707351, 937957965},
    {1891499427, 1885608988, 3850740167, 3832766153, 2073041664,
     3289176644, 989474400, 2841420218, 4096852366, 1816963771},
    {2552602868, 2086504389, 219288614, 3347214808, 215326247,
     3609464630, 3506494207, 997691580, 1726903302, 3302470737}
  };
  unsigned int ii, jj, rr;

  AIR_UNUSED(argc);
  me = argv[0];
  
  rng = airRandMTStateNew(0);
  airMopAdd(mop, rng, (airMopper)airRandMTStateNix, airMopAlways);
  for (jj=0; jj<NUM; jj++) {
    airSrandMT_r(rng, 42*jj);
    for (ii=0; ii<NUM; ii++) {
      rr = airUIrandMT_r(rng);
      if (rval[jj][ii] != rr) {
        fprintf(stderr, "%s: rval[%u][%u] %u != %u\n",
                me, jj, ii, rval[jj][ii], rr);
        airMopError(mop);
        exit(1);
      }
    }
  }

  airMopOkay(mop);
  exit(0);
}



