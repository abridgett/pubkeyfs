%global real_version __VERSION__
%global fixed_version %(echo %real_version | sed -e 's/-/_/g')

Name:           pubkeyfs
Version:        %{fixed_version}
Release:        1%{?dist}
Summary:        FUSE filesystem for SSH Public Keys

Group:          System Environment/Daemons
License:        MIT
URL:            https://github.com/kelseyhightower/pubkeyfs
Source0:        %{name}-%{real_version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gcc, fuse, fuse-libs, fuse-devel, openldap-devel, libconfig-devel, libconfig
Requires:       fuse, fuse-libs, fuse-devel, openldap-devel, libconfig-devel, libconfig

%description


%prep
%setup -q -n  %{name}-%{real_version}


%build
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT PREFIX=/usr


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc
%{_bindir}/pkfs

%changelog
* Wed Dec 12 2012 <stahnma@puppetlabs.com> - 0.0.0.0
- Initial packaging
