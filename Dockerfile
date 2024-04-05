FROM docker.io/gcc:12 as build

RUN mkdir -p /build/src
RUN mkdir -p /build/out
COPY src/BambuP1Streamer.cpp /build/src/
COPY src/BambuTunnel.h /build/src/

RUN mkdir -p /build/deps
WORKDIR /build/deps
RUN curl -LOJ https://public-cdn.bambulab.com/upgrade/studio/plugins/01.04.00.15/linux_01.04.00.15.zip
RUN unzip linux_01.04.00.15.zip

RUN gcc /build/src/BambuP1Streamer.cpp -o /build/out/BambuP1Streamer

#ENV PRINTER_ADDRESS
#ENV PRINTER_ACCESS_CODE

FROM debian:12

RUN apt update && apt install -y \
 curl \
 && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /app

COPY --from=build /build/out/BambuP1Streamer /build/deps/libBambuSource.so /app/

RUN echo \
'streams:\n'\
'   p1s: "exec:./BambuP1Streamer ./libBambuSource.so ${PRINTER_ADDRESS} ${PRINTER_ACCESS_CODE}"\n'\
'log:\n'\
'  level: debug\n'\
'api:\n'\
'  origin: "*"\n'\
> /app/go2rtc.yaml

WORKDIR /app
RUN curl -LOJ https://github.com/AlexxIT/go2rtc/releases/download/v1.6.2/go2rtc_linux_amd64
RUN chmod +x go2rtc_linux_amd64

WORKDIR /app

CMD [ "./go2rtc_linux_amd64" ]
