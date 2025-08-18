This directory has header file(s) that are only #include'd as part of building
Teem, but not by anything using Teem.

teemPng.h: ensures that if Teem is being compiled to have PNG file format
support, that it is also being compiled with support for the underlying zlib
compression.
