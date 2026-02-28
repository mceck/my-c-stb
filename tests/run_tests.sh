#!/bin/bash
set -e
mkdir -p tests/build
cc tests/test_ds.c -o tests/build/test_ds
cc tests/test_jsb_jsp.c -o tests/build/test_jsb_jsp
cc tests/test_jsgen.c -o tests/build/test_jsgen
cc tests/test_http.c -o tests/build/test_http -lcurl

echo "Running tests..."
./tests/build/test_ds
./tests/build/test_jsb_jsp
./tests/build/test_jsgen
./tests/build/test_http