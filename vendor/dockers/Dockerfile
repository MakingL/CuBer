FROM centos:8

MAINTAINER michallee104@126.com

ENV LANG C.UTF-8

RUN mkdir -p /usr/local/CuBer/conf/ /usr/local/CuBer/www/ /var/log/CuBer && \
    cd /usr/local/src && \
    yum -y install git make cmake gcc-c++ && \
    git clone https://github.com/MakingL/CuBer.git && \
    cd CuBer && \
    cp vendor/index/index.html /usr/local/CuBer/www/ && \
    cp conf/cuber.yaml /usr/local/CuBer/conf/ && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local/CuBer/ ./ && \
    make install && \
    cd .. && rm CuBer/ -rf  && \
    yum -y remove git make cmake gcc-c++ && \
    yum clean all

WORKDIR /usr/local/CuBer

EXPOSE 8080

CMD ["/usr/local/CuBer/bin/CuBer", "-c", "/usr/local/CuBer/conf/cuber.yaml"]
