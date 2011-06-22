#!/bin/sh

CURL_BIN=`which curl`

ICU_URI="http://rcouch.refuge.io/dl/libs/icu4c-4_8-src.tgz"
ICUDATA_URI="http://rcouch.refuge.io/dl/libs/icudt48l.zip"
TARGET=`pwd`/c_src/icu4c-4_8-src.tgz
TARGET1=`pwd`/c_src/icudt48l.zip


if ! test -f $TARGET; then
    echo "==> Fetch icu library"
    $CURL_BIN $ICU_URI -o $TARGET
fi

if ! test -f $TARGET1; then
    echo "==> Fetch icu data"
    $CURL_BIN $ICUDATA_URI -o $TARGET1
fi
