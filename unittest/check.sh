#!/bin/sh

SRC=resources
RES=results

PLOT=ab.p
GP=ab_plot
AR=ab_results

ZOMBIE=zombie

GF=get_full
HF=head_full

NF=not_found


PORT=80

if [ $# -ne 1 ]; then
	echo "usage: $0 <run-name> (port)"
	exit 1
fi

if [ $# -ge 2 ]; then
		PORT=$2
fi

mkdir $RES/$1

printf "%b" "$(cat $SRC/$GF)\r\n" | nc -q 10 localhost $PORT > $RES/$1/$GF
printf "%b" "$(cat $SRC/$HF)\r\n" | nc -q 10 localhost $PORT > $RES/$1/$HF

printf "%b" "$(cat $SRC/$NF)\r\n" | nc -q 10 localhost $PORT > $RES/$1/$NF

# check concurrent request handling
ab -c 50 -n 1000 -g $RES/$1/$GP localhost:$PORT/index.html > $RES/$1/$AR
cp $SRC/$PLOT $RES/$1/$PLOT
sed -i s/PATH/$1/g $RES/$1/$PLOT
gnuplot $RES/$1/$PLOT

# check for zombie processes
ps aux | awk '"[Zz]" ~ $8 { printf("%s, %s, PID = %d\n", $11, $8, $2); }' > $RES/$1/$ZOMBIE


echo "\n"

./parser.py $RES/$1/$GF
./parser.py $RES/$1/$HF
./parser.py $RES/$1/$NF

echo "\n"

cat $RES/$1/$ZOMBIE | sort





