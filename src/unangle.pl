#!/usr/bin/perl -w

# GK uses this to process text output to be used in <blockquote><pre>
# settings.  Currently, just replaces angle brackets with the their
# equivalent character sequences.

$line = 0;
while (<>) {
    s/</&lt;/g;
    s/>/&gt;/g;
    s/  [\b][\b]//g;
    if ($line) {
	print;
    }
    $line += 1;
}
