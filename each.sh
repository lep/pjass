#!/usr/bin/env bash

[[ -n "$VERBOSE" ]] && echo "$0 $@"

expected=$(for f in "$@"; do ./pjass "$f"; done | grep -v -e '<total>' -e ' total$')
actual=$(./pjass --each "$@" | grep -v -e '<total>' -e ' total$')

if [[ "$expected" != "$actual" ]]; then
	echo "--each differs from checking the files one by one"
	if [[ "$VERBOSE" ]]; then
		diff <(echo "$expected") <(echo "$actual")
	fi
	exit 1
fi
