# Use the official image as a parent image
FROM ubuntu:18.04
# Run the commands
RUN apt-get update
RUN apt-get install sudo
RUN adduser --disabled-password --gecos '' docker
RUN adduser docker sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
USER docker
RUN sudo apt-get update
RUN sudo apt-get install software-properties-common -y
RUN sudo apt-get install wamerican
RUN sudo apt-get install make
RUN sudo apt install build-essential xz-utils curl -y
#RUN sudo curl -SL http://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz | sudo tar $
RUN sudo curl -SL http://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz | sudo tar -xJC .
RUN sudo mv clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04 clang_9.0.0
RUN sudo mv clang_9.0.0 /usr/local
ENV PATH="/usr/local/clang_9.0.0/bin:${PATH}"
ENV LD_LIBRARY_PATH=/usr/local/clang_9.0.0/lib:$LD_LIBRARY_PATH
RUN sudo ldconfig
RUN sudo add-apt-repository ppa:mhier/libboost-latest -y
RUN sudo apt update
RUN sudo apt-get --purge remove libboost-all-dev libboost-dev libboost-doc
RUN sudo apt-get install -f
RUN sudo apt autoremove
RUN sudo dpkg --configure -a
RUN sudo apt-get clean
RUN sudo apt-get update
RUN sudo apt-get install libboost1.70-dev -y
RUN sudo add-apt-repository ppa:ubuntu-toolchain-r/test
RUN sudo apt-get update

# you should have already cloned dtree: `git clone https://github.com/questrel/dtree.git`
COPY --chown=docker . dtree

RUN sudo ln -s /usr/local/clang_9.0.0/bin/clang++ /bin/clang++

RUN cd dtree
RUN ls
RUN pwd
WORKDIR ./dtree
RUN echo $PATH
# Inform Docker that the container is listening on the specified port at runtime.
EXPOSE 8080
CMD [ "/bin/bash" ]