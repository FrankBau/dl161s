# dl-161s
sound level data logger

The protocol seems to be the same as described here:

http://www.produktinfo.conrad.com/datenblaetter/100000-124999/100032-da-01-en-Schnittstelle_DL160S_SCHALLPDATENLOGGER.pdf

see also:

https://sigrok.org/wiki/Voltcraft_DL-160S

# dependencies

written with libusb 0.1 API, therefore
* libusb-1_0-devel
* libusb-compat-devel
need to be installed, e.g.

    sudo apt-get install libusb-dev

# how to build

## bitbake (Yocto) 

t.b.d.

## autotools

cd dl161s
./autogen.sh 
./configure 
make

