#!/usr/bin/perl -w

# GK uses this to process text output to be used in <blockquote><pre>
# settings.  Currently, just replaces angle brackets with the their
# equivalent character sequences.

while (<>) {
    chop;
    s/</&lt;/g;
    s/>/&gt;/g;
    print;
}
