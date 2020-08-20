# !/bib/bash
set -e
##################################
# env
##################################
BUILD_DIR="build"
BUILD_TYPE="Release"
CMAKE="cmake"
GCC=`which gcc`
GXX=`which g++`


if [ ! -d ${BUILD_DIR} ]; then
  mkdir ${BUILD_DIR}
else
  echo "-"
  # rm -rf ${BUILD_DIR}/*
fi

pushd ${BUILD_DIR}
 ${CMAKE} \
 -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
 -DCMAKE_C_COMPILE=${GCC} \
 -DCMAKE_CXX_COMPILER=${GXX} \
 ..

 make -j32
popd
