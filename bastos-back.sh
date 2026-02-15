#!/bin/bash

websocat -t -E --no-line ws-l:127.0.0.1:1967 exec:lib/basic/test/bin/bastos
