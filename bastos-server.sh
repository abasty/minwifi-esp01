#!/bin/bash

exec ncat -kl -m1 -vvv -e lib/basic/test/bin/bastos-linux-amd64 127.0.0.1 1967
