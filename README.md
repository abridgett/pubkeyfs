# Public Key Filesystem

## Installation

### Debian

**Edit** `/etc/apt/sources.list.d/pubkeyfs.list`

    deb http://apt.pubkeyfs.org/debian Squeeze main

-

    wget http://apt.pubkeyfs.org/debian/conf/pubkeyfs.gpg.key
    apt-key add pubkeyfs.gpg.key
    apt-get update
    apt-get install pubkeyfs

### Centos 5

**Edit** `/etc/yum.repos.d/pubkeyfs.repo`

    [pubkeyfs]
    name=PubkeyFS
    baseurl=http://yum.pubkeyfs.org/centos/5/i386/
    enabled=1
    gpgkey=http://yum.pubkeyfs.org/RPM-GPG-KEY-pubkeyfs-release

-

    yum update
    yum install pubkeyfs


## Configuration

**Edit** `/etc/pkfs.conf`

    uri  = "ldaps://ldap.example.com";
    dn   = "cn=root,dc=example,dc=com";
    pass = "secret";
    base = "ou=users,dc=example,dc=com";
    key_attr = "sshPublicKey";


### key_attr
The `key_attr` setting should match the name of the LDAP attribute that you are
using to store user ssh public keys in the directory. Any attribute will do, but
the [OpenLDAP Public Key schema](http://code.google.com/p/openssh-lpk/source/browse/trunk/schemas/openssh-lpk_openldap.schema)
from the openssh-lpk project works great.

## Example Usage

First we need to create a directory where the public key filesystem will be
mounted:

**Create** `/var/lib/publickeys`

    mkdir /var/lib/publickeys

**Edit** `/etc/ssh/sshd_config`

    AuthorizedKeysFile /var/lib/publickeys/%u

The above setting tells the ssh daemon to look under `/var/lib/publickeys` for
SSH public keys. %u becomes the userid that will be looked up in LDAP.

Finally mount pkfs using the pkfs binary:

    /usr/bin/pkfs -o allow_other /var/lib/publickeys

Or add the following entry to fstab:

    /usr/bin/pkfs           /var/lib/publickeys     fuse    allow_other,_netdev     0 0

At this point you should have a read-only pseudo filesystem mounted under
`/var/lib/publickeys`.


## Packaging

### Install

By default pubkeyfs is installed into /usr/bin. This can be overridden with
something like:

    make install PREFIX=/usr

### Versioning

Versioning for this project is done via git describe. This means you simply
create a tag and that is the version number.  Commits without an associated tag
are referenced via a describe methodology from the latest found tag. See
https://www.kernel.org/pub/software/scm/git/docs/git-describe.html.

### RPM
Building rpms is easy.

    make srpm

From the output of that command, you can either run:

    rpmbuild --rebuild pubkeyfs-*.src.rpm

or throw the srpm in mock to build for multiple EL/Fedora targets.
http://fedoraproject.org/wiki/Projects/Mock

### DEB

Building debs is easy too.

    make deb
