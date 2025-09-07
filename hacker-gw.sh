#!/bin/bash

exec ncat -kl -m1 -vvv 127.0.0.1 1963 -e "/home/alain/Downloads/websocat -b ws://mntl.joher.com:2018"
