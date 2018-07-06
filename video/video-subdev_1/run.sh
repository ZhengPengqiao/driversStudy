#!/bin/bash

sudo rmmod myvivid
make
sudo insmod myvivid.ko
xawtv -c /dev/video5

