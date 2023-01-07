#! /bin/bash

if ! [[ -x wunzip ]]; then
    echo "wunzip executable does not exist"
    exit 1
fi

../run-tests.sh $*


