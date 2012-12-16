NAME=pubkeyfs
CC=gcc -Wall
CFLAGS=-std=c99 -pthread
LIBS=-lldap -lrt -ldl -lfuse -lconfig
EXTRA_CFLAGS=-D_FILE_OFFSET_BITS=64
VERSION=$(shell git describe)
PREFIX=/usr/bin
SPEC_TEMPLATE=ext/redhat/$(NAME).spec
SPEC_FILE=$(NAME).spec

RPMBUILD := $(shell if test -f /usr/bin/rpmbuild ; then echo /usr/bin/rpmbuild ; else echo "x" ; fi)

RPM_DEFINES =   --define "_specdir $(shell pwd)/SPECS" --define "_rpmdir $(shell pwd)/RPMS" --define "_sourcedir $(shell pwd)/SOURCES" --define  "_srcrpmdir $(shell pwd)/SRPMS" --define "_builddir $(shell pwd)/BUILD"

MAKE_DIRS= $(shell pwd)/SPECS $(shell pwd)/SOURCES $(shell pwd)/BUILD $(shell pwd)/SRPMS $(shell pwd)/RPMS 

.PHONEY: install srpm

all: pkfs

pkfs: ldapapi.o utils.o pkfs.o
	$(CC) $(CFLAGS) ldapapi.o utils.o pkfs.o $(LIBS) -o $@

pkfs.o: pkfs.c
	$(CC) $(EXTRA_CFLAGS) -c pkfs.c

ldapapi.o: ldapapi.c
	$(CC) -c ldapapi.c

utils.o: utils.c
	$(CC) -c utils.c

clean:
	rm -rf *.o pkfs *.tar.gz BUILD SRPMS RPMS *.rpm SOURCES SPECS debian/pubkeyfs*

install: pkfs
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	install -m0755 pkfs $(DESTDIR)/$(PREFIX)/bin

tarball:
	@mkdir -p $(NAME)-$(VERSION)
	@cp -pr `git ls-files`  $(NAME)-$(VERSION)
	@tar pzcf $(NAME)-$(VERSION).tar.gz $(NAME)-$(VERSION) --exclude .gitignore
	@rm -rf $(NAME)-$(VERSION)
	@echo "Wrote: $(PWD)/$(NAME)-$(VERSION).tar.gz"

rpmcheck:
ifeq ($(RPMBUILD),x)
	$(error "rpmbuild not found, exiting...")
endif
	@mkdir -p $(MAKE_DIRS)

## use this to build an srpm locally
srpm:  rpmcheck tarball
	@wait
	@sed -e s'/__VERSION__/$(VERSION)/g' $(SPEC_TEMPLATE) > SPECS/$(SPEC_FILE)
	@cp -pr $(NAME)-$(VERSION).tar.gz SOURCES
	@$(RPMBUILD) $(RPM_DEFINES)  -bs SPECS/$(SPEC_FILE)
	@mv -f SRPMS/* .
	@rm -rf BUILD SRPMS RPMS

deb:
	@mv ext/debian debian
	dpkg-buildpackage
