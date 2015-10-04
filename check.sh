#!/usr/bin/env bash

msg=$(./pjass ../pjass-tests/common.j ../pjass-tests/Blizzard.j "$1" )
if [[ "$?" == 1 ]]; then
	echo "Error in file '$1', but there should be none"
	echo "pjass output:"
	echo $msg
	exit 1
fi
