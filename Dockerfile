# dockerfile by baigang
#

FROM centos:centos6
MAINTAINER Sina Ad Algo Team <adtech-algo@staff.sina.com.cn>

# install main pkgs
RUN yum -y update
RUN yum -y install epel-release
RUN yum -y groupinstall "Development Tools"
RUN yum -y install tar boost-devel libunwind libevent-devel zlib-devel openssl-devel
RUN yum clean all

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

ADD server.app /home/sina/bin/recomm_server
ADD conf/recomm_plugin.conf /home/sina/conf/recomm_plugin.conf
ADD conf/redis_cluster.conf /home/sina/redis_cluster.conf
ADD conf/recomm_plugin.conf /home/sina/recomm_plugin.conf

#ADD include/* /home/sina/include
#ADD idl/gen-cpp/*.h /home/sina/include
#ADD src/*.h /home/sina/include

#ADD lib/librecomm_engine_dev.a /home/sina/lib64


