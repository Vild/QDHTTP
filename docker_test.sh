#!/bin/sh
sed 's@archive.ubuntu.com@ftp.acc.umu.se@' -i /etc/apt/sources.list
apt-get update
apt-get install -y build-essential git xutils-dev apache2-utils gnuplot metcat python2.7
cd /tmp
git clone https://github.com/Vild/QDHTTP.git
cd QDHTTP/
make test

