#!/usr/bin/env bash

[[ -n "$VERBOSE" ]] && echo "$0 $1"

if [[ "$MAPSCRIPT" ]]; then
    msg=$(./pjass tests/common.j tests/Blizzard.j "$1" )
    ret=$?
else
    msg=$(./pjass "$1")
    ret=$?
fi


if [[ "$PROF" ]]; then
	gprof pjass.exe gmon.out > "$1-analysis.txt"
fi
if [[ "$ret" == 1 ]]; then
	echo "Error in file '$1', but there should be none"
	if [[ "$VERBOSE" ]]; then
		echo "pjass output:"
		echo $msg
	fi
	exit 1
fi
