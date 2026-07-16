#设置CMake最小版本要求
cmake_minimum_required(VERSION 3.5)
#定义项目名称和类型C CXX
project(goblin_pcl )
#设置目标系统名称
set(CMAKE_SYSTEM_NAME Linux)
#设置平台变量
set(PLATFORM "X9" CACHE STRING "")

set(BUILD_TYPE "Debug" CACHE STRING "")

#是否开启详细输出 编译的整个流程打印
set(CMAKE_VERBOSE_MAKEFILE OFF)

option(EXECUTABLE_TUYA_ONLY "Export tuya" OFF)

option(EXECUTABLE_RTP_LIB "Expirt RTP" OFF)

#设置编译标志 自定义的
#set(my_flag2 -Wno-error=sign-compare -Wno-error=format -Wno-unused-variable -Wno-address -fvisibility=hidden -Wfatal-errors -fdata-sections -rdynamic -funwind-tables -ffunction-sections -Wl,--gc-sections -std=gnu99)


#添加预处理器定义(全局) -D_GNU_SOURCE
#add_definitions(-g -Werror  -D${PLATFORM}  -D_GNU_SOURCE)
#设置编译选项(全局)
#add_compile_options(-Wall -Wextra)

if(${EXECUTABLE_RTP_LIB} STREQUAL "OFF")
#分别为C和C++编译器设置编译标志，如果是变量要字符串隔开(参考my_flag) 2> warnings.log -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILE_FLAG} -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11  -Wall -Werror ")
endif()

# 编译类型: Debug/Release/MinSizeRel/RelWithDebInfo
set(CMAKE_BUILD_TYPE ${BUILD_TYPE})
# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2"  )
# set(CMAKE_CXX_FLAGS_RELEASE "-O2 -std=c++11 ")
# set(CMAKE_C_FLAGS_DEBUG "-g2"  )
# set(CMAKE_CXX_FLAGS_DEBUG "-g2 -std=c++11 ")

# 定义平台相关设置
set(PLATFORM_SETTINGS "X9;V3;NX5;F1;V1;V6")

# 检查 PLATFORM 是否在支持的列表中
if(NOT PLATFORM IN_LIST PLATFORM_SETTINGS)
    message(FATAL_ERROR "PLATFORM ${PLATFORM} is not supported. Supported platforms are: ${PLATFORM_SETTINGS}")
endif()


#设置编译器链接库的路径 根据 PLATFORM变量的值，设置不同平台的库路径(全部有效)
# link_directories(${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib)

#设置编译器路径 据 PLATFORM变量的值，设置不同平台的 编译器路径
if(${PLATFORM} STREQUAL "X9")
    set(CMAKE_C_COMPILER "/opt/X9/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc")
    set(CMAKE_CXX_COMPILER "/opt/X9/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++")
elseif(${PLATFORM} STREQUAL "V3")
    set(CMAKE_C_COMPILER /opt/V3/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER /opt/V3/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++)
elseif(${PLATFORM} STREQUAL "NX5")
    set(CMAKE_C_COMPILER /opt/X5/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc)
    set(CMAKE_CXX_COMPILER /opt/X5/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-g++)
elseif(${PLATFORM} STREQUAL "F1")
    set(CMAKE_C_COMPILER /opt/F1/bin/riscv64-unknown-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER /opt/F1/bin/riscv64-unknown-linux-gnu-g++)
    set(CMAKE_STRIP /opt/F1/bin/riscv64-unknown-linux-gnu-strip)

elseif(${PLATFORM} STREQUAL "V1")
    set(CMAKE_C_COMPILER /opt/V1/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-gcc)
    set(CMAKE_CXX_COMPILER /opt/V1/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-g++)
    set(CMAKE_STRIP /opt/V1/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-strip)
elseif(${PLATFORM} STREQUAL "V6")
    set(CMAKE_C_COMPILER /opt/V6/arm-fhva12c-linux-uclibcgnueabihf-b6/bin/arm-fhva12c-linux-uclibcgnueabihf-gcc)
    set(CMAKE_CXX_COMPILER /opt/V6/arm-fhva12c-linux-uclibcgnueabihf-b6/bin/arm-fhva12c-linux-uclibcgnueabihf-g++)
    set(CMAKE_STRIP /opt/V6/arm-fhva12c-linux-uclibcgnueabihf-b6/bin/arm-fhva12c-linux-uclibcgnueabihf-strip)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_EXTENSIONS ON)  # 启用 GNU 扩展
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4")
else()
    message(FATAL_ERROR "PLATFORM ${PLATFORM} NOT SUPPORTED", ${PLATFORM})
endif()

# foreach(PLATFORM ${PLATFORM_SETTINGS})
#     if(${PLATFORM} STREQUAL "X9")
#         set(PLATFORM_C_COMPILE "/opt/X9/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc")
#         set(PLATFORM_CXX_COMPILE "/opt/X9/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++")
#         set(PLATFORM_SOURCE_PATTERNS "*.c;src/base/*.c;src/base/sys/*.c;src/dpdata/*.c;src/utils/*.c")
#     elseif(${PLATFORMP} STREQUAL "V3")
#         set(PLATFORM_C_COMPILE "/opt/V3/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc")
#         set(PLATFORM_CXX_COMPILE "/opt/V3/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++")
#         set(PLATFORM_SOURCE_PATTERNS "*.c;src/base/*.c;src/base/sys/*.c;src/dpdata/*.c;src/utils/*.c")
#     elseif(${PLATFORM} STREQUAL "NX5")
#         set(PLATFORM_C_COMPILE "/opt/X5/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc")
#         set(PLATFORM_CXX_COMPILE "/opt/X5/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-g++")
#         set(PLATFORM_SOURCE_PATTERNS "*.c;src/base/*.c;src/base/sys/*.c;src/dpdata/*.c;src/utils/*.c")
#     elseif(${PLATFORM} STREQUAL "V1")
#         set(PLATFORM_C_COMPILE "/opt/V1/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-gcc")
#         set(PLATFORM_CXX_COMPILE "/opt/V1/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-g++")
#         set(PLATFORM_SOURCE_PATTERNS "*.c;src/media/*.c;src/base/*.c;src/base/sys/*.c;src/dpdata/*.c;src/periphery/*.c;src/service/*.c;src/sound/*.c;src/utils/*.c")
#         set(PLATFORM_STRIP "/opt/V1/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-strip")
#     else()
#         message(FATAL_ERROR "PLATFORM ${PLATFORM} NOT SUPPORTED", ${PLATFORM})
#     endif()
#     # 存储平台相关设置
#     set(PLATFORM_${PLATFORM}_C_COMPILE ${PLATFORM_C_COMPILE})
#     set(PLATFORM_${PLATFORM}_CXX_COMPILE ${PLATFORM_CXX_COMPILE})
#     set(PLATFORM_${PLATFORM}_SOURCE_PATTERNS ${PLATFORM_SOURCE_PATTERNS})
#     if(DEFINED PLATFORM_STRIP)
#         set(PLATFORM_${PLATFORM}_STRIP ${PLATFORM_STRIP})
#     endif()
# endforeach()

# message(STATUS "============= PLATFORM Env =============", ${PLATFORM}, ${PLATFORM_C_COMPILE})
# # 根据PLATFORM变量设置实际的编译和源文件查找
# if(PLATFORM)
#     set(CMAKE_C_COMPILER ${PLATFORM_${PLATFORM}_C_COMPILE})
#     set(CMAKE_CXX_COMPILER ${PLATFORM_${PLATFORM}_CXX_COMPILE})
#     #file(GLOB SOURCES ${PLATFORM_${PLATFORM}_SOURCE_PATTERNS})
#     if(DEFINED PLATFORM_${PLATFORM}_STRIP)
#         set(CMAKE_STRIP ${PLATFORM_${PLATFORM}_STRIP})
#     endif()
# else()
#     message(FATAL_ERROR "PLATFORM variable is not set")
# endif()

#编译环境信息再次使用message(STATUS ...)打印CMake版本、CPU信息、操作系统信息等，以供调试和记录
message(STATUS "")
message(STATUS "============= Compile Env =============")
message(STATUS "CMake Version: " ${CMAKE_VERSION})
message(STATUS " CPU Info:     " ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "  OS Info:     " ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_VERSION})
message(STATUS "Host Info:     " ${CMAKE_HOST_SYSTEM_NAME})
message(STATUS "C COMPILER:    " ${CMAKE_C_COMPILER_ID}-${CMAKE_C_COMPILER_VERSION})
message(STATUS " CMAKE_BUILD_TYPE:    " ${CMAKE_BUILD_TYPE})

message(STATUS " PROJECT_SOURCE_DIR:    " ${PROJECT_SOURCE_DIR})
message(STATUS " CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "============= Compile Env =============")
message(STATUS "")

#if(NOT ${PLATFORM} STREQUAL "V6")
#编译通讯库
add_subdirectory(${PROJECT_SOURCE_DIR}/src/transport)
#endif()

if(${PLATFORM} STREQUAL "V1")
#自动查找源文件 GLOB_RECURSE：递归查找对应目录及下级目录  GLOB：对应目录
#使用file(GLOB_RECURSE ...)命令自动查找src目录下所有的.c文件，并赋值给SOURCES变量。
#file(GLOB_RECURSE SOURCES src/*.c)
file(GLOB SOURCES *.c src/media/*.c
                    src/base/*.c
                    src/base/sys/*.c
                    src/dpdata/*.c
                    src/periphery/*.c
                    src/service/*.c
                    src/sound/*.c
                    src/dtln/*.cc
                    src/utils/*.c
                    src/watchdog/*.c)

set(ENV{STAGING_DIR} "/path/to/your/staging/directory")

#通过SET_SOURCE_FILES_PROPERTIES为src/specific_file.c设置了-Wno-all编译选项，覆盖了全局的-Wall设置，使其不生效。
set_source_files_properties(src/media/video_encode.c PROPERTIES COMPILE_FLAGS "-Wno-all")

add_executable(dp_exe  ${SOURCES})

#设置目标属性 OUTPUT_NAME： 设置名称
set_target_properties(dp_exe PROPERTIES OUTPUT_NAME "dp_v1_door")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "Debug Mode define")
    target_compile_definitions(dp_exe PRIVATE ENABLE_DEBUG ENABLE_MEDIA NB_BETA_3_0_1=1 ENABLE_V1 )
else()
#特定目标添加预处理器定义
    target_compile_definitions(dp_exe PRIVATE ENABLE_MEDIA NB_BETA_3_0_1=1 ENABLE_V1 )
endif()
#对特定目标设置编译属性(局部)
#target_compile_options(dp_exe PRIVATE -rdynamic -funwind-tables -ffunction-sections)
target_compile_options(dp_exe PRIVATE -fno-strict-aliasing)

#目标依赖库包含的路径(只对指定目标有效)
target_link_directories(dp_exe PUBLIC ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib
                                ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib/
                                ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib/libs
                                ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib/libs/eyesee-mpp
                                ${PROJECT_SOURCE_DIR}/build/src/transport/)

#crypto ssl awnn_full
set(media_libs  z log awion aw_mpp media_utils MemAdapter VE cdc_base cedarxstream cdx_common cdx_base Resample ISP isp_dev isp_ini  isp_ae  isp_af
                isp_afs isp_awb isp_base isp_gtm isp_iso isp_math isp_md isp_pltm isp_rolloff vencoder venc_base
                ffavutil awnn_full ev dplib_rtsp venc_h264 venc_h265 venc_jpeg venc_common
                glog expat  AudioCore mp3dec ota-burnboot tensorflow-lite)

#目标链接的库
target_link_libraries(dp_exe  -Wl,--start-group ${media_libs} goblin_plc_tran -Wl,--end-group pthread m stdc++)

#目标头文件包含的头文件路径
target_include_directories(dp_exe PUBLIC  ${PROJECT_SOURCE_DIR}/include
                                    ${PROJECT_SOURCE_DIR}/include/base
                                    ${PROJECT_SOURCE_DIR}/include/service
                                    ${PROJECT_SOURCE_DIR}/include/utils
                                    ${PROJECT_SOURCE_DIR}/src
                                    ${PROJECT_SOURCE_DIR}/src/base
                                    ${PROJECT_SOURCE_DIR}/src/service
                                    ${PROJECT_SOURCE_DIR}/src/sound
                                    ${PROJECT_SOURCE_DIR}/src/dtln
                                    ${PROJECT_SOURCE_DIR}/src/transport/event_plc
                                    ${PROJECT_SOURCE_DIR}/src/watchdog
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/audionn
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/audionn/tensorflow
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/system/public/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/system/public/include/utils
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/sample/configfileparser
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/include/utils
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/include/media
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/include/media/utils
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/include/component
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/include/utils
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/LIBRARY/libisp/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/LIBRARY/libisp/isp_tuning
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/LIBRARY/libisp/include/V4l2Camera
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/LIBRARY/libISE/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/LIBRARY/libcedarc/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/LIBRARY/agc_lib/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/LIBRARY/libisp
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/middleware/media/LIBRARY/libisp/include/device
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/eyesee-mpp/system/public/include/utils)

# 安装
install(TARGETS dp_exe DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/bin)

#install(FILES ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/bin/dp_v1_door DESTINATION /home/shm/project_A/V1/pack/PLC/pack_p_vcu_a0/app)
#file(COPY ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/bin/dp_v1_door DESTINATION /home/shm/project_A/V1/pack/PLC/pack_p_vcu_a0/app)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "Debug Mode.")
else()
# message(STATUS "The strings are not equal.")
# 添加构建后命令，直接处理构建目录下的文件
add_custom_command(TARGET dp_exe POST_BUILD
    #COMMAND ${CMAKE_STRIP} ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/bin/dp_v1_door
    COMMAND ${CMAKE_STRIP} $<TARGET_FILE:dp_exe>
    COMMENT "Stripping symbols from app0"
    COMMENT $<TARGET_FILE:dp_exe>)
endif()

#V6 编译选项 if(${PLATFORM} STREQUAL "V6")
elseif(${PLATFORM} STREQUAL "V6")

file(GLOB SOURCES main.c application.c src/base/*.c
                        src/base/sys/*.c
                        src/dpdata/*.c
                        src/utils/*.c
                        src/media_v6/*.c
                        src/media_v6/*.cpp
                        src/media_v6/libdmc/*.c
                        src/media_v6/librtsp/*.c
                        src/media_v6/media/*.c
                        src/media_v6/vlcview/*.c
                        src/periphery/*.c
                        src/service/*.c
                        src/sound/*.c
                        src/dtln/*.cc
                        src/watchdog/*.c)

add_executable(dp_exe  ${SOURCES})

set_target_properties(dp_exe PROPERTIES OUTPUT_NAME "dp_v6_door")

#set_target_properties(dp_exe PROPERTIES PREFIX "PREFIX2" SUFFIX "---")

#ENABLE_DTLN 回声降噪
#ENABLE_NS_NN 单降噪
#ENABLE_AOV AOV输出唤醒
target_compile_definitions(dp_exe PRIVATE ENABLE_V6 ENABLE_DTLN ENABLE_NS_NN ENABLE_AOV2 ENABLE_SPINAAD SAMPLE_NPU_TEST2)

target_compile_options(dp_exe PRIVATE -fno-strict-aliasing -funwind-tables -g -rdynamic)

#target_compile_features(dp_exe PRIVATE cxx_std_11)

target_include_directories(dp_exe PUBLIC  ${PROJECT_SOURCE_DIR}/include
                                    ${PROJECT_SOURCE_DIR}/include/base
                                    ${PROJECT_SOURCE_DIR}/include/service
                                    ${PROJECT_SOURCE_DIR}/include/utils
                                    ${PROJECT_SOURCE_DIR}/include/media_v6
                                    ${PROJECT_SOURCE_DIR}/include/media_v6/dmc
                                    ${PROJECT_SOURCE_DIR}/include/media_v6/rtsp
                                    ${PROJECT_SOURCE_DIR}/src/media_v6/vlcview
                                    ${PROJECT_SOURCE_DIR}/src/base
                                    ${PROJECT_SOURCE_DIR}/src
                                    ${PROJECT_SOURCE_DIR}/src/service
                                    ${PROJECT_SOURCE_DIR}/src/sound
                                    ${PROJECT_SOURCE_DIR}/src/dtln
                                    ${PROJECT_SOURCE_DIR}/src/transport/event_plc
                                    ${PROJECT_SOURCE_DIR}/src/watchdog
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/codec
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/isp
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/dsp
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/libz
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/ai_video
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/audionn
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/audionn/tensorflow)

target_link_directories(dp_exe PUBLIC ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib
                                ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib/static
                                ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib/NS
                                ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib/NPU
                                ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib/ai_video
                                ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/lib
                                ${PROJECT_SOURCE_DIR}/build/src/transport)

#mpi 依赖 audio_codec fhses tensorflow-lite
#set(aec_libs audio_aec_core farmhash fft2d flatbuffers ruy tensorflow-lite)
#set(npu_libs hcp_npu hcp_model dcl_runtime hcp_sys stream_sched dcl_utils drv_dp2000 dcl_ioctl mdl_parse)
#set(dtln_libs  tensorflow-lite) #DTLN
set(ns_libs audio_ns_core farmhash fft2d ruy flatbuffers  tensorflow-lite) # NS
set(media_libs advapi_smartir mpi audio_codec fhses gc4653_mipi advapi_isp isp ispcore sensor acw_mpi AudioCore advapi  z) #static
#set(media_rpc_libs advapi_smartir dbi.arc mpi audio_codec fhses  isprpc_leader sensor_prg acw_mpi AudioCore advapi  z) #rpc
target_link_libraries(dp_exe PUBLIC -Wl,--start-group ${media_libs} ${media_rpc_libs} ${aec_libs} ${npu_libs} ${dtln_libs} ${ns_libs} VideoAI goblin_plc_tran mp3dec -Wl,--end-group rt pthread m stdc++)

install(TARGETS dp_exe DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/bin)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "Debug Mode.")
else()
# message(STATUS "The strings are not equal.")
# 添加构建后命令，直接处理构建目录下的文件
add_custom_command(TARGET dp_exe POST_BUILD
    #COMMAND ${CMAKE_STRIP} ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/bin/dp_v1_door
    COMMAND ${CMAKE_STRIP} $<TARGET_FILE:dp_exe>
    COMMENT "Stripping symbols from app0"
    COMMENT $<TARGET_FILE:dp_exe>)
endif()

#X9 编译选项 if(${PLATFORM} STREQUAL "X9") F1 不编译应用
elseif(NOT ${PLATFORM} STREQUAL "F1")

file(GLOB SOURCES *.c src/base/*.c src/base/sys/*.c src/dpdata/*.c src/utils/*.c)

add_executable(dp_exe  ${SOURCES})

set_target_properties(dp_exe PROPERTIES OUTPUT_NAME "dp_test")

#set_target_properties(dp_exe PROPERTIES PREFIX "PREFIX2" SUFFIX "---")

target_compile_definitions(dp_exe PRIVATE ENABLE_NX5 ENABLE_DEBUG)

target_include_directories(dp_exe PUBLIC  ${PROJECT_SOURCE_DIR}/include
                                    ${PROJECT_SOURCE_DIR}/include/base
                                    ${PROJECT_SOURCE_DIR}/include/service
                                    ${PROJECT_SOURCE_DIR}/include/utils
                                    ${PROJECT_SOURCE_DIR}/src/base
                                    ${PROJECT_SOURCE_DIR}/src
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include)

target_link_directories(dp_exe PUBLIC ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib
                                ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/lib
                                ${PROJECT_SOURCE_DIR}/build/src/transport)

target_link_libraries(dp_exe PUBLIC -Wl,--start-group goblin_lan_lib AudioCore VideoDec -Wl,--end-group rt pthread m stdc++)

install(TARGETS dp_exe DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/bin)
endif()
#install(TARGETS ${PROJECT_SOURCE_DIR}/include/GoblinLanService.h DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/include)
# 头文件安装路径

#file(COPY ${PROJECT_SOURCE_DIR}/include/GoblinLanService.h DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/include)


##server
# file(GLOB SOURCES_server src/server/*.c)
# add_executable(mqttserver  ${SOURCES_server})

# install(TARGETS mqttserver DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/bin)

#添加子目录
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/transport)


if(${EXECUTABLE_TUYA_ONLY} STREQUAL "ON")
    #add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/server)
    add_subdirectory(${PROJECT_SOURCE_DIR}/tuya)
else()

endif()


if(${EXECUTABLE_RTP_LIB} STREQUAL "ON")
    #add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/server)
    add_subdirectory(${PROJECT_SOURCE_DIR}/RTP)
endif()
