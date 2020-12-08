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

# 2.build dir
if [ ! -d ${BUILD_DIR} ]; then
  mkdir ${BUILD_DIR}
fi

# 3.handle build options
if [ $# != 0 ]; then
  while [ $# != 0 ]; do
    case "$1" in
      -d | --debug)
        BUILD_TYPE="Debug"
        shift
        ;;
      *)
        echo "-- Unknown options ${1}, use -h or --help"
        exit -1
        ;;
    esac
  done
fi

# 4.force checking env.sh
if [ ! ${RUNTIME_BUILD_ENV_ENABLED} ]; then
  echo "--Please source env.sh before build."
  exit -1
fi

##################################
# build
##################################
pushd ${BUILD_DIR}
 rm -rf *
 ${CMAKE} \
 -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
 -DCMAKE_C_COMPILE=${GCC} \
 -DCMAKE_CXX_COMPILER=${GXX} \
 -DRUNTIME_JUST_LIBRARY='OFF' \
 ..

 make -j32
popd
