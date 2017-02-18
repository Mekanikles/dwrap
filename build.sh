#!/bin/sh
g++ -g -std=c++14 `/opt/fltk-1.3.4-1/fltk-config --cxxflags --ldflags` source/Main.cpp -o build/dwrap
