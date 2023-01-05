FROM python:latest

RUN apt update && apt install -y cron && rm -rf /var/lib/apt/lists/*
COPY kepu_client /opt/kepu_client
COPY scheduling/kepu-cron /etc/cron.d/kepu-cron
RUN crontab /etc/cron.d/kepu-cron
CMD cron && tail -F /var/log/kepucli.log