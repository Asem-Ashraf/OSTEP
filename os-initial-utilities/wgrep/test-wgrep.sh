#! /bin/bash

if ! [[ -x wgrep ]]; then
    echo "wgrep executable does not exist"
    exit 1
fi

../run-tests.sh $*



