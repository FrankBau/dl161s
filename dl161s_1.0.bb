DESCRIPTION = "hello"
SECTION = "examples"
DEPENDS = "libusb"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=121076f22426f071132beb530878a672"

SRC_URI = "file://src/Makefile.am \
           file://src/dl161s.c \
           file://Makefile.am \
           file://COPYING \
           file://configure.ac \
           file://autogen.sh \
          "

S = "${WORKDIR}"

inherit autotools pkgconfig

