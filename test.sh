#!/bin/bash

cd unittest

thePID=`cd ../webserver; ./qdhttp -d -s fork`
rm -rf results/fork/* || true
./check.sh fork
CODE=$?
kill $thePID

thePID=`cd ../webserver; ./qdhttp -d -s thread`
rm -rf results/thread/* || true
./check.sh thread
CODE=$?
kill $thePID

thePID=`cd ../webserver; ./qdhttp -d -s prefork`
rm -rf results/prefork/* || true
./check.sh prefork
CODE=$?
kill $thePID

thePID=`cd ../webserver; ./qdhttp -d -s mux`
rm -rf results/mux/* || true
./check.sh mux
CODE=$?
kill $thePID

