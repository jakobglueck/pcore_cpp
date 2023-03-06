#!/bin/bash
cd ..
git submodule add https://github.com/protocolbuffers/protobuf.git external/google-protobuf
cd external/google-protobuf
git checkout v21.9
git submodule update --init --recursive
cd ../..