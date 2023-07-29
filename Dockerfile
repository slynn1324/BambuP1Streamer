FROM debian:12

#ENV PRINTER_ADDRESS
#ENV PRINTER_ACCESS_CODE

RUN mkdir -p /app

COPY BambuP1Streamer go2rtc_linux_amd64 go2rtc.yaml libBambuSource.so /app

WORKDIR /app

CMD [ "./go2rtc_linux_amd64" ]
