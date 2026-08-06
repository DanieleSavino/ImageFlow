#!/usr/bin/env bash
set -e

usage() {
    echo "usage: $0 <debug|release> [--enable cuda|hip]" >&2
    exit 1
}

if [[ -z "$1" ]]; then
    usage
fi
BUILD_TYPE="$1"
shift

ENABLE="none"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --enable)
            ENABLE="$2"
            shift 2
            ;;
        --enable=*)
            ENABLE="${1#*=}"
            shift
            ;;
        *)
            echo "unknown arg: $1" >&2
            usage
            ;;
    esac
done

case "$ENABLE" in
    cuda|hip|none) ;;
    *)
        echo "error: --enable must be one of: cuda, hip" >&2
        exit 1
        ;;
esac

REPO_ROOT="$(realpath ./)"
FASTEST_BUILD="$(realpath ./build)"
FASTEST_HOME="$(realpath tests/vendor/fastest)"
name="IF_tests"
FASTEST_USER_LIB="$REPO_ROOT/build/$BUILD_TYPE/libImageFlow_tests.a"
IMAGEFLOW_CORE_LIB="$REPO_ROOT/build/$BUILD_TYPE/libImageFlow.a"

if [[ ! -f "$FASTEST_USER_LIB" ]]; then
    echo "error: $FASTEST_USER_LIB not found — build it first:" >&2
    echo "  cmake --build build/$BUILD_TYPE --target imageflow_tests_static" >&2
    exit 1
fi
if [[ ! -f "$IMAGEFLOW_CORE_LIB" ]]; then
    echo "error: $IMAGEFLOW_CORE_LIB not found — build it first:" >&2
    echo "  cmake --build build/$BUILD_TYPE --target imageflow_static" >&2
    exit 1
fi

EXTRA_COMPILE_ARGS=""
EXTRA_LIBS="$IMAGEFLOW_CORE_LIB"

case "$ENABLE" in
    cuda)
        NVCC_PATH="$(command -v nvcc || true)"
        if [[ -z "$NVCC_PATH" ]]; then
            echo "error: nvcc not found on PATH (need CUDA toolkit)" >&2
            exit 1
        fi
        CUDA_HOME="$(dirname "$(dirname "$(realpath "$NVCC_PATH")")")"
        CUDA_LIBDIR="$CUDA_HOME/lib64"
        [[ -d "$CUDA_LIBDIR" ]] || CUDA_LIBDIR="$CUDA_HOME/lib"
        EXTRA_COMPILE_ARGS="-I${CUDA_HOME}/include -DFASTEST_ENABLE_CUDA"
        EXTRA_LIBS="$EXTRA_LIBS -L${CUDA_LIBDIR} -lcudart"
        ;;
    hip)
        HIPCC_PATH="$(command -v hipcc || true)"
        if [[ -z "$HIPCC_PATH" ]]; then
            echo "error: hipcc not found on PATH (need ROCm/HIP toolkit)" >&2
            exit 1
        fi
        if command -v hipconfig >/dev/null 2>&1; then
            ROCM_HOME="$(hipconfig --path 2>/dev/null || hipconfig -R 2>/dev/null)"
        fi
        if [[ -z "$ROCM_HOME" ]]; then
            ROCM_HOME="$(dirname "$(dirname "$(realpath "$HIPCC_PATH")")")"
        fi
        ROCM_LIBDIR="$ROCM_HOME/lib64"
        [[ -d "$ROCM_LIBDIR" ]] || ROCM_LIBDIR="$ROCM_HOME/lib"
        EXTRA_COMPILE_ARGS="-I${ROCM_HOME}/include -DFASTEST_ENABLE_HIP"
        EXTRA_LIBS="$EXTRA_LIBS -L${ROCM_LIBDIR} -lamdhip64"
        ;;
    none)
        ;;
esac

pushd "$FASTEST_HOME/bindings" || exit 1
pip install -r requirements.txt
export FASTEST_HOME
export FASTEST_BUILD
export FASTEST_USER_LIB
export FASTEST_EXTRA_COMPILE_ARGS="$EXTRA_COMPILE_ARGS"
export FASTEST_EXTRA_LIBS="$EXTRA_LIBS"
export FASTEST_MODULE_NAME="$name"
pip install -e .
popd
