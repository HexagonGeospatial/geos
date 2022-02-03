#!/bin/sh
#
# Raspberry Pi (berrie64) CI script runner for GEOS
# 64-bit ARM chip
#
# Copyright (c) 2020 Regina Obe <lr@pcorp.us>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU Lesser General Public Licence as published
# by the Free Software Foundation.
# See the COPYING file for more information.
#
# read version from Version.txt file
. Version.txt
export REL_PATH=~/workspace/geos/rel-${GEOS_VERSION_MAJOR}.${GEOS_VERSION_MINOR}.${GEOS_VERSION_PATCH}
echo $REL_PATH
# auto tools
if false; then
    sh autogen.sh
    ./configure --prefix=${REL_PATH}
    make && make install
    make check
fi

# cmake
if true; then
    rm -rf build
    mkdir -p build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX:PATH=${REL_PATH} ../
    make && make install
    [ -f CMakeCache.txt ] && \
        ctest --output-on-failure . || \
        make check
fi
