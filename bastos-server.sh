#!/bin/bash

ncat -kl -m1 -vvv -e lib/basic/test/bin/bastos 127.0.0.1 1967
