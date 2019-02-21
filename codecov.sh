if [[ ! ("$TRAVIS_COMPILER" == "clang") && ("$CONFIG" == "debug") && ("$EXTCALL" == "manual") ]]; then
  bash <(curl -s https://codecov.io/bash)
else
  echo "No coverage data in this configuration"
fi
exit 0

