# `hest`: command-line parsing

## Intro

The purpose of `hest` is to bridge the `int argc`, `char *argv[]` command-line arguments and a set of C variables that need to be set for a C program to run. The variables can be of most any type (boolean, `int`, `float`, `char *` strings, or user-defined types), and the variables can hold single values (such as `float thresh`) or multiple values (such as `float RGBA[4]`).

`hest` was created in 2002 out of frustration with how limiting other C command-line parsing libraries were, and has become essential for the utility of tools like `unu`. To the extent that `hest` bridges the interactive command-line with compiled C code, it has taken on some of the roles that in other contexts are served by scripting languages with C extensions. The `hest` code was revisited in 2023 to add long-overdue support for `--help`, and to add typed functions for specifying options like `hestOptAdd_4_Float`. Re-writing the code in 2025 finally fixed long-standing bugs with how quoted strings were handled and how response files were parsed, and to add `-{`, `}-` comments.

## Terminology and concepts

`hest` has possibly non-standard terminology for the elements of command-line parsing. Here is a bottom-up description of the command-line and what `hest` can do with it:

- What `main()` gets as `char *argv[]` is the vector of _arguments_ or _args_; each one is a `char*` string. An arg can contain spaces and other arbitrary characters if the user quoted strings or escaped characters; that is between the user and shell (the shell is responsible for taking the command-line and tokenizing it into `char *argv[]`).
- Arguments like `-v` and `-size`, which identify the variable to be set, are called _flags_.
- Some flags are really just flags; no further information is given beyond their presence or absence. Other flags introduce subsequent arguments that together supply information for setting one variable.
- The set of arguments that logically belong together (often following a flag) in the service of setting a variable are called _parameters_ (or _parms_). There is some slippage of terminology between the `char *` string that communicates the parameter, and the value (such an `int`) parsed from the parameter string.
- Separately, and possibly confusingly, `hest`'s behavior has many knobs and controls, stored in the `hestParm` struct. The pointer-to-struct is always named `hparm` in the code, to try to distinguish it from the parameters appearing on the command-line.
- An _option_ determines how to set one C variable. In the C code, one `hestOpt` struct stores everything about how to parse one option, _and_ the results of that parsing. An array of `hestOpt` structs (not pointers to structs) is how a `hest`-using program communicates what it wants to learn from the command-line. The `hestOpt` array is usually built up by calls to one of the `hestOptAdd` functions.
- On the command-line, the option may be defined by a flag and its associated parms; this is a _flagged_ option. Options may also be _unflagged_, or what others call "positional" arguments, because which C variable is set by parsing that option is disambiguated by the option's position on the command-line, and the corresponding ordering of `hestOpt` structs.
- The typical way of using `hest` is to process _all_ the args in the `argv` you give it. In this way `hest` is more like Python's `argparse` that tries to make sense of the entire command-line, rather than, say, POSIX `getopt` which sees some parts of `argv` as flag-prefixed options but the rest as "operands". `hest` doesn't know what an operand is, and tries to slot every `argv` element into an argument of some (possibly unflagged) option.
- An option may have no parms, one parm, a fixed number of parms, or a variable number of parms. Unflagged options must have one or more parms. With `mv *.txt dir`, the `*.txt` filenames could be parsed as a variable number of parms for an unflagged option, and `dir` would be a fixed single parm for a second unflagged option. Flagged options can appear in any order on the command-line, and the same option can be repeated: later appearances over-ride earlier appearances.
- Sometimes multiple command-line options need to be saved and re-used together, over a time span longer than one shell or any variables set it. Command-line options can thus be stored in _response files_, and the contents of response files effecively expanded into the command-line. Response files can have comments, and response files can name other response files.
- The main `hest` function that does the parsing is `hestParse`. Its job is to set one variable (which may have multiple components) for every `hestOpt`. Information for setting each variable can come from the command-line, or from the default string set in the `hestOpt`, but it has to come from somewhere. Essentially, if no default string is given, then the option _must_ be set on the command-line (or a response file named there). In this sense, `hest`'s "options" are badly named, because they are not really optional.

Note that `hest` does not attempt to follow POSIX conventions (or terminology) for command-line descriptions, because those conventions don't empower the kind of expressivity and flexibility that motivated `hest`'s creation. POSIX does not encompass the scientific computing and visualization contexts that Teem was built for.

## The different `kind`s of options, and how to `hestOptAdd` them.

There are lot of moving pieces inside `hestParse`, and the description of how it works is complicated by how flexible a `hestOpt` can be. Two of the fields in the `hestOpt` are `min` and `max`: the min and max number of parameters that may be parsed for that option. All the different traditional uses of the command-line can be parameterized in terms of `min` and `max`, but the full range of possibilities of `min`,`max` (which `hest` supports) include some less conventional uses. Once the possibility of mapping out all possibilities for command-line options in terms of `min` and `max` was recognized, `hest` implementation was organized around that, even if typical uses of `hest` are not thought of that way. See also the **concrete examples** below.

The _`kind`_ is `hest`'s term for a numeric identifier for the kind of option that a `hestOpt` describes. The following ASCII-art illustrates how `min` and `max` determine:

- numeric `kind`, shown as `(`_n_`)`; for _n_ = 1,2,3,4,5
- the prose name for that kind of option, which appears in the code and a description of its operation.
- Which `hestOptAdd_` function or family of functions is used to parse that kind of option. Here, `hestOptAdd_` is abbreviated `hOA_` and the final `_T` stands for the type (eg. `_Bool`, `_Int`, `_Float`, `_Enum`, `_Other`, etc).

```
    |              .                      /       .       /
    |              .                   /       .       /
    |              .                /       .       /
    |       (5) multiple           | (3) multiple
 2--|          variable            |    fixed
    |           parms              |    parms
    |         hOA_Nv_T             |  hOA_{2,3,4,N}_T
    |.............................../
    |  (4) single   | (2) single
 1--|    variable   |    fixed
    |      parm     |    parm
    |    hOA_1v_T   |   hOA_1_T
    |...............|/
    | (1) stand-alone
 0--|      flag;
    |    no parms
    |    hOA_Flag
 ^  |/_____________________________________________
max          |              |              |
    min >    0              1              2
```

The `kind` of option is independent of whether it is flagged or unflagged, and independent of being optional (due to the `hestOpt` having a default string) versus required (when no default is given). Note that users of `hest` may not ever need to worry about `kind`, and it certainly is not part of the API calls to create options and parse command-lines.

Some **concrete examples** may be helpful for understanding `hest`'s utility ... (IN PROGRESS) ...
... Give some specific examples, flagged and unflagged ...
.. show a response file being used, show -{ }- commenting

## Limits on what may be parsed, and the `--` flag

If there are no variable parameter options (i.e. limiting oneself to kinds 1, 2, and 3), then there is no limit on options may be used, including the intermixing of flagged and unflagged options, because there is then zero ambiguity about how to attribute a given command-line argument to the parameters of some option.

Things became more complicated with variable parameter options. ... two unflagged variable options wouldn't make sense ...

In [POSIX command-lines](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html#tag_12_01), the elements of `argv` can be first "options" and then "operands", where "options" are
indicated by something starting with `-`, and may have 0 or more "option-arguments" (what `hest` calls parameters). Then, according to Guideline 10:

> The first -- argument that is not an option-argument should be accepted as a
> delimiter indicating the end of options. Any following arguments should be treated
> as operands, even if they begin with the `-` character.
> So `--` marks the end of some "option-arguments".

Even though, as noted above, `hest` itself does not traffic in "operands", and is _not_ currently compliant with the POSIX behavior just quoted, it does make _limited_ use of the `--` flag.

The limits in using variable parameter options are:

- There can be only one _unflagged_ multiple variable parameter option (kind 5). Having more than one creates ambiguity about which option consumes which arguments, and even though `--` could be used for demarcating them, this seems fragile and has not been implemented.
- ... The `--` flag indicates the explicit end of a _flagged_ variable parameter options.
