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