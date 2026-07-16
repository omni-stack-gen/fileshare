#!/bin/bash
SHELL_FOLDER=$(dirname $(readlink -f "$0"))

# - eq（等于）、-ne（不等于）、-lt（小于）、-le（小于等于）、-gt（大于）、-ge（大于等于）
if [ "$#" -lt "1" ]; then
    echo "Usage: ./cmake_build.sh <clean/all> <platform, eg:NX5/V1>"
    exit 0
elif [ "$#" -ge "2"  -a $1 == "all" ]; then # build clean
    echo "Build all (SDK lib and sample)"
    # rm -rf output/$2/*
    rm -rf build
    rm -rf output/$2/lib/*
    mkdir -p build
    cd build

    if [ "$3" == "Release" ]; then
        echo "Build Release SDK"
        cmake .. -DPLATFORM=$2 -DBUILD_TYPE="Release"
    else
        cmake .. -DPLATFORM=$2
    fi
    #cmake .. -DPLATFORM=$2
    # make VERBOSE=on && make install
    make -j8 && make install

    if [ $2 == "V1" -a "$3" == "Release" ]; then
        cd ..
        echo "Build V1 sample, copy to pack"
        cp output/$2/bin/dp_v1_door /home/shm/project_A/V1/pack/PLC/pack_p_vcu_a0_patch_v1.2/app/dp_v1_door
        cp output/$2/lib/libgoblin_plc_tran.so /home/shm/project_A/V1/pack/PLC/pack_p_vcu_a0_patch_v1.2/app/libs/libgoblin_plc_tran.so
    elif [ $2 == "V6" -a "$4" == "spinand" ]; then
        cd ..
        echo "Build V6 spinand sample, copy to pack"
        cp output/$2/bin/dp_v6_door /home/shm/project_A/V6/pack/pack_spinand_new/pack_src/system/app/dp_v6_door
        cp output/$2/lib/libgoblin_plc_tran.so /home/shm/project_A/V6/pack/pack_spinand_new/pack_src/system/app/libs/libgoblin_plc_tran.so
    elif [ $2 == "V6" -a "$3" == "Release" ]; then
        cd ..
        echo "Build V6 sample, copy to pack"
        cp output/$2/bin/dp_v6_door /home/shm/project_A/V6/pack/pack/pack_src/system/dp_v6_door
        cp output/$2/lib/libgoblin_plc_tran.so /home/shm/project_A/V6/pack/pack/pack_src/system/libs/libgoblin_plc_tran.so
    fi

    echo "Build done" "$#"
    exit 0
elif [ $1 == "make" ]; then
    cd build
    #cmake .. -DPLATFORM=$2
    make -j8 && make install
elif [  $1 == "clean" ]; then # build clean
    echo "Clean sdk"
    rm -rf output/$2/lib/*
    rm -rf output/$2/bin/dp_v1_door
    rm -rf build
    exit 0
elif [  $1 == "export" ]; then # build debug
    echo "Export sdk" $SHELL_FOLDER
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SHELL_FOLDER/output/$2/lib:$SHELL_FOLDER/third_party/$2/lib/mqtt:$SHELL_FOLDER/third_party/$2/lib/openssl
else
    echo "Usage: ./cmake_build.sh <all/debug>"
    exit -1
fi