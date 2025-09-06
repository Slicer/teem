# CTestConfig.cmake - Dashboard submission settings for Teem
# Copyright (C) 1998--2025 Teem contributors
# Licensed under the GNU LGPL v2.1 or later (see LICENSE.txt for details)

# This file is used only when running CTest in "dashboard mode":
#   ctest -D Experimental
#   ctest -D Nightly
#   ctest -D Continuous
#
# Local developer runs (just `ctest`) do NOT use this file.

# Project name shown on CDash
set(CTEST_PROJECT_NAME "Teem")

# Define when the "nightly" dashboard day starts
set(CTEST_NIGHTLY_START_TIME "05:00:00 UTC")

# Dashboard server information
# If you don’t use CDash yet, you can leave these as-is or comment them out.
set(CTEST_DROP_METHOD "https")
set(CTEST_DROP_SITE "my.cdash.org")
set(CTEST_DROP_LOCATION "/submit.php?project=Teem")
set(CTEST_DROP_SITE_CDASH TRUE)
