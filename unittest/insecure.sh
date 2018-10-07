#!/bin/bash

if [ $# -lt 1 ]; then
	echo "usage: $0 <src-folder> (<src-folder2>...)"
	exit 1
fi

OUTPUT="$(grep -Rn "\<strcpy\>" $*)"
if [[ "$OUTPUT" ]]; then
		OUTPUT="strcpy:\n${OUTPUT}"
fi

OUTPUT2="$(grep -Rn "\<strcat\>" $*)"
if [[ "$OUTPUT2" ]]; then
		OUTPUT+="\n\nstrcat:\n${OUTPUT2}"
fi

OUTPUT3="$(grep -Rn "\<sprintf\>" $*)"
if [[ "$OUTPUT3" ]]; then
		OUTPUT+="\n\nsprintf:\n${OUTPUT3}"
fi

OUTPUT4="$(grep -Rn "\<gets\>" $*)"
if [[ "$OUTPUT4" ]]; then
		OUTPUT+="\n\ngets:\n${OUTPUT4}"
fi

OUTPUT5="$(grep -Rn "\<strtok\>" $*)"
if [[ "$OUTPUT5" ]]; then
		OUTPUT+="\n\nstrtok:\n${OUTPUT5}"
fi

OUTPUT6="$(grep -Rn "\<scanf\>" $*)"
if [[ "$OUTPUT6" ]]; then
		OUTPUT+="\n\nscanf:\n${OUTPUT6}"
fi

OUTPUT7="$(grep -Rn "\<itoa\>" $*)"
if [[ "$OUTPUT7" ]]; then
		OUTPUT+="\n\nitoa:\n${OUTPUT7}"
fi

if [[ "$OUTPUT" ]]; then
		echo -e $OUTPUT >&2
		echo >&2
		exit 1
fi
