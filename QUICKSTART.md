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


## Running dtree in Docker

You can use the Dockerfile in this repo to run dtree as a Docker image. 

In the same directory as your Dockerfile, build your image by running `docker build --tag dtree:0.1 .` (The `0.1` is your version number.) 

Then run the image with `sudo docker run -it dtree:0.1`.

In the image, run `sudo bash test.bash`. 

Note: `test.bash` may take several minutes to run and may take up to 32 GB

If everything has gone well, you should eventually see output like 

````
SELECT ASCII_word
NUMERIC_TO_STRING(letter_product,letter_sum)
FROM word
WHERE letter_product*letter_sum<=letter_product+letter_sum
"A"_s,"1, 1"_s,
````

You can test the query by entering `SELECT *
FROM word`. 
