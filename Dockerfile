FROM ubuntu:16.04
MAINTAINER libretro

# Update all dependencies
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get upgrade -y

# Install RetroArch's build dependencies
RUN apt-get install -y make git-core curl g++ pkg-config libglu1-mesa-dev freeglut3-dev mesa-common-dev libsdl1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev software-properties-common

# Install RetroArch from the PPA
RUN apt-add-repository -y multiverse
RUN add-apt-repository -y ppa:libretro/testing
RUN apt-get update && apt-get upgrade -y
RUN apt-get install retroarch -y
RUN mkdir -p ~/.config/retroarch
RUN echo "video_driver = \"null\"" >> ~/.config/retroarch/retroarch.cfg
RUN echo "audio_driver = \"null\"" >> ~/.config/retroarch/retroarch.cfg
RUN echo "confirm_on_exit = \"false\"" >> ~/.config/retroarch/retroarch.cfg

VOLUME ["/app"]
WORKDIR /app
