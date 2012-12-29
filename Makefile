NAME        = pubkeyfs
CC          = gcc -Wall
CFLAGS      = -std=c99 -pthread -D_GNU_SOURCE
LIBS        = -lldap -lfuse -lconfig
FUSE_CFLAGS = -D_FILE_OFFSET_BITS=64
VERSION     = 0.0.1
PREFIX      = /usr
OBJECTS     = ldapapi.o utils.o pkfs.o

vpath %.c src
vpath %.h src

all: pkfs

pkfs: $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

pkfs.o: pkfs.c ldapapi.h utils.h
	$(CC) $(FUSE_CFLAGS) -c $<

ldapapi.o: ldapapi.c ldapapi.h
	$(CC) $(CFLAGS) -c $<

utils.o: utils.c utils.h
	$(CC) -c $<


.PHONEY: clean cleanrpm cleantarball cleandeb

clean: cleanrpm cleantarball cleandeb
	rm -rf $(OBJECTS) pkfs

cleandeb:
	rm -rf debian/pubkeyfs* *.deb

cleanrpm:
	rm -rf BUILD SRPMS RPMS *.rpm SOURCES SPECS

cleantarball:
	rm -rf *.tar.gz


.PHONEY: install

install: pkfs
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	install -m0755 pkfs $(DESTDIR)/$(PREFIX)/bin


.PHONEY: tarball srpm deb

include ext/Makefile.tarball
include ext/Makefile.rpm
include ext/Makefile.deb
