FROM docker.io/gcc:12 as build

RUN mkdir -p /build/src
COPY src/BambuP1Streamer.cpp /build/src/
COPY src/BambuTunnel.h /build/src/

RUN mkdir -p /build/deps
WORKDIR /build/deps
RUN wget https://public-cdn.bambulab.com/upgrade/studio/plugins/01.04.00.15/linux_01.04.00.15.zip
RUN unzip linux_01.04.00.15.zip

RUN gcc /build/src/BambuP1Streamer.cpp -o /build/out/BambuP1Streamer

#ENV PRINTER_ADDRESS
#ENV PRINTER_ACCESS_CODE

FROM debian:12
RUN mkdir -p /app

COPY --from=build build/BambuP1Streamer build/libBambuSource.so /app/

RUN echo \
'streams:\n'\
'   p1s: "exec:./BambuP1Streamer ./libBambuSource.so ${PRINTER_ADDRESS} ${PRINTER_ACCESS_CODE}"\n'\
'log:\n'\
'  level: debug\n'\
'api:\n'\
'  origin: "*"\n'\
> /app/go2rtc.yaml

WORKDIR /app
RUN wget https://github.com/AlexxIT/go2rtc/releases/download/v1.6.2/go2rtc_linux_amd64
RUN chmod +x go2rtc_linux_amd64

WORKDIR /app

CMD [ "./go2rtc_linux_amd64" ]
