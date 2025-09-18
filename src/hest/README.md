# `hest`: command-line parsing

## Intro

The purpose of `hest` is to bridge the `int argc`, `char *argv[]` command-line arguments and a set of C variables that need to be set for a C program to run. The variables can be of most any type (boolean, `int`, `float`, `char *` strings, or user-defined types), and the variables can hold single values (such as `float thresh`) or multiple values (such as `float RGBA[4]`).

`hest` was created in 2002 out of frustration with how limiting other C command-line parsing libraries were, and has become essential for the utility of tools like `unu`. To the extent that `hest` bridges the interactive command-line with compiled C code, it has taken on some of the roles that in other contexts are served by scripting languages with C extensions. The `hest` code was revisited in 2023 to add long-overdue support for `--help`, and to add typed functions for specifying options like `hestOptAdd_4_Float`. Re-revisiting the code in 2025 finally fixed long-standing bugs with how quoted strings were handled and how response files were parsed, and to add `-{`, `}-` comments.

## Terminology and concepts

`hest` has possibly non-standard terminology for the elements of command-line parsing. Here is a bottom-up description of the command-line and what `hest` can do with it:

- What `main()` gets as `char *argv[]` is the vector of _arguments_ or _args_; each one is a `char*` string. An arg can contain spaces and other arbitrary characters if the user quoted strings or escaped characters; that is between the user and shell (the shell is responsible for taking the command-line and tokenizing it into `char *argv[]`). `hest` processes all the args in the `argv` you give it.
- Arguments like `-v` and `-size`, which identify the variable to be set, are called _flags_.
- Some flags are really just flags; no further information is given beyond their presence or absence. Other flags introduce subsequent arguments that together supply information for setting one variable.
- The set of arguments that logically belong together (often following a flag) in the service of setting a variable are called _parameters_ (or _parms_). There is some slippage of terminology between the `char *` string that communicates the parameter, and the value (such an `int`) parsed from the parameter string.
- Separately, and possibly confusingly, `hest`'s behavior has many knobs and controls, stored in the `hestParm` struct. The pointer-to-struct is always named `hparm` in the code, to try to distinguish it from the parameters appearing on the command-line.
- An _option_ determines how to set one C variable. In the C code, one `hestOpt` struct stores everything about how to parse one option, _and_ the results of that parsing. An array of `hestOpt` structs (not pointers to structs) is how a `hest`-using program communicates what it wants to learn from the command-line. The `hestOpt` array is usually built up by calls to one of the `hestOptAdd` functions.
- On the command-line, the option may be defined by a flag and its associated parms; this is a _flagged_ option. Options may also be _unflagged_, or what others call "positional" arguments, because which C variable is set by parsing that option is disambiguated by the option's position on the command-line, and the corresponding ordering of `hestOpt` structs.
- An option may have no parms, one parm, a fixed number of parms, or a variable number of parms. Unflagged options must have one or more parms. With `mv *.txt dir`, the `*.txt` filenames could be parsed as a variable number of parms for an unflagged option, and `dir` would be a fixed single parm for a second unflagged option. Flagged options can appear in any order on the command-line, and the same option can be repeated: later appearances over-ride earlier appearances.
- Sometimes multiple command-line options need to be saved and re-used together, over a time span longer than one shell or any variables set it. Command-line options can thus be stored in _response files_, and the contents of response files effecively expanded into the command-line. Response files can have comments, and response files can name other response files.
- The main `hest` function that does the parsing is `hestParse`. Its job is to set one variable (which may have multiple components) for every `hestOpt`. Information for setting each variable can come from the command-line, or from the default string set in the `hestOpt`, but it has to come from somewhere. Essentially, if no default string is given, then the option _must_ be set on the command-line (or a response file named there). In this sense, `hest`'s "options" are badly named, because they are not really optional.

Note that `hest` does not attempt to follow POSIX conventions (or terminology) for command-line descriptions, because those conventions don't empower the kind of expressivity and flexibility that motivated `hest`'s creation. POSIX does not encompass the scientific computing and visualization contexts that Teem was built for.

## The different `kind`s of options, and how to `hestOptAdd` them.

There are lot of moving pieces inside `hestParse`, and the description of how it works is complicated by how flexible a `hestOpt` can be. Two of the fields in the `hestOpt` are `min` and `max`: the min and max number of parameters that may be parsed for that option. All the different traditional uses of the command-line can be parameterized in terms of `min` and `max`, but the full range of possibilities of `min`,`max` (which `hest` supports) include some less conventional uses. The _`kind`_ is `hest`'s term for a numeric identifier for the kind of option that a `hestOpt` describes. The following ASCII-art illustrates how `min` and `max` determine:

- numeric `kind`, shown as `(`_n_`)`; for _n_ = 1,2,3,4,5
- the prose name for that kind of option, which appears in the code and a description of its operation.
- Which `hestOptAdd_` function or family of functions is used to parse that kind of option. Here, `hestOptAdd_` is abbreviated `hOA_` and the final `_T` stands for the type (eg. `_Bool`, `_Int`, `_Float`, `_Enum`, `_Other`, etc).

```
    |              .                    /       .       /
    |              .                  /       .       /
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

The `kind` of option is independent of whether it is flagged or unflagged, and independent of being optional (because the `hestOpt` has a default string) or required (because no default is given).
