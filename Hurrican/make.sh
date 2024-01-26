#!/bin/bash
cd src
make -j4 TARGET=linux_GL2
cd ..
./hurricanlinux
