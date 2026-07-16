
set(BUILD_TYPE "")



message(STATUS "############# PROJECT_SOURCE_DIR:    " ${PROJECT_SOURCE_DIR}, ${CMAKE_CURRENT_SOURCE_DIR})
# message(STATUS "#############??????? OPYER:    " ${CMAKE_SERVER_ONLY}, ${EXECUTABLE_SERVER_ONLY})
# message(STATUS "# EXPORT_ENCRYPTION_LIBS:    " ${EXPORT_ENCRYPTION_LIBS})





#生成mqttserver可执行文件
file(GLOB SOURCES_server ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/src/payload/*.c)

#add_executable(goblin_exe  ${SOURCES_server}) SHARED STATIC
add_library( goblin_rtp_lib STATIC ${SOURCES_server})

set_target_properties(goblin_rtp_lib PROPERTIES OUTPUT_NAME "goblin_rtp" )

#对特定目标设置编译属性(局部)
#set_target_properties(goblin_lan_lib PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
#target_compile_options(goblin_lan_lib PRIVATE -rdynamic -funwind-tables -Wno-implicit-function-declaration )
target_compile_options(goblin_rtp_lib PUBLIC -Wall -fPIC)

#目标链接的库 -lcrypto -lssl  -lpaho-mqtt3as -lpaho-mqtt3cs
# target_link_libraries(goblin_lan_lib -Wl,--start-group  tuyaos tuyaos_adapter -Wl,--end-group pthread m stdc++)

target_include_directories(goblin_rtp_lib PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include)


install(TARGETS goblin_rtp_lib DESTINATION ${PROJECT_SOURCE_DIR}/output/${PLATFORM}/lib)