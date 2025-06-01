#!/bin/bash

ncat -kl -vvv -e lib/basic/test/bin/bastos 127.0.0.1 1967
