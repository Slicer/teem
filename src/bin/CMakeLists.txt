#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2023  University of Chicago
# Copyright (C) 2005--2008  Gordon Kindlmann
# Copyright (C) 1998--2004  University of Utah
#
# This library is free software; you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License (LGPL) as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also include exceptions to
# the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with
# this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
# Fifth Floor, Boston, MA 02110-1301 USA
#

macro(CREATE_EXEC name sources)
  add_executable(${name} ${sources})
  target_link_libraries(${name} teem)
  install(TARGETS ${name}
    RUNTIME DESTINATION bin
    COMPONENT Runtime
    )
  set_target_properties(${name} PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib
    )
endmacro()

# These binaries should not be depending on BUILD_EXPERIMENTAL_LIBS
CREATE_EXEC(nrrdSanity nrrdSanity.c)
CREATE_EXEC(overrgb overrgb.c)
CREATE_EXEC(mrender mrender.c)
CREATE_EXEC(miter miter.c)
CREATE_EXEC(vprobe vprobe.c)
CREATE_EXEC(gprobe gprobe.c)
# CREATE_EXEC(ninspect ninspect.c)  # moved to "unu ninspect"
CREATE_EXEC(unu unu.c)
CREATE_EXEC(puller puller.c)
CREATE_EXEC(tend tend.c)

# For Teem v2 many of these were removed from compilation: they have no compelling
# general purpose, for reasons indicated with further comments. No reason
# to delete the sources though.
# NOTE: The BUILD_EXPERIMENTAL_LIBS-dependent inclusion of binaries has to be
# done with knowledge of how BUILD_EXPERIMENTAL_LIBS behaves in the top-level
# CMakeLists.txt file
if(BUILD_EXPERIMENTAL_APPS)
#   CREATE_EXEC(airSanity airSanity.c)  # tests included in nrrdSanity
#   CREATE_EXEC(cubic cubic.c) # just calls ell_cubic; more easily done via Python wrappers
#   CREATE_EXEC(qbert qbert.c) # created for volume rendering on GPUs from 20 years ago
#   CREATE_EXEC(ilk ilk.c)     # now part of unu: "unu ilk"
  CREATE_EXEC(emap emap.c)     # (functionality not currently handled elsewhere)
#   CREATE_EXEC(talkweb talkweb.c)   # the web has changed in 20 years
#   CREATE_EXEC(pprobe pprobe.c)     # gprobe can do this
#   CREATE_EXEC(ungantry ungantry.c) # gprobe can do this
   CREATE_EXEC(deconv deconv.c) # (functionality not currently handled elsewhere)
#   if(BUILD_EXPERIMENTAL_LIBS)
#     CREATE_EXEC(gkms gkms.c)   # hasn't been tested in a long time
#     CREATE_EXEC(spots spots.c) # hasn't been tested in a long time
#   endif()
endif()
