#!/usr/bin/env bash
set -e

if [[ -z "$1" ]]; then
    echo "usage: $0 <debug|release>" >&2
    exit 1
fi

REPO_ROOT="$(realpath ./)"
FASTEST_HOME="$(realpath tests/vendor/fastest)"
name="IF_tests"

FASTEST_USER_LIB="$REPO_ROOT/build/$1/libImageFlow_tests.a"
IMAGEFLOW_CORE_LIB="$REPO_ROOT/build/$1/libImageFlow.a"

if [[ ! -f "$FASTEST_USER_LIB" ]]; then
    echo "error: $FASTEST_USER_LIB not found — build it first:" >&2
    echo "  cmake --build build/$1 --target imageflow_tests_static" >&2
    exit 1
fi

if [[ ! -f "$IMAGEFLOW_CORE_LIB" ]]; then
    echo "error: $IMAGEFLOW_CORE_LIB not found — build it first:" >&2
    echo "  cmake --build build/$1 --target imageflow_static" >&2
    exit 1
fi

pushd "$FASTEST_HOME/bindings" || exit 1
pip install -r requirements.txt
export FASTEST_HOME
export FASTEST_USER_LIB
export FASTEST_EXTRA_LIBS="$IMAGEFLOW_CORE_LIB"
export FASTEST_MODULE_NAME="$name"
pip install -e .
popd
