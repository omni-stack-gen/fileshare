
set(BUILD_TYPE "")



#message(STATUS "############# PROJECT_SOURCE_DIR:    " ${PROJECT_SOURCE_DIR}, ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "#############??????? OPYER:    " ${CMAKE_SERVER_ONLY}, ${EXECUTABLE_SERVER_ONLY})
message(STATUS "# EXPORT_ENCRYPTION_LIBS:    " ${EXPORT_ENCRYPTION_LIBS})





#生成mqttserver可执行文件
file(GLOB SOURCES_server ${CMAKE_CURRENT_SOURCE_DIR}/*.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/linux/*.c)

add_executable(goblin_exe  ${SOURCES_server})
#add_library( goblin_lan_lib SHARED ${SOURCES_server})

set_target_properties(goblin_exe PROPERTIES OUTPUT_NAME "goblin_tuya" )

#对特定目标设置编译属性(局部)
#set_target_properties(goblin_lan_lib PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
target_compile_options(goblin_exe PRIVATE -rdynamic -funwind-tables -Wno-implicit-function-declaration )

#目标链接的库 -lcrypto -lssl  -lpaho-mqtt3as -lpaho-mqtt3cs
target_link_libraries(goblin_exe -Wl,--start-group  tuyaos tuyaos_adapter -Wl,--end-group pthread m stdc++)

target_include_directories(goblin_exe PUBLIC  ${PROJECT_SOURCE_DIR}/include
                                    ${PROJECT_SOURCE_DIR}/tuya
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/include/tuya)
target_link_directories(goblin_exe PUBLIC ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib
                                    ${PROJECT_SOURCE_DIR}/third_party/${PLATFORM}/lib/tuya)

install(TARGETS goblin_exe DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/bin)