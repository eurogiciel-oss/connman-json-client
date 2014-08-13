Summary:        A simple ncurses interface for connman
Name:           connman-ncurses
Version:        0.1
Release:        1
License:        GPL-2.0
Group:          Network & Connectivity/Connection Management
Source:         %{name}-%{version}.tar.gz
URL:            https://github.com/alan-mushi/connman-json-client
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(ncurses)
BuildRequires:  libjson-devel
Requires:       connman
Requires:       libjson

%description
Ncurses interface for connman dbus service.
It's meant to be easy to use, try it out.

%prep
%setup -q

%build
autoreconf -i
%configure --disable-optimization --enable-debug
make %{?_smp_mflags}

%install
install -Dm755 connman_ncurses %{buildroot}/%{_bindir}/%{name}

%files
%{_bindir}/%{name}

%changelog
