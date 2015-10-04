#!/usr/bin/env bash

./pjass ../pjass-tests/common.j ../pjass-tests/Blizzard.j "$1" > /dev/null
if [[ "$?" == 1 ]]; then
	echo "Error in file '$1', but there should be none"
	exit 1
fi
