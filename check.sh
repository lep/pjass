#!/usr/bin/env bash

msg=$(./pjass tests/common.j tests/Blizzard.j "$1" )
if [[ "$?" == 1 ]]; then
	echo "Error in file '$1', but there should be none"
	if [[ "$VERBOSE" ]]; then
		echo "pjass output:"
		echo $msg
	fi
	exit 1
fi
