
if  [ $# -eq 0 ] || [ "${1,,}" != "debug" ] && [ "${1,,}" != "release" ]
  then
    echo "Usage: $@ (Debug|Release)"
    exit 1
fi

if [ "${1,,}" = "debug" ]; then
  extra_args=(--with-c-flags="-fPIC -g -Wall" LDFLAGS=-fPIC CFLAGS_ENGINE1="-C -fPIC" --prefix="`pwd`/../deploy/debug")
else
  extra_args=("--prefix=`pwd`/../deploy/release")
fi

if ! command -v gcc &> /dev/null
  then
    echo Installing gcc...
    sudo apt-get update
    sudo apt install gcc
  fi
# || [ ! (grep -Fxq "${1,,}" configured) ]
if  ! test -f configured || ! grep -Fxq "${1,,}" configured 
then
  echo Configuring ${1,,}...
  make clean
  ./configure "${extra_args[@]}"
  echo ${1,,} > configured
  make config 
fi

make
make install-system

pushd .. &> /dev/null
deploy_dir=`pwd`/deploy/${1,,}
popd &> /dev/null

echo
echo GProlog installed in $deploy_dir

