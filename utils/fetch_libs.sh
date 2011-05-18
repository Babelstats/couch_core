#!/bin/sh

CURL_BIN=`which curl`

ICU_URI="http://rcouch.refuge.io/dl/libs/icu4c-4_6_1-src.tgz"
TARGET=`pwd`/c_src/icu4c-4_6_1-src.tgz

if ! test -f $TARGET; then
    echo "==> Fetch ${TARGET}"
    $CURL_BIN $ICU_URI -o $TARGET
fi
