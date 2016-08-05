#!/usr/bin/env bash

./pjass "$1" > /dev/null
ret=$?
if [[ "$PROF" ]]; then
	gprof pjass.exe gmon.out > "$1-analysis.txt"
fi
if [[ "$ret" == 0 ]]; then
	echo "No error in file '$1', but there should be some"
	exit 1
fi
