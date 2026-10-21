#!/bin/sh

set -e
LD_LIBRARY_PATH=. ./test_lib
LD_PRELOAD=./libmalloc.so factor 20 30 40 50 60 70 80 90
LD_PRELOAD=./libmalloc.so cat Makefile
LD_PRELOAD=./libmalloc.so tree /
