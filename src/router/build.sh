#!/bin/sh -ex

./touches.sh
cp configs/northstar/.config_northstar .config
#make -f Makefile.northstar koldconf
#make -f Makefile.northstar kprep
make -f Makefile.northstar kernel
make -f Makefile.northstar install_headers
make -f Makefile.northstar configure
make -f Makefile.northstar json-c-clean
make -f Makefile.northstar json-c-configure
make -f Makefile.northstar
make -f Makefile.northstar package

