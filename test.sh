#!/bin/bash

cd unittest

thePID=`cd ../webserver; ./qdhttp -d -s fork`
rm -rf results/fork/* || true
./check.sh fork
CODE=$?
kill $thePID

thePID=`cd ../webserver; ./qdhttp -d -s mux`
rm -rf results/mux/* || true
./check.sh mux
CODE=$?
kill $thePID

