#!/bin/sh -e

make -f Makefile.northstar koldconf
make -f Makefile.northstar kprep
make -f Makefile.northstar configure
make -f Makefile.northstar kernel
make -f Makefile.northstar
make -f Makefile.northstar package

