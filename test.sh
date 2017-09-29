#!/bin/bash
./build.sh && ./build/dwrap --tool p4merge testdir2/ testdir1/ --allowMultipleDiffs --debug
