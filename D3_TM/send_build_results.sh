#!/bin/bash

# SCP: TO BOARD
BOARD_TARGET="/tmp/NETWORK"
scp -r ./build/install root@10.10.94.251:${BOARD_TARGET}