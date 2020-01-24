# Quickstart

## Installation

NOTE: you will need a largish VM to run dtree. This was tested on a n1-standard-4 (4 vCPUs, 15 GB memory) on GCP.

On Ubuntu 18

* `sudo apt-get update`
* `sudo apt install clang`
* `sudo apt install make`
* `wget -c 'http://sourceforge.net/projects/boost/files/boost/1.70.0/boost_1_70_0.tar.bz2`
* `bunzip2 boost_1_70_0.tar.bz2`
* `tar -xvof boost_1_70_0.tar`
* `cd boost_1_70_0/`
* `./bootstrap.sh --with-toolset=clang`
* `./b2 headers`
* `sudo ./b2 install`
* `pushd ~/pl` 
* `make 2>&1 | tee make.log`
* `git clone https://github.com/questrel/dtree.git`


```bash
add-apt-repository ppa:mhier/libboost-latest
apt update
apt-get --purge remove libboost-all-dev libboost-dev libboost-doc
apt-get install -f
apt autoremove
dpkg --configure -a
apt-get clean
apt-get update
apt-get install libboost1.70-dev
add-apt-repository ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install g++-9
```

# Installing dtree under Windows
```bash
1. Install Ubuntu
https://docs.microsoft.com/en-us/windows/wsl/install-win10

2. Make sure everything is up-to-date
sudo apt-get update && sudo apt-get upgrade

3. Install /usr/share/dict/words
sudo apt-get install wamerican

4. Install make
sudo apt-get install make

5. Prerequisites for clang
sudo apt install build-essential xz-utils curl

5. Install clang (9.0.0) [via https://solarianprogrammer.com/2017/12/13/linux-wsl-install-clang-libcpp-compile-cpp-17-programs/]
curl -SL http://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz | tar -xJC .
mv clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04 clang_9.0.0
sudo mv clang_9.0.0 /usr/local
export PATH=/usr/local/clang_9.0.0/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/clang_9.0.0/lib:$LD_LIBRARY_PATH
sudo ldconfig

6. Install boost
sudo apt-get install libboost-all-dev
```
