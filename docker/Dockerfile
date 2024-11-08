FROM ubuntu:22.04
RUN apt-get update && apt-get install -y gcc g++ cmake libtool-bin python3-dev
RUN mkdir -p cinnamon_artifact
COPY simulator cinnamon_artifact/simulator
WORKDIR /cinnamon_artifact/simulator
RUN chmod +x build_sst.sh && ./build_sst.sh

# FROM cinnamon_artifact:v1
COPY evaluation /cinnamon_artifact/evaluation
COPY scripts /cinnamon_artifact/scripts
RUN apt-get update && apt-get install -y pip && pip install matplotlib

WORKDIR /cinnamon_artifact/scripts/
CMD ["/cinnamon_artifact/scripts/entrypoint.sh"]
