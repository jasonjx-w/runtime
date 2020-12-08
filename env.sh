#!/bin/bash
set -e

#########################################################
#   config
#########################################################
echo "-- Config PATH/LIBRARY_PATH/LD_LIBRARY_PATH ..."
export PATH=${PWD}/build/bin:$PATH
export LIBRARY_PATH=${PWD}/build/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=${PWD}/build/lib:$LD_LIBRARY_PATH

echo "-- Config git hooks ..."
if [ -d "${PWD}/.git/" ]; then
  # .git is dir, use config files in tools/
  COMMIT_TEMPLATE="${PWD}/tools/commit-template"
  if [ -f $COMMIT_TEMPLATE ]; then
    git config --local commit.template $COMMIT_TEMPLATE
    echo "-- Set commit-template as ${COMMIT_TEMPLATE}."
  fi

  PRE_COMMIT="${PWD}/tools/pre-commit"
  if [ -f "${PWD}/.git/hooks/pre-commit" ]; then
    rm -f ${PWD}/.git/hooks/pre-commit
  fi
  ln -sf $PRE_COMMIT ${PWD}/.git/hooks
  echo "-- Set pre-commit as ${PRE_COMMIT}."
elif [ -f "${PWD}/.git" ]; then
  # .git is file, use config files in ../tools/
  COMMIT_TEMPLATE="${PWD}/../tools/commit-template"
  if [ -f $COMMIT_TEMPLATE ]; then
    git config --local commit.template $COMMIT_TEMPLATE
    echo "-- Set commit-template as ${COMMIT_TEMPLATE}."
  fi

  PRE_COMMIT="${PWD}/../tools/pre-commit"
  if [ -f "${PWD}/../.git/hooks/pre-commit" ]; then
    rm -r ${PWD}/../.git/hooks/pre-commit
  fi
  ln -sf $PRE_COMMIT ${PWD}/../.git/hooks
  echo "-- Set pre-commit as ${PRE_COMMIT}."
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
