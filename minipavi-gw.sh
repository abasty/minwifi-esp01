#!/bin/bash

exec ncat -kl -m1 -vvv 192.168.1.126 1967 -e "/home/alain/Downloads/websocat -b --ws-dont-check-headers ws://go.minipavi.fr:8182/"
