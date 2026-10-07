#!/usr/bin/env bash

[[ -n "$VERBOSE" ]] && echo "$0 $@"

prelude=tests/each/prelude.j
normalize() { grep -Fv "$prelude" | grep -v -e '<total>' -e ' total$'; }

compare() {
	local expected= actual output expected_status=0 actual_status f
	for f in "$@"; do
		output=$(./pjass "$prelude" "$f") || expected_status=1
		expected+="$output"$'\n'
	done
	actual=$(./pjass "$prelude" --each "$@")
	actual_status=$?
	expected=$(printf '%s' "$expected" | normalize)
	actual=$(printf '%s\n' "$actual" | normalize)
	if [[ "$expected" != "$actual" || "$expected_status" != "$actual_status" ]]; then
		echo "--each differs from checking the files one by one (exit $actual_status, expected $expected_status)"
		[[ "$VERBOSE" ]] && diff <(printf '%s\n' "$expected") <(printf '%s\n' "$actual")
		exit 1
	fi
}

compare tests/each/a-defines-f.j tests/each/b-defines-f-again.j
compare "$@"
output=$(./pjass "$prelude" --each - tests/each/b-defines-f-again.j < tests/each/a-defines-f.j)
[[ $? == 0 && "$output" == *'<stdin>'* ]] || exit 1
# Both command-line orders and source annotations must reject hash checking.
for args in '+checkstringhash --each' '--each +checkstringhash'; do
	output=$(./pjass $args tests/each/a-defines-f.j 2>&1)
	[[ $? == 1 && "$output" == *'--each cannot be combined with +checkstringhash'* ]] || exit 1
done
output=$(./pjass "$prelude" --each tests/each/hash-annotation.j 2>&1)
[[ $? == 1 && "$output" == *'--each cannot be combined with +checkstringhash'* ]]
