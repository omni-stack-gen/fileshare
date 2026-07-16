message(STATUS, "========================== >>>>>> CMakeLists.txt of transport : ${PLATFORM}")
# 编译类型: release/debug
set(BUILD_TYPE "")

# 代码抽取，ON表示根据配置抽取源码到output/qcloud_iot_c_sdk目录
# set(EXTRACT_SRC OFF)

# set(CMAKE_SERVER_ONLY ON)

# option(EXECUTABLE_SERVER_ONLY "Export MQTT EXE" ON)

# option(UDHCPC_CLIENT "Export MQTT EXE" ON)

# option(CAT_CLIENT "Export MQTT EXE" ON)


#message(STATUS "############# PROJECT_SOURCE_DIR:    " ${PROJECT_SOURCE_DIR}, ${CMAKE_CURRENT_SOURCE_DIR})
# message(STATUS "#############??????? OPYER:    " ${CMAKE_SERVER_ONLY}, ${EXECUTABLE_SERVER_ONLY})
message(STATUS "# EXPORT_ENCRYPTION_LIBS:    " ${EXPORT_ENCRYPTION_LIBS})


# if(${EXECUTABLE_SERVER_ONLY} STREQUAL "ON")
#     add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/server)
# else()

# endif()

# if(${UDHCPC_CLIENT} STREQUAL "ON")
#     add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dhcpc)
# endif()


# if(${CAT_CLIENT} STREQUAL "ON")
#     add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/catclient)
# endif()


#生成mqttserver可执行文件
file(GLOB SOURCES_server ${PROJECT_SOURCE_DIR}/src/utils/*.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/event_plc/*.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/spi_plc/*.c)

#add_executable(goblin_lan_lib  ${SOURCES_server})
add_library( goblin_lan_lib SHARED ${SOURCES_server})

set_target_properties(goblin_lan_lib PROPERTIES OUTPUT_NAME "goblin_plc_tran" )

if(${PLATFORM} STREQUAL "V1")
    target_compile_definitions(goblin_lan_lib PRIVATE ENABLE_MEDIA)
    set(media_libs AudioCore)
elseif(${PLATFORM} STREQUAL "V6")
    target_compile_definitions(goblin_lan_lib PRIVATE ENABLE_V6 ENABLE_DTLN)
elseif(${PLATFORM} STREQUAL "F1")
    target_compile_definitions(goblin_lan_lib PRIVATE ENABLE_F1 )
    set(media_libs AudioCore)
else()
    target_compile_definitions(goblin_lan_lib PRIVATE ENABLE_NX5 )
    set(media_libs VideoDec AudioCore)
endif()

#对特定目标设置编译属性(局部)
#set_target_properties(goblin_lan_lib PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
target_compile_options(goblin_lan_lib PRIVATE -rdynamic -funwind-tables -ffunction-sections -fvisibility=hidden )


#目标链接的库 -lcrypto -lssl  -lpaho-mqtt3as -lpaho-mqtt3cs
target_link_libraries(goblin_lan_lib -Wl,--start-group  ${media_libs} -Wl,--end-group pthread m stdc++)

target_include_directories(goblin_lan_lib PUBLIC  ${PROJECT_SOURCE_DIR}/include
                                    ${PROJECT_SOURCE_DIR}/include/utils
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include
                                    ${CMAKE_CURRENT_SOURCE_DIR}/event_plc
                                    ${CMAKE_CURRENT_SOURCE_DIR}/spi_plc
                                    ${CMAKE_CURRENT_SOURCE_DIR}/)
#目标依赖库包含的路径(只对指定目标有效)
target_link_directories(goblin_lan_lib PUBLIC ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib)

install(TARGETS goblin_lan_lib DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/lib)

# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/event_plc/goblin_plc_callcore.h DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/include)
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/event_plc/goblin_plc_process.h DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/include)
# 安装单个头文件
# install(FILES ${PROJECT_SOURCE_DIR}/include/GoblinLanService.h DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/include)

# 安装多个头文件
# install(FILES
#     ${PROJECT_SOURCE_DIR}/include/header1.h
#     ${PROJECT_SOURCE_DIR}/include/header2.h
#     DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/include)

# 安装整个目录下的头文件（递归）
#install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/include FILES_MATCHING PATTERN "*.h" PATTERN "*/" EXCLUDE)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/event_plc/goblin_plc_callcore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/event_plc/goblin_plc_process.h
    DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/include)

#install(FILES ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/lib/libgoblin_plc_tran.so DESTINATION /home/shm/project_A/V1/pack/PLC/pack_p_vcu_a0/app/libs)

# if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
#     message(STATUS "Debug Mode.")
# else()
# # message(STATUS "The strings are not equal.")
# # 添加构建后命令，直接处理构建目录下的文件
# add_custom_command(TARGET goblin_lan_lib POST_BUILD
#     COMMAND ${CMAKE_STRIP} $<TARGET_FILE:goblin_lan_lib>)
# endif()