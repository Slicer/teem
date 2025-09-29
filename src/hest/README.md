# `hest`: command-line parsing

## Intro

The purpose of `hest` is to bridge the `int argc`, `char *argv[]` command-line arguments and a set of C variables that need to be set for a C program to run. The variables can be of most any type (boolean, `int`, `float`, `char *` strings, or user-defined types), and the variables can hold single values (such as `float thresh`) or multiple values (such as `float RGBA[4]`).

`hest` was created in 2002 out of frustration with how limited other C command-line parsing libraries were, and has become essential for the utility of tools like `unu`. To the extent that `hest` bridges the interactive command-line with compiled C code, it approaches some of the roles that in other contexts are served by scripting languages with C extensions. The `hest` code was revisited in 2023 to add long-overdue support for `--help`, and to add typed functions for specifying options like `hestOptAdd_4_Float`. Re-writing the code in 2025 finally fixed long-standing bugs with how quoted strings were handled and how response files were parsed, and to add `-{`, `}-` comments.

`hest` is powerful and not simple. This note attempts to give a technical description useful for someone thinking about using `hest`, as well as anyone trying wrap their head around the `hest` source code, including its author.

First, some examples ...

## Terminology and concepts

`hest` has possibly non-standard terminology for the elements of command-line parsing. Here is a bottom-up description of the command-line and what `hest` can do with it:

- What `main()` gets as `char *argv[]` is the vector of _arguments_ or _args_; each one is a `char*` string. An arg can contain spaces and other arbitrary characters if the user quoted strings or escaped characters; that is between the user and shell (the shell is responsible for taking the command-line and tokenizing it into `char *argv[]`).
- Arguments like `-v` and `-size`, which identify the variable to be set, are called _flags_.
- Some flags are really just flags; no further information is given beyond their presence or absence. Other flags introduce subsequent arguments that together supply information for setting one variable.
- The sub-sequence of arguments that logically belong together (often following a flag) in the service of setting a variable are called _parameters_ (or _parms_). There is some slippage of terminology between the `char *` string that communicates the parameter, and the value (such an `int`) parsed from the parameter string.
- Separately, and possibly confusingly, `hest`'s behavior has many knobs and controls, stored in the `hestParm` struct. The pointer-to-struct is always named `hparm` in the code, to try to distinguish it from the parameters appearing on the command-line.
- One _option_ determines how to set one C variable. In the C code, one `hestOpt` struct stores everything about how to parse one option, _and_ intermediate state during the parsing process, _and_ the final results of that parsing. An _array_ of `hestOpt` structs (which we can call an _option list_) is how a `hest`-using program communicates what it wants to learn from the command-line, and how to interpret the contents of the command-line. The `hestOpt` array is usually built up by calls to one of the `hestOptAdd` functions.
- On the command-line, the option may be communicated by a flag (e.g. `-sz`) and its associated parms (e.g. `3 640 480`); this is a _flagged_ option. Options may also be _unflagged_, or what others call "positional" arguments, because determining which option a given argument belongs to is disambiguated by where that option's `hestOpt` struct occurs with the option list.
- An option may have no parms, one parm, a fixed number of parms, or a variable number of parms; `hest` calls these _variadic_ options to separate the description of the options from the information (the C _variable_) that the option describes. Unflagged options must have one or more parms. With `mv *.txt dir`, the `*.txt` filenames could be parsed as the variadic parms for an unflagged option, and `dir` would be a fixed single parm for a second unflagged option.
- Sometimes multiple command-line options need to be saved and re-used together, over a time span longer than any one shell. Command-line options can thus be stored in _response files_, and the contents of response files effecively expanded into the command-line. Response files can have comments (from `#` to end of line, just like shell scripts), and response files can in turn name other response files.
- The main `hest` function that does the parsing is `hestParse`. Its job is to set one variable (which may have multiple components) for every `hestOpt`. Information for setting each variable can come from the command-line, or from the default string set in the `hestOpt`, but it has to come from somewhere. Essentially, if no default string is given, then the option _must_ be set on the command-line (or a response file named there). In this sense, `hest`'s "options" are badly named, because they are not really optional.

Pre-dating `hest` are the [POSIX conventions for command-line arguments](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html#tag_12_01), wherein the elements of `argv` can be first "options" and then "operands", where "options" are indicated by an initial `-` character, and may have zero or more "option-arguments" (what `hest` calls "parameters"). However, `hest` does not attempt to follow POSIX conventions (or terminology) for command-line usage. In particular, `hest` has no idea of an "operand" while it tries to interpret _every_ the `argv` args as some argument of some (possibly unflagged) option. Disregard for POSIX has not limited `hest`'s expressivity and flexibility, or its utility for the scientific computing and visualization contexts that it was built for.

## The different `kind`s of options, and how to `hestOptAdd` them.

There are lot of moving pieces inside `hestParse`, and the description of how it works is complicated by how flexible a `hestOpt` can be. Two of the most important yet unconventional fields in the `hestOpt` are `min` and `max`: the min and max number of parameters that may be parsed for that option. All the different traditional uses of the command-line can be parameterized in terms of `min` and `max`, but the full range of possibilities of `min`,`max` (which `hest` supports) include unusual use-cases. Once the possibility of mapping out all possibilities for command-line options in terms of `min` and `max` was recognized, `hest` implementation was organized around that, even if typical uses of `hest` are not thought of that way. Refer also to the **concrete examples** above and below.

The _`kind`_ is `hest`'s term for a numeric identifier for the kind of option that a `hestOpt` describes. The following ASCII-art illustrates how `min` and `max` determine:

- numeric `kind`, shown as `(`_n_`)`; for _n_ = 1,2,3,4,5
- the prose name for that kind of option, which appears in the code and a description of its operation.
- Which `hestOptAdd_` function or family of functions is used to parse that kind of option. Here, `hestOptAdd_` is abbreviated `hOA_` and the final `_T` stands for the type (eg. `_Bool`, `_Int`, `_Float`, `_Enum`, `_Other`, etc).

```
    |              .                      /       .       /
    |              .                   /       .       /
    |              .                /       .       /
    |       (5) multiple           | (3) multiple
 2--|          variadic            |    fixed
    |           parms              |    parms
    |         hOA_Nv_T             |  hOA_{2,3,4,N}_T
    |.............................../
    |  (4) single   | (2) single
 1--|    variadic   |    fixed
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

The `kind` of option is mostly independent of whether it is flagged or unflagged, and independent of being optional (due to the `hestOpt` having a default string) versus required (when no default is given). The one wrinkle is that unflagged options must have at least one parameter (i.e. `min` > 0), either by the command-line or via a default string. An unflagged option allowed to have zero parameters has no explicit textual existence, which seems out-of-bounds for a command-line parser. Thus for unflagged options, `kind`s 1 and 4 are ruled out, and kind 5 is possible only with `min` >= 1. This is likely already too much low-level information: users of `hest` will probably never need to worry about `kind`, and certainly `kind` is not part of the API calls to create options and parse command-lines.

More examples may help show `hest`'s utility ... (IN PROGRESS) ...
... Give some specific examples, flagged and unflagged ...
.. show a response file being used, show -{ }- commenting

## The over-all process `hestParse`, its limits, and the `--` flag

Given an `argc`,`argv` command-line and a `hestOpt` array describing the options to parse, `hestParse` must first answer: **which elements of `argv` are associated with which options?** If there are no variadic options (i.e. limiting oneself to kinds 1, 2, and 3), then the answer is straight-forward, even with flagged options being able to appear on the command-line in any order. The option set has some fixed number of slots. The flag arguments for the flagged options, and the position of arguments of unflagged options, implies how to put each `argv` element into each slot.

Things became more complicated with variadic options. Suppose ... two unflagged variadic options wouldn't make sense ...

Understanding how `hest` attributes arguments to options starts with knowing the main phases of `hestParse`:

1. Validate the given `hestOpt` array
1. Convert given `argc`,`argv`, to an internal (`hestArgVec`) representation of the argument vector (called `havec` in the code). This is the phase in which response files are expanded and `-{`,`}-` comments are interpreted.
1. Extract from `havec` all the arguments associated with flagged options: the flag args, plus any immediately subsequent associated parm args. Arguments are transferred to the per-option arg vector within the `hestOpt` struct.
1. What remains in the command-line `havec` should be the concatenation of args associated with the unflagged (positional) options. As long as there is at most one unflagged variadic option, there is an unambigious assignment of arguments to options, so transfer these args to their corresponding option.
1. For any option that did not receive any args from the command-line, tokenize the option's default string and save into its per-option arg vector. Whether by command-line, response file, or default, every option should have the information it needs.
1. Parse the per-option arg vectors (of strings) to set the value(s) of C variable pointed to by each `hestOpt`. Each `hestOpt` also remembers the source of information for setting the variable (command-line vs response-file vs default string).

Fans of [POSIX Guidelines](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html#tag_12_02) may know that Guideline 10 describes the role of `--`:

> The first -- argument that is not an option-argument should be accepted as a delimiter indicating the end of options. Any following arguments should be treated as operands, even if they begin with the `-` character. So `--` marks the end of some "option-arguments".

Even though, as noted above, `hest` itself does not traffic in "operands" or follow POSIX, it does borrow `--` in a limited way to help with phase 3 above: `--` marks the end of arguments for a _flagged_ variadic option, to demarcate them from any arguments intended for _unflagged_ options (variadic or not). In this sense, the `--` mark is more about seperating phases 3 and 4 above, than it is about separating options from operands.

With this context, some further details and limitations of `hest`' processing can be outlined:

- There can be only one _unflagged_ multiple variadic option (kind 5). Having more than one could create ambiguity about which option consumes which arguments, and `hest` currently attempts nothing (not even `--`) to resolve this. There can be, however, more than one _flagged_ multiple variadic options.
- The `--` flag indicates the explicit end of arguments for a _flagged_ variadic option. But the `--` is not necessary: any option flag also marks the end of variadic args, as does the end of the command-line.
- `hestProc` strives to interpret the entire `argc`,`argv` argument vector you give it; there is currently no way to tell `hest` (via `--` or otherwise): "stop processing `argv` here, and leave the rest as operands for something else to interpret".
- Arguments for options (flagged or unflagged) can only come from the user (the `argc`,`argv` command-line or a response file it invokes) or from the option's default string, but not from any mix of both: there is no way to supplement arguments from the user with those from the default string. Though it will probably be more confusing than helpful, options can get arguments from a mix of the command-line and response files, since response files are effectively expanded into the command-line prior to any per-option processing.
- Flagged options can appear in any order on the command-line, and the same option can be repeated: the last appearance over-rides all earlier appearances. `hest` currently cannot remember a list of occurance of repeated options (unlike, say, `sed -e ... -e ... `. )
- By their nature, unflagged options can appear at most once on the command-line, or not at all if they have a default. The attribution of arguments to unflagged options depends on the ordering of the options in the option list (later arguments are always attributed to later options), but arguments are not extracted from the command-line (and moved to the per-option arg vec) if the option has a default. .... forward and back order ...
