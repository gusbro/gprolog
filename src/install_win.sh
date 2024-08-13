
if  [ $# -eq 0 ] || [ "${1,,}" != "debug" ] && [ "${1,,}" != "release" ]
  then
    echo "Usage: $@ (Debug|Release)"
    exit 1
fi

if [ "${1,,}" = "debug" ]; then
  extra_args="--with-c-flags=debug --prefix=`pwd`/../deploy/debug"
else
  extra_args="--prefix=`pwd`/../deploy/release"
fi

if (! test -f /usr/local/bin/yasm-win64); then
  if ! command -v make &> /dev/null
  then
    echo Installing make...
    sudo apt-get update
    sudo apt install make
  fi
  echo Creating links...
  sudo ln -s "`which cl.exe`" /usr/local/bin/cl
  sudo ln -s "`which lib.exe`" /usr/local/bin/lib
  sudo ln -s "`which rc.exe`" /usr/local/bin/rc
  echo Copying Yasm...
  sudo cp ../../yasm/Mkfiles/vs/x64/Debug/yasm.exe /usr/local/bin/yasm-win64
  if (! test -f /usr/local/bin/yasm-win64); then
    echo Error: could not copy Yasm!
    exit 1
  fi
  #sudo wget -O /usr/local/bin/yasm-win64 gprolog.univ-paris1.fr/yasm-win64.exe  # Warning: version 1.3.0 mishandles comments within literals - https://github.com/yasm/yasm/pull/81
  #sudo chmod 777 /usr/local/bin/yasm-win64
  cp /usr/local/bin/yasm-win64 ./yasm-win64.exe
fi
export PATH=`pwd`/Wam2Ma:`pwd`/Ma2Asm:`pwd`/Pl2Wam:`pwd`/Fd2C:`pwd`/TopComp:$PATH
if (! test -f configured); then
  make clean
  ./configure --with-msvc $extra_args
  echo ${1,,} > configured
  make config 
fi
if (! test -f TopComp/gplc); then
  ln -s gplc.exe TopComp/gplc
  ln -s wam2ma.exe Wam2Ma/wam2ma
  ln -s cpp_headers.exe Tools/cpp_headers
fi
make
make install-system

pushd .. &> /dev/null
deploy_dir=`pwd`/deploy/${1,,}
popd &> /dev/null

gprolog_version=`cat Makefile | grep ^PKG_NAME | cut -d = -f 2  | xargs`
cp ./yasm-win64.exe $deploy_dir/$gprolog_version/bin/

echo
echo GProlog installed in $deploy_dir

