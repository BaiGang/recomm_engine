# dockerfile by baigang
#

FROM centos:centos6
MAINTAINER Sina Ad Algo Team <adtech-algo@staff.sina.com.cn>

# install main pkgs
RUN yum -y update; yum clean all
RUN yum -y install epel-release; yum clean all
RUN yum -y groupinstall "Development Tools"
RUN yum -h install libevent-devel zlib-devel openssl-devel
RUN yum -y install libunwind; yum clean all
RUN yum -y install boost-devel; yum clean all

# install Apache Thrift
ENV THRIFT_PATH http://mirrors.cnnic.cn/apache/thrift/0.9.1/thrift-0.9.1.tar.gz
ADD $THRIFT_PATH thrift-0.9.1.tar.gz
RUN tar xzf thrift-0.9.1.tar.gz
RUN rm thrift-0.9.1.tar.gz

RUN cd thrift-0.9.1 && ./configure --without-tests && make && make install

RUN mkdir -p /home/sina/include 
RUN mkdir -p /home/sina/bin
RUN mkdir -p /home/sina/lib64
RUN mkdir -p /home/sina/conf
RUN mkdir -p /home/sina/log


