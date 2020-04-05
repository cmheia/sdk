#!/usr/bin/env sh

CC=gcc

tool_src=tools/makeimgsource
tool_bin=tools

[ -d $tool_src ] && {
	for s in $(ls $tool_src); do
		echo "building $tool_bin/${s%.*}"
		$CC -O2 -o $tool_bin/${s%.*} $tool_src/$s
	done
}
