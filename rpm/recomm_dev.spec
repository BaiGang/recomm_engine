##############################################################
# http://www.sina.com.cn
# http://www.rpm.org/max-rpm/ch-rpm-inside.html              #
##############################################################
Name:%(echo recomm_engine_dev${SUFFIX})
#
Version:%{_version}
# if you want get version number from outside, use like this
Release:%(echo $RELEASE)%{?dist}
# if you want use the parameter of rpm_create on build time,
# uncomment below
Summary: The dev and util tools lib for the recomm service.
# this is the svn URL of current dir
URL: %{_svn_path}
Group: sina_recomm
License: Commercial

# uncomment below, if depend on other package
#BuildRequires: zeromq-dev = 1.0.0

# uncomment below, if depend on other package
#Requires: zeromq >= 2.1.9

AutoReq: no

%description
# if you want publish current svn URL or Revision use these macros
Recomm service projects should rely on this package for the idl definitions.
%{_svn_path}
%{_svn_revision}

# prepare your files
%install
# OLDPWD is the dir of rpm_create running
# _prefix is an inner var of rpmbuild,
# can set by rpm_create, default is "/home/w"
# _lib is an inner var, maybe "lib" or "lib64" depend on OS

# those dirs will be in in installed path
# create dirs
mkdir -p .%{_prefix}/include/
mkdir -p .%{_prefix}/include/gflags
mkdir -p .%{_prefix}/include/glog
mkdir -p .%{_prefix}/include/leveldb
mkdir -p .%{_prefix}/include/gtl
mkdir -p .%{_prefix}/%{_lib}

# copy files you want to include in the package
# copy files
cp $OLDPWD/../include/*.h            .%{_prefix}/include
cp $OLDPWD/../idl/gen-cpp/*.h        .%{_prefix}/include
cp $OLDPWD/../include/gflags/*.h     .%{_prefix}/include/gflags
cp $OLDPWD/../include/glog/*.h       .%{_prefix}/include/glog
cp $OLDPWD/../include/gtl/*.h        .%{_prefix}/include/gtl
cp $OLDPWD/../include/leveldb/*.h    .%{_prefix}/include/leveldb

cp $OLDPWD/../src/*.h                .%{_prefix}/include

cp $OLDPWD/../lib/*.a    .%{_prefix}/%{_lib}

# Pre build commands
%pre

# Post build commands
%post

# package infomation
%files
# set file attribute here
%defattr(755,root,root)
# need not list every file here, keep it as this
%{_prefix}
#%config(noreplace)	%{_prefix}/share/engine/plugins/conf/librank_rtsv1.so.conf

# create an empy dir
# need bakup old config file, so indicate here
# indicate the dir for crontab

%changelog
* Sun Oct 26 2013 baigang<baigang@staff.sina.com.cn> 1.0.0
- Added support for smoothed ctr, which replaces the model-calculated ctr.
