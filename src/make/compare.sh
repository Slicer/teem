#
# teem: Gordon Kindlmann's research software
# Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#

# Stoopid little shell script to determine if a .usable file needs
# to name the installed headers and libs as prerequisites (thereby
# triggering their re-install).  Before this is run, two variables
# are set:
#
#   $me : .usable filename for "me", lib L
#   $need : .usable filenames of every lib which L depends on
#
# The re-install is needed if:
# (test 1) The needed .usable wasn't there, then the fact that its
# already been named as a prerequisite to $me means that it will be
# built, which means that L will have to re-installed, or
# (test 2) The needed .usable file is newer than $me

for nd in $need
do   # (test 1)    (  test 2 )
  if [ ! -f $nd -o $nd -nt $me ]
    then echo $nd
  fi
done

