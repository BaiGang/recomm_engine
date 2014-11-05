# dockerfile by baigang
#

FROM centos:centos6
MAINTAINER Sina Ad Algo Team <adtech-algo@staff.sina.com.cn>

# install main pkgs
RUN yum -y install epel-release; yum clean all
RUN yum -y install gcc gcc-c++ ; yum clean all
RUN yum -y install tar ; yum clean all
RUN yum -y install boost-devel ; yum clean all
RUN yum -y install libunwind ; yum clean all
RUN yum -y install libevent-devel ; yum clean all
RUN yum -y install zlib-devel ; yum clean all
RUN yum -y install openssl-devel ; yum clean all

# install Apache Thrift
ADD http://mirrors.cnnic.cn/apache/thrift/0.9.1/thrift-0.9.1.tar.gz thrift-0.9.1.tar.gz
RUN tar xzf thrift-0.9.1.tar.gz && cd thrift-0.9.1 && ./configure --enable-shared --without-python --without-tests  && make -j4 && make install #&& cd .. && rm -rf thrift-0.9.1*

RUN mkdir -p /home/sina/include \
    && mkdir -p /home/sina/bin \
    && mkdir -p /home/sina/lib64 \
    && mkdir -p /home/sina/conf \
    && mkdir -p /home/sina/share/recomm_plugins \
    && mkdir -p /home/sina/log/indexing_replay_log 

ADD server.app /home/sina/bin/recomm_server
ADD conf/recomm_plugin.conf /home/sina/conf/recomm_plugin.conf
ADD conf/redis_cluster.conf /home/sina/conf/redis_cluster.conf
ADD conf/recomm_plugin.conf /home/sina/conf/recomm_plugin.conf

EXPOSE 5300
EXPOSE 5100

CMD ["/home/sina/bin/recomm_server", "--flagfile", "/home/sina/conf/recomm_server.conf"]

