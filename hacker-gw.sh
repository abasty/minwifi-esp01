#!/bin/bash

exec ncat -kl -m1 -vvv 127.0.0.1 1963 -e "/usr/local/bin/websocat -b ws://mntl.joher.com:2018/?echo"
