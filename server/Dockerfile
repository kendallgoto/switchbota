FROM node:current-alpine
# set via build
ARG PROJECT_NAME="switchbota-server"
ARG BUILD_VERSION="1.0.0-snapshot"

COPY . /app

RUN \
  apk update && \
  rm -rf /var/cache/apk/* && \
  cd /app && \
  npm install

WORKDIR /app

ENTRYPOINT "node" "/app/index.js"