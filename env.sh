#!/bin/bash
set -e

#########################################################
#   config
#########################################################
export PATH=$PATH:/${PWD}/build/bin
export LIBRARY_PATH=$LIBRARY_PATH:/${PWD}/build/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/${PWD}/build/lib

if [ -d "${PWD}/.git/" ]; then
  if [ -f "${PWD}/tools/commit-template" ]; then
    git config --local commit.template tools/commit-template
  fi

  if [ -f "${PWD}/.git/hooks/pre-commit" ]; then
    rm -r ${PWD}/.git/hooks/pre-commit
  fi
  ln -sf ${PWD}/tools/pre-commit ${PWD}/.git/hooks
fi

export RUNTIME_BUILD_ENV_ENABLED=1

#########################################################
#   env check
#########################################################
if [ "$(which clang-format)" == "" ];then
  echo "Please install clang-format."
  echo "sudo apt-get install clang-format"
  echo "brew install clang-format"
fi
