#!/usr/bin/perl -w

print "\n\#include \"nrrd.h\"\n\n";

# generate a stub kernel for when the user incorrectly indexes
# into ef == 0, which is undefined; ef must be >= 1
print "double\n";
print "_nrrd_TMFBAD_Int(double *parm) {\n";
print "  fprintf\(stderr, \"_nrrd_TMFBAD: Invalid TMF indexing: ef == 0\\n\"\);\n";
print "  return 0.0;\n";
print "}\n\n";
print "double\n";
print "_nrrd_TMFBAD_Sup(double *parm) {\n";
print "  fprintf\(stderr, \"_nrrd_TMFBAD: Invalid TMF indexing: ef == 0\\n\"\);\n";
print "  return 0.0;\n";
print "}\n\n";
print "double\n";
print "_nrrd_TMFBAD_1_d(double x, double *parm) {\n";
print "  fprintf\(stderr, \"_nrrd_TMFBAD: Invalid TMF indexing: ef == 0\\n\"\);\n";
print "  return 0.0;\n";
print "}\n\n";
print "float\n";
print "_nrrd_TMFBAD_1_f(float x, double *parm) {\n";
print "  fprintf\(stderr, \"_nrrd_TMFBAD: Invalid TMF indexing: ef == 0\\n\"\);\n";
print "  return 0.0;\n";
print "}\n\n";
print "void\n";
print "_nrrd_TMFBAD_N_d(double *f, double *x, size_t len, double *parm) {\n";
print "  fprintf\(stderr, \"_nrrd_TMFBAD: Invalid TMF indexing: ef == 0\\n\"\);\n";
print "}\n\n";
print "void\n";
print "_nrrd_TMFBAD_N_f(float *f, float *x, size_t len, double *parm) {\n";
print "  fprintf\(stderr, \"_nrrd_TMFBAD: Invalid TMF indexing: ef == 0\\n\"\);\n";
print "}\n\n";
print "NrrdKernel\n";
print "_nrrdKernel_TMFBAD = {\n";
print "  \"TMFBAD\",\n";
print "  1, _nrrd_TMFBAD_Sup, _nrrd_TMFBAD_Int,\n";
print "  _nrrd_TMFBAD_1_f, _nrrd_TMFBAD_N_f,\n";
print "  _nrrd_TMFBAD_1_d, _nrrd_TMFBAD_N_d\n";
print "};\n";
print "NrrdKernel *\n";
print "nrrdKernel_TMFBAD = &_nrrdKernel_TMFBAD;\n\n";

# generate hash of nrrdKernels that have already been declared 
# ("done") and has of nrrdKernels that must be declared AFTER their
# dependant kernel has been declared ("post")
%done = ();
%post = ();

$maxD = 0;
$maxC = 0;
$maxA = 0;
# process body of file
while (<>) {
    if (/\#define/) {
	print;
	next;
    }
    if (/float (d[012]_c[n0123]_[1234]ef)\(float a/) {
	$kern = "TMF_$1";
	if ($_ =~ m/d([012])_c([0123])_([1234])ef/) {
	    $maxD = $1 > $maxD ? $1 : $maxD;
	    $maxC = $2 > $maxC ? $2 : $maxC;
	    $maxA = $3 > $maxA ? $3 : $maxA;
	}
	print "\n/* ------------------------ $kern --------------------- */\n\n";
	print "\#define ${kern}(a, i, t) ( \\\n";
	while (<>) {
	    # for when the filter is just a wrapper around another filter
	    if (/return (d[012]_c[n0123]_[1234]ef)\(a, t\)/) {
		$depk = "TMF_$1";
		# we don't need the #define, but at this point we need to finish it
		print "  ${depk}(a, i, t))\n\n";
		# create string to represent kernel definition;
		$def = ("NrrdKernel\n" .
			"_nrrdKernel_${kern} = {\n" .
			"  \"${kern}\",\n" .
			"  1, _nrrd_${depk}_Sup, _nrrd_${depk}_Int,\n" .
			"  _nrrd_${depk}_1_f,  _nrrd_${depk}_N_f,\n" .
			"  _nrrd_${depk}_1_d,  _nrrd_${depk}_N_d\n" .
			"};\n\n");
		if (exists $done{$depk}) {
		    print "/* exists done{$depk} */\n";
		    print "$def";
		}
		else {
		    if (not exists $post{$depk}) {
			$post{$depk} = "";
		    }
		    $post{$depk} .= $def;
		}
		last;
	    }

	    # fix typos
	    s/result = result = /result = /g;
	    s/case 1: 1-t/case 1: result = 1-t/g;

	    # process the switch cases
	    s/case ([0-9]): +result *= *(.+); +break;$/\(i == $1 ? $2 : \\/g;
	    $n = $1;
	    $sup = ($n + 1)/2;
	    
	    # when we've reached the end of the switch cases
	    if (/default: result = 0;/) {
		# the default case
		print "  0";

		# print one end paren for each of the cases
		for ($i=0; $i <= $n; $i++) {
		    print ")";
		}
		
		# and one more end paren to finish the #define
		print ")\n\n";

		# and now define the C functions ...
		# integral
		print "double\n";
		print "_nrrd_${kern}_Int\(double *parm\) {\n";
		if ($kern =~ m/_d0_/) {
		    # not a derivative (zero-eth derivative)
		    print "  return 1.0;\n";
		} else {
		    # is some sort of derivative
		    print "  return 0.0;\n";
		}
	        print "}\n\n";

		# support
		print "double\n";
		print "_nrrd_${kern}_Sup\(double *parm\) {\n";
		print "  return $sup;\n";
	        print "}\n\n";

		# 1_d
		print "double\n";
		print "_nrrd_${kern}_1_d\(double x, double *parm\) {\n";
		print "  int i;\n\n";
		print "  x += $sup;\n";
		print "  i = (x<0) ? x-1 : x;\n";
		print "  x -= i;\n";
		print "  return ${kern}\(parm[0], i, x\);\n";
		print "}\n\n";

		# 1_f
		print "float\n";
		print "_nrrd_${kern}_1_f\(float x, double *parm\) {\n";
		print "  int i;\n\n";
		print "  x += $sup;\n";
		print "  i = (x<0) ? x-1 : x;\n";
		print "  x -= i;\n";
		print "  return ${kern}\(parm[0], i, x\);\n";
		print "}\n\n";

		# N_d
		print "void\n";
		print "_nrrd_${kern}_N_d(double *f, double *x, size_t len, double *parm) {\n";
		print "  double t;\n";
		print "  size_t I;\n";
		print "  int i;\n\n";
		print "  for \(I=0; I<len; I++\) {\n";
		print "    t = x[I] + $sup;\n";
		print "    i = (t<0) ? t-1 : t;\n";
		print "    t -= i;\n";
		print "    f[I] = ${kern}\(parm[0], i, t\);\n";
		print "  }\n";
		print "}\n\n";

		# N_f
		print "void\n";
		print "_nrrd_${kern}_N_f(float *f, float *x, size_t len, double *parm) {\n";
		print "  float t;\n";
		print "  size_t I;\n";
		print "  int i;\n\n";
		print "  for \(I=0; I<len; I++\) {\n";
		print "    t = x[I] + $sup;\n";
		print "    i = (t<0) ? t-1 : t;\n";
		print "    t -= i;\n";
		print "    f[I] = ${kern}\(parm[0], i, t\);\n";
		print "  }\n";
		print "}\n\n";

		# nrrdKernel
		print "NrrdKernel\n";
		print "_nrrdKernel_${kern} = {\n";
		print "  \"${kern}\",\n";
		print "  1, _nrrd_${kern}_Sup, _nrrd_${kern}_Int,\n";
		print "  _nrrd_${kern}_1_f,  _nrrd_${kern}_N_f,\n";
		print "  _nrrd_${kern}_1_d,  _nrrd_${kern}_N_d\n";
		print "};\n";

		$done{$kern} = True;
		# any kernels which depend on this one
		if (exists $post{$kern}) {
		    print "/* exists post{$kern} */\n";
		    print "$post{$kern}";
		}

		last;
	    }

	    print;
	}
    }
}

# generate 3-D array of all TMFs
print "\nNrrdKernel *\n";

if (2 != $maxD) {
    print "teem/src/nrrd/tmf/fix2.pl error: maxD = $maxD, not 2\n";
    exit;
}
if (3 != $maxC) {
    print "teem/src/nrrd/tmf/fix2.pl error: maxC = $maxC, not 3\n";
    exit;
}
if (4 != $maxA) {
    print "teem/src/nrrd/tmf/fix2.pl error: maxA = $maxA, not 4\n";
    exit;
}
print "nrrdKernelTMF[3][4][5] = {\n";
for ($d=0; $d<=2; $d++) {
    print "  {            /* d = $d */ \n";
    for ($c=0; $c<=3; $c++) {
	print "    {\n";
	print "       &_nrrdKernel_TMFBAD,\n";
	for ($ef=1; $ef<=4; $ef++) {
	    print "       &_nrrdKernel_TMF_d${d}_c${c}_${ef}ef,\n";
	}
	print "    },\n";
    }
    print "  },\n";
}
print "};\n\n";

print "int nrrdKernelTMF_maxD = $maxD;\n";
print "int nrrdKernelTMF_maxC = $maxC;\n";
print "int nrrdKernelTMF_maxA = $maxA;\n";
