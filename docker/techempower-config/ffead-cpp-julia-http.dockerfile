FROM sumeetchhetri/ffead-cpp-5.0-base:5.2

ENV IROOT=/installs

ENV DEBIAN_FRONTEND noninteractive
RUN rm -f /usr/local/lib/libffead-* /usr/local/lib/libte_benc* /usr/local/lib/libinter.so /usr/local/lib/libdinter.so && \
	ln -s ${IROOT}/ffead-cpp-5.0/lib/libte_benchmark_um.so /usr/local/lib/libte_benchmark_um.so && \
	ln -s ${IROOT}/ffead-cpp-5.0/lib/libffead-modules.so /usr/local/lib/libffead-modules.so && \
	ln -s ${IROOT}/ffead-cpp-5.0/lib/libffead-framework.so /usr/local/lib/libffead-framework.so && \
	ln -s ${IROOT}/ffead-cpp-5.0/lib/libinter.so /usr/local/lib/libinter.so && \
	ln -s ${IROOT}/ffead-cpp-5.0/lib/libdinter.so /usr/local/lib/libdinter.so && \
	ldconfig

WORKDIR ${IROOT}
RUN wget https://julialang-s3.julialang.org/bin/linux/x64/1.5/julia-1.5.2-linux-x86_64.tar.gz
RUN tar -xvzf julia-1.5.2-linux-x86_64.tar.gz
RUN mv julia-1.5.2 /opt/
RUN rm -f julia-1.5.2-linux-x86_64.tar.gz
RUN ln -s /opt/julia-1.5.2/bin/julia /usr/local/bin/julia

WORKDIR /

CMD ./run_ffead.sh ffead-cpp-5.0 julia-http
