#!/usr/bin/perl 


$emit = 1;
while (<>) {

    # nix carraige returns
    s///g;
    
    # nix pragmas
    s/\#pragma warning.+$//g;

    # process all assignments to globals to parse out variables and values
    if (/w[pm][0-9][a-z] = /) {
	chomp;         # nix newline
	s/ //g;        # nix spaces 
	s/;$//g;       # nix last semicolon
	foreach $pair (split /;/) {
	    ($var, $val) = split /=/, $pair;
	    $$var = $val;
	}
	next;
    }

    # these lines nix the declaration of the wm* and wp* globals
    s/float  wm4h, wm4g, wm4f, wm4e, wm4d, wm4c, wm4b, wm4a;//;
    s/float  wm3h, wm3g, wm3f, wm3e, wm3d, wm3c, wm3b, wm3a;//;
    s/float  wm2h, wm2g, wm2f, wm2e, wm2d, wm2c, wm2b, wm2a;//;
    s/float  wm1h, wm1g, wm1f, wm1e, wm1d, wm1c, wm1b, wm1a;//;
    s/float  wm0h, wm0g, wm0f, wm0e, wm0d, wm0c, wm0b, wm0a;//;
    s/float  wp1h, wp1g, wp1f, wp1e, wp1d, wp1c, wp1b, wp1a;//;
    s/float  wp2h, wp2g, wp2f, wp2e, wp2d, wp2c, wp2b, wp2a;//;
    s/float  wp3h, wp3g, wp3f, wp3e, wp3d, wp3c, wp3b, wp3a;//;
    
    # this is how the variables values are folded into the expressions
    s/wm4h/$wm4h/g; s/wm4g/$wm4g/g; s/wm4f/$wm4f/g; s/wm4e/$wm4e/g;
    s/wm4d/$wm4d/g; s/wm4c/$wm4c/g; s/wm4b/$wm4b/g; s/wm4a/$wm4a/g;
    s/wm3h/$wm3h/g; s/wm3g/$wm3g/g; s/wm3f/$wm3f/g; s/wm3e/$wm3e/g;
    s/wm3d/$wm3d/g; s/wm3c/$wm3c/g; s/wm3b/$wm3b/g; s/wm3a/$wm3a/g;
    s/wm2h/$wm2h/g; s/wm2g/$wm2g/g; s/wm2f/$wm2f/g; s/wm2e/$wm2e/g;
    s/wm2d/$wm2d/g; s/wm2c/$wm2c/g; s/wm2b/$wm2b/g; s/wm2a/$wm2a/g;
    s/wm1h/$wm1h/g; s/wm1g/$wm1g/g; s/wm1f/$wm1f/g; s/wm1e/$wm1e/g;
    s/wm1d/$wm1d/g; s/wm1c/$wm1c/g; s/wm1b/$wm1b/g; s/wm1a/$wm1a/g;
    s/wm0h/$wm0h/g; s/wm0g/$wm0g/g; s/wm0f/$wm0f/g; s/wm0e/$wm0e/g;
    s/wm0d/$wm0d/g; s/wm0c/$wm0c/g; s/wm0b/$wm0b/g; s/wm0a/$wm0a/g;
    s/wp1h/$wp1h/g; s/wp1g/$wp1g/g; s/wp1f/$wp1f/g; s/wp1e/$wp1e/g;
    s/wp1d/$wp1d/g; s/wp1c/$wp1c/g; s/wp1b/$wp1b/g; s/wp1a/$wp1a/g;
    s/wp2h/$wp2h/g; s/wp2g/$wp2g/g; s/wp2f/$wp2f/g; s/wp2e/$wp2e/g;
    s/wp2d/$wp2d/g; s/wp2c/$wp2c/g; s/wp2b/$wp2b/g; s/wp2a/$wp2a/g;
    s/wp3h/$wp3h/g; s/wp3g/$wp3g/g; s/wp3f/$wp3f/g; s/wp3e/$wp3e/g;
    s/wp3d/$wp3d/g; s/wp3c/$wp3c/g; s/wp3b/$wp3b/g; s/wp3a/$wp3a/g;

    # nix the minimal function body before and after the switch lines
    s/  float result;//g;
    s/  int i;//g;
    s/  i = \(t\<0\) \? \(int\)t-1:\(int\)t;//g;
    s/  t = t - i;//g;
    s/  switch \(i\) \{//g;
    s/  \}//g;
    s/  return result;//g;

    # print lines that aren't all whitespace
    s/^ +$//g;
    if (!m/^$/) {
	print;
    }
}
