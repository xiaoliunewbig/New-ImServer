cmake_minimum_required(VERSION 3.10)

# 设置CMake策略
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

project(im_server)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找必要的包
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread filesystem program_options)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)
find_package(absl REQUIRED)

# 如果没有找到 abseil 配置，尝试使用 pkg-config
if(NOT absl_FOUND)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(ABSL QUIET absl_base absl_strings absl_synchronization absl_time)
    endif()
endif()

# 检查是否找到了 abseil
if(absl_FOUND OR ABSL_FOUND)
    message(STATUS "Found Abseil library")
else()
    message(WARNING "Abseil library not found. Linking may fail if gRPC requires it.")
endif()

# 手动查找MySQL库
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h
    /usr/include
    /usr/local/include
    /usr/mysql/include
    /usr/local/mysql/include
)
find_library(MYSQL_LIBRARY NAMES mysqlclient
    PATHS
    /usr/lib
    /usr/local/lib
    /usr/mysql/lib
    /usr/local/mysql/lib
)
if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
    set(MYSQL_FOUND TRUE)
    message(STATUS "Found MySQL: ${MYSQL_LIBRARY}")
else()
    message(FATAL_ERROR "MySQL not found. Install libmysqlclient-dev or specify paths manually.")
endif()

# 查找Redis库
find_path(HIREDIS_HEADER hiredis/hiredis.h)
find_library(HIREDIS_LIB hiredis)

# 设置gRPC和Protobuf相关变量
set(GRPC_CPP_PLUGIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# 生成Protocol Buffers和gRPC代码
set(PROTO_FILES 
    proto/service.proto
)
set(GENERATED_PROTO_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(PROTO_SRCS
        "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.cc"
        "${GENERATED_PROTO_DIR}/${PROTO_NAME}.grpc.pb.cc"
    )
    set(PROTO_HDRS
        "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.h"
        "${GENERATED_PROTO_DIR}/${PROTO_NAME}.grpc.pb.h"
    )
    add_custom_command(
        OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS
            --proto_path=${CMAKE_SOURCE_DIR}/proto
            --cpp_out=${GENERATED_PROTO_DIR}
            --grpc_out=${GENERATED_PROTO_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            ${CMAKE_SOURCE_DIR}/${PROTO_FILE}
        DEPENDS ${CMAKE_SOURCE_DIR}/${PROTO_FILE}
        COMMENT "Generating Protocol Buffers and gRPC code for ${PROTO_FILE}"
    )
endforeach()

# 创建符号链接目标
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/proto/service.pb.h ${CMAKE_BINARY_DIR}/proto/service.grpc.pb.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/proto
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${GENERATED_PROTO_DIR}/service.pb.h ${CMAKE_BINARY_DIR}/proto/service.pb.h
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${GENERATED_PROTO_DIR}/service.grpc.pb.h ${CMAKE_BINARY_DIR}/proto/service.grpc.pb.h
    DEPENDS ${GENERATED_PROTO_DIR}/service.pb.h ${GENERATED_PROTO_DIR}/service.grpc.pb.h
    COMMENT "Creating symbolic links for protobuf headers"
)
add_custom_target(proto_symlinks DEPENDS ${CMAKE_BINARY_DIR}/proto/service.pb.h ${CMAKE_BINARY_DIR}/proto/service.grpc.pb.h)

# 设置头文件搜索路径
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}  # 添加构建目录到包含路径
    ${GENERATED_PROTO_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIR}
    ${HIREDIS_HEADER}
    ${OPENSSL_INCLUDE_DIR}
)

# 添加所有源文件
file(GLOB_RECURSE SERVER_SOURCES
    src/server/*.cpp
    src/server/db/*.cpp
    src/server/utils/*.cpp
    src/server/kafka/*.cpp
)

# 添加可执行文件
add_executable(im_server 
    ${SERVER_SOURCES}
    ${PROTO_SRCS}
)

# 添加依赖关系
add_dependencies(im_server proto_symlinks)

# 链接库（注意顺序）
target_link_libraries(im_server
    # 应用库
    ${MYSQL_LIBRARY}
    ${HIREDIS_LIB}
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
    dl
    
    # Abseil 库
    absl::check
    absl::log
    absl::strings
    absl::str_format
    absl::base
    absl::log_internal_message
    absl::log_internal_check_op
    absl::log_internal_conditions
    absl::log_internal_format
    absl::log_internal_globals
    absl::log_internal_proto
    absl::log_internal_nullguard
    absl::log_internal_log_sink_set
    
    # Protobuf 和 gRPC（移除了 reflection）
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)

# 如果找到了 abseil，添加链接
if(absl_FOUND)
    target_link_libraries(im_server
        absl::base
        absl::strings
        absl::synchronization
        absl::time
    )
elseif(ABSL_FOUND)
    target_link_libraries(im_server
        ${ABSL_LIBRARIES}
    )
else()
    # 尝试直接链接系统库
    target_link_libraries(im_server
        -labsl_base
        -labsl_synchronization
        -labsl_strings
        -labsl_time
        -labsl_raw_logging_internal
        -labsl_throw_delegate
        -labsl_log_severity
        -labsl_spinlock_wait++
        -labsl_log_internal_check_op
        -labsl_log_internal_message
        -labsl_log_internal_conditions
        -labsl_log_internal_format
        -labsl_log_internal_globals
        -labsl_log_internal_proto
        -labsl_log_internal_nullguard
        -labsl_log_globals
        -labsl_log_initialize
    )
endif()

# 添加Kafka库依赖
find_library(RDKAFKA_LIBRARY rdkafka)
find_library(RDKAFKA_CPP_LIBRARY rdkafka++)
if(RDKAFKA_LIBRARY AND RDKAFKA_CPP_LIBRARY)
    target_link_libraries(im_server ${RDKAFKA_LIBRARY} ${RDKAFKA_CPP_LIBRARY})
endif()

# 安装目标
install(TARGETS im_server DESTINATION bin)
install(FILES conf/server.json DESTINATION etc/im_server)

# 创建必要的目录
install(CODE "file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/var/im_server/logs)")
install(CODE "file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/var/im_server/uploads)")

# 设置 CPack 配置
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)