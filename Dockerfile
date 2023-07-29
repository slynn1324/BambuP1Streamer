FROM debian:12

#ENV PRINTER_ADDRESS
#ENV PRINTER_ACCESS_CODE

RUN mkdir -p /app

COPY build/BambuP1Streamer build/go2rtc_linux_amd64 build/libBambuSource.so go2rtc.yaml /app

WORKDIR /app

CMD [ "./go2rtc_linux_amd64" ]
