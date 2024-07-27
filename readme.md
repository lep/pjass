# pjass

A lightweight and fast Jass2 parser for bison/yacc

Original by Rudi Cilibrasi

And improved by many others

This repo now maintained by lep

# Usage

To use this program, list the files you would like to parse in order.
If you would like to parse from standard input (the keyboard), then
use `-` as an argument.  If you supply no arguments to pjass, it will
parse the console standard input by default.

To test this program:
```
$ pjass common.j common.ai Blizzard.j
```

## Installation for JNGP

If you are using pjass via JNGP replace the `pjass.exe` in `JNGP/jasshelper/`
with the updated one.

## Installation for bundled JassHelper for Patches 1.31+

Blizzard ships their own pjass signed by them so you can't just replace the
.exe-file. But you can still use the new pjass by using `jasshelper.conf`
like this:

    [jasscompiler]
    "pjass-fresh.exe"
    "$COMMONJ $BLIZZARDJ $WAR3MAPJ"

`pjass-fresh.exe` is of course the newly downloaded pjass executable (just put
it next to the original pjass.exe).

## Command line arguments

pjass provides a bunch of command line flags to customize and turn on specific
checks. pjass provides a special system to easily enable and disable features
for specific files. Say you want to enable the "returnbug" but only for
`Blizzard.j`, here is how you would do that:

    $ pjass common.j +rb Blizzard.j -rb war3map.j

So any `+`-flag enables the flag until the next `-`-flag. If you want to have
flag on for all your files just put before any file arguments.

    $ pjass +checkstringhash common.j Blizzard.j war3map.j

You can also (de)activate the flags on a per function base directly in your
.j-file like this:

    //# +rb 
    function H2I takes handle h returns integer
        return h
        return 0
    endfunction


Here is a table of all currently supported flags.
All of these options are **off** by default.


 Flag                  | Description
-----------------------|-------------------
 `rb`                  | When enabled pjass allows the old returnbug usage.
 `filter`              | When enabled pjass checks for functions in `Filter` or `Condition` to return boolean.
 `shadow`              | When enabled this checks if a local variable name is same as a previously defined global variable name.
 `checkglobalsinit`    | When enabled pjass checks for potentially usage of uninitialized global variables.
 `checkstringhash`     | When enabled pjass checks for calls to `StringHash` and reports when two different strings hash to the same integer.
 `noruntimeerror`      | When enabled pjass ignores all runtime errors. Runtime errors are wrong usage of specific natives for example.
 `nosemanticerror`     | When enabled pjass ignores all semantic errors. This is/was used to be able to check the memhack scripts.
 `nosyntaxerror`       | When enabled pjass ignores all syntax errors.
 `nomodulooperator`    | When enabled pjass reports any usage of the modulo (`%`) operator.
 `checklongnames`      | When enabled pjass checks if any name is longer than 3958 characters.
 `checknumberliterals` | When enabled pjass checks if any number literal overflows.
 `oldpatch`            | Enables `+rb`, `+filter`, `+nomodulooperator` and `+checklongnames` at once.

# Building

This uses flex and bison (atleast version 3.0), so install them first.
Then just run GNUMake (e.g. `make` or `gmake`).

You can also run `make help` to get a bunch of options but the other most
important make-target is probably `test` which builds pjass and runs all
supplied tests in `tests/`.

# History

In this repo i tried to capture the whole history of pjass.
For this i imported the cvs repo from sourceforge and went through the
pjass-thread on wc3c. On top of that i added my own modifications.
This probably isn't the real history as for one some attachments on wc3c
were faulty but i guess it's as good as it can get.

# Links

You can find news about this program and updates at
http://hiveworkshop.com/threads/pjass-updates.258738/ and
http://www.wc3c.net/showthread.php?t=75239 and
http://jass.sourceforge.net


