ifeq ($(CONFIG_SAMBA3),y)
	JANSSON=jansson
endif
ifeq ($(CONFIG_FTP),y)
	JANSSON=jansson
endif
ifeq ($(CONFIG_MINIDLNA),y)
	JANSSON=jansson
endif
ifeq ($(CONFIG_ATH9K),y)
	TINY=libnltiny
endif


libutils-clean:
	make -C libutils clean

shared/revision.h:
	echo "#define SVN_REVISION \""`git rev-parse HEAD`"\"" > shared/revision.h

libutils: shared/revision.h nvram $(TINY)  $(JANSSON)
	make -C libutils

libutils-install:
	make -C libutils install

