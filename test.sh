#!/bin/bash

webserver/bin/qdhttp &
thePID=$!
cd unittest
rm -rf results/run1
./check.sh run1
CODE=$?
kill $thePID
exit $CODE
