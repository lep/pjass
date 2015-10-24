#!/usr/bin/env bash

./pjass tests/common.j tests/Blizzard.j "$1" > /dev/null
if [[ "$?" == 0 ]]; then
	echo "No error in file '$1', but there should be some"
	exit 1
fi
