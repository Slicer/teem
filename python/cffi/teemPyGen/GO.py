#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

verbose = 0

# (TEEM_LIB_LIST)
libs = ['air', 'hest', 'biff', 'nrrd', 'ell', 'unrrdu', 'alan', 'moss', 'tijk',
        'gage', 'dye', 'bane', 'limn', 'echo', 'hoover', 'seek', 'ten', 'elf',
        'pull', 'coil', 'push', 'mite', 'meet']

def check_path(sPath):
    if (not path.isdir(sPath)):
        raise Exception(f'source path {sPath} not a directory')
    missingSrc = filter(lambda L: not path.isdir(f'{sPath}/{L}'), libs)
    if (len(missingSrc) < len(libs):
        raise Exception(f'missing source dir for {missingSrc} library')

def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(description='Utility for generating the teem.py '
                                     'wrapper around the _teem.py CFFI-based '
                                     'python3 module around shared libteem library')
    parser.add_argument('-v', metavar='verbosity', type=int, default=1, required=False,
                        help='verbosity level (0 for silent)')
    parser.add_argument('source_path',
                        help='path of the Teem *source* (not the install dir)')
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    verbose = args.v
    build(args.source_path)
