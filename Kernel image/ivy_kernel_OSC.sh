#!/bin/bash
ROOT_DIR=`pwd`
qwe=1
name=scriptlog"$qwe".txt
dt_str=`date +"%Y%m%d"`
LOG_FILE=ivykenel_"$dt_str"_scriptlog"$qwe".txt
function do_main(){
	date
	echo -e " \n$ROOT_DIR > pwd"
	pwd
	echo -e "\n$ROOT_DIR > ls"
	ls
	if [ ! -d "ivy_kernel_OSC" ]; then
	    echo -e "\n$ROOT_DIR > tar -xvf ivy_kernel_OSC.tar.gz"
	    tar -xvf ivy_kernel_OSC.tar.gz
	    echo -e ""
	else
	    echo "$ROOT_DIR >  ----------------  ivy_kernel_OSC.tar.gz  has been decompressed-------------------"
	fi
	echo -e "\n$ROOT_DIR > cd ivy_kernel_OSC"
	cd ivy_kernel_OSC
	ROOT_DIR=`pwd`
	echo -e "\n$ROOT_DIR > ls  -F"
	ls  -F
	echo -e "\n$ROOT_DIR > "
	date
	echo -e "\n$ROOT_DIR > cat README_instructions.txt"
	cat README_instructions.txt
	echo -e "\n$ROOT_DIR > ls -l build-kernel.sh"
	ls -l build-kernel.sh
	echo -e "\n$ROOT_DIR > cat build-kernel.sh"
	cat build-kernel.sh
	echo -e "\n$ROOT_DIR > ls -f arm-linux-androideabi-4.9"
	ls -f arm-linux-androideabi-4.9
	
	if [ ! -d "out" ]; then
	    echo -e "\n$ROOT_DIR > ./build-kernel.sh"
	    ./build-kernel.sh
	else
	    echo -e "\n$ROOT_DIR > ./build-kernel.sh"
	    echo -e "\n$ROOT_DIR > out is existed and will delete"
	    echo -e "\n$ROOT_DIR > rm -rf out/"
	    rm -rf out/
	    ./build-kernel.sh
	fi
	echo -e "\n$ROOT_DIR > ls -F"
	ls -F
	echo -e "\n$ROOT_DIR > cd out"
	cd out
	ROOT_DIR=`pwd`
	echo -e "\n$ROOT_DIR > ls -F"
	ls -F
	echo -e "\n$ROOT_DIR > find . | grep -i image"
	find . | grep -i image
	echo -e "\n$ROOT_DIR > cd arch/arm/boot/"
	cd arch/arm/boot/
	ROOT_DIR=`pwd`
	echo -e "\n$ROOT_DIR > ls -F"
	ls -F
	echo -e "\n$ROOT_DIR > ls -lF"
	ls -lF
	echo -e "\n$ROOT_DIR > cksum *"
	cksum *
	echo -e "\n$ROOT_DIR > exit"
	date
}

# start
while true
do
    if [ ! -f  *"$name" ]; then
        do_main $@ | tee $LOG_FILE 
        exit
    else
        let qwe++
        name=scriptlog"$qwe".txt
        LOG_FILE=ivykenel_"$dt_str"_scriptlog"$qwe".txt
    fi
done
