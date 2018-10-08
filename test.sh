#!/bin/bash

thePID=`cd webserver; qdhttp -d`
cd unittest
rm -rf results/run1/* || true
./check.sh run1
CODE=$?
kill $thePID
exit $CODE
