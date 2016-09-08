FROM ubuntu:16.04
MAINTAINER libretro

# Update all dependencies
RUN apt-get update && apt-get upgrade -y

# Install RetroArch's build dependencies
RUN apt-get install -y make git-core curl g++ pkg-config libglu1-mesa-dev freeglut3-dev mesa-common-dev libsdl1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev

VOLUME ["/app"]
WORKDIR /app
