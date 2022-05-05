# Use the official image as a parent image
FROM ubuntu:20.04

# Add Jenkins as a user with sufficient permissions
RUN mkdir /home/jenkins
RUN groupadd -g 136 jenkins
RUN useradd -r -u 126 -g jenkins -G plugdev -d /home/jenkins jenkins
RUN chown jenkins:jenkins /home/jenkins

WORKDIR /home/jenkins

CMD ["/bin/bash"]

# override interactive installations
ENV DEBIAN_FRONTEND=noninteractive 

# Install prerequisites
RUN apt-get update && apt-get install -y \
    cmake \
    gcc \
    gcc-arm-none-eabi \
    git \
    g++ \
    libbluetooth-dev \
    libnewlib-arm-none-eabi \
    libusb-1.0-0-dev \
    make \
    pkg-config \
    python3-distutils \
    python3-numpy \
    python3-pip \
    python3-qtpy \
    python3-setuptools \
    python-is-python3 \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install git+https://github.com/CapableRobot/CapableRobot_USBHub_Driver --upgrade

RUN wget https://github.com/greatscottgadgets/libbtbb/archive/2020-12-R1.tar.gz -O libbtbb-2020-12-R1.tar.gz &&\
    tar -xf libbtbb-2020-12-R1.tar.gz &&\
    cd libbtbb-2020-12-R1 &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make &&\
    make install &&\
    ldconfig

USER jenkins

# Inform Docker that the container is listening on the specified port at runtime.
EXPOSE 8080

# Copy the rest of your app's source code from your host to your image filesystem.
COPY . .