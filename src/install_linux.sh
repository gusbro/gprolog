
if  [ $# -eq 0 ] || [ "${1,,}" != "debug" ] && [ "${1,,}" != "release" ]
  then
    echo "Usage: $@ (Debug|Release)"
    exit 1
fi

if [ "${1,,}" = "debug" ]; then
  extra_args=(--with-c-flags="-fPIC -g -Wall" LDFLAGS=-fPIC CFLAGS_ENGINE1="-C -fPIC" --prefix="`pwd`/../deploy/debug")
else
  extra_args=(--with-c-flags="-fPIC -Wall" LDFLAGS=-fPIC CFLAGS_ENGINE1="-C -fPIC" --prefix="`pwd`/../deploy/release")
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

gprolog_version=`cat Makefile | grep ^PKG_NAME | cut -d = -f 2  | xargs`
mkdir -p $deploy_dir/$gprolog_version/include/Tools &> /dev/null
mkdir -p $deploy_dir/$gprolog_version/include/EnginePl &> /dev/null
cp ./tools/hash_fct.h $deploy_dir/$gprolog_version/include/Tools/

# Extra include files to copy
files_to_copy_engine=("arch_dep.h" "atom.h" "bool.h" "engine.h" "engine_pl.h" "gp_config.h" "hash.h" "if_no_fd.h" "machine.h" "machine1.h" "misc.h" "obj_chain.h" "oper.h" "pl_long.h" "pl_params.h" "pred.h" "stacks_sigsegv.h" "wam_archi.h" "wam_inst.h" "wam_regs.h" "wam_stacks.h")
for file in "${files_to_copy_engine[@]}"; do
  cp "./EnginePl/$file" "$deploy_dir/$gprolog_version/include/EnginePl/"
done

echo
echo GProlog installed in $deploy_dir

