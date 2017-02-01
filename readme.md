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

If you are using pjass via JNGP replace the `pjass.exe` in `JNGP/jasshelper/`
with the updated one.

Also look at the output of `pjass -h`.

# Building

This uses flex and bison, so install them first.
Then just run GNU-make (e.g. `make` or `gmake`).

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


