cmake_minimum_required(VERSION 3.0)

project(test)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer $ENV{CXXFLAGS} -lcrypto -rdynamic -O3 -fPIC -ggdb -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations -Werror=unused-variable -Werror=sign-compare")


# set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer $ENV{CXXFLAGS}  -lcrypto   -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations  -Werror=unused-variable  -Werror=unused-variable -Werror=sign-compare")

include_directories(.)
include_directories(./code)
# Include directories
include_directories(${PCAP_INCLUDE_DIRS})

set(LIB_SRC
    # code/model.cc
    code/kcp/ikcp.cc
    code/kcp/kcp_socket.cc
    code/kcp/kcp_server.cc
    code/kcp/kcp_client.cc
    code/kcp/server_epoll.cc
    )

add_library(hzh SHARED ${LIB_SRC})

set(LIB_LIB
    hzh 
    dl
    ${PCAP_LIBRARIES}
)

# set(EXEC_SRC
#     tests/test_kcp.cpp
# )

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server hzh)
target_link_libraries(test_tcp_server ${LIB_LIB})# 设置编译选项

add_executable(test_tcp_client tests/test_tcp_client.cc)
add_dependencies(test_tcp_client hzh)
target_link_libraries(test_tcp_client ${LIB_LIB})# 设置编译选项

add_executable(server_qps tests/fake_tcp_epoll.cc)
add_dependencies(server_qps hzh)
target_link_libraries(server_qps ${LIB_LIB})# 设置编译选项

add_executable(kcp_qps tests/test_kcp_qps.cc)
add_dependencies(kcp_qps hzh)
target_link_libraries(kcp_qps ${LIB_LIB})# 设置编译选项

# add_executable(test tests/test_kcp_client.cc)
# add_dependencies(test hzh)
# target_link_libraries(test ${LIB_LIB})# 设置编译选项

# add_executable(client tests/test_client.cpp)
# add_dependencies(client hzh)
# target_link_libraries(client ${LIB_LIB})# 设置编译选项

# add_executable(server tests/test_server.cpp)
# add_dependencies(server hzh)
# target_link_libraries(server ${LIB_LIB})# 设置编译选项

# add_executable(fake_tcp tests/test_fake_tcp.cc)
# add_dependencies(fake_tcp hzh)
# target_link_libraries(fake_tcp ${LIB_LIB})# 设置编译选项


# add_executable(test_b tests/test_tcp_client.cc)
# add_dependencies(test_b hzh)
# target_link_libraries(test_b ${LIB_LIB})# 设置编译选项

# add_executable(test_kcp_a tests/test_kcp_a.cpp)
# add_dependencies(test_kcp_a hzh)
# target_link_libraries(test_kcp_a ${LIB_LIB})# 设置编译选项

# add_executable(test_kcp_b tests/test_kcp_b.cpp)
# add_dependencies(test_kcp_b hzh)
# target_link_libraries(test_kcp_b ${LIB_LIB})# 设置编译选项





# add_executable(b tests/b.cpp)
# add_dependencies(b hzh)
# target_link_libraries(b ${LIB_LIB})# 设置编译选项


# add_executable(test_sm4 tests/test.cpp)
# add_dependencies(test_sm4 hzh)
# target_link_libraries(test_sm4 ${LIB_LIB})# 设置编译选项




SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)