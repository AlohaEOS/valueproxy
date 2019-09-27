#!/bin/bash
cd src
eosio-cpp bptracking.cpp -o ../bptracking.wasm -abigen -I ../include -I ./ -R ../resources
