#!/bin/sh -ex

./touches.sh
cp configs/northstar/.config_northstar .config
#make -f Makefile.northstar koldconf
#make -f Makefile.northstar kprep
make -f Makefile.northstar kernel || (echo kernel; false)
make -f Makefile.northstar install_headers || (echo install_headers; false)
make -f Makefile.northstar configure || make -f Makefile.northstar configure || (echo configure; false)
make -f Makefile.northstar json-c-clean || (echo json-c-clean; false)
make -f Makefile.northstar json-c-configure || (echo json-c-configure; false)
make -f Makefile.northstar || (echo make; false)
make -f Makefile.northstar package || (echo package; false)

