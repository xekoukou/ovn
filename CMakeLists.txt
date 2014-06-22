cmake_minimum_required(VERSION 2.8)
    project(ovn)

    set(CMAKE_C_FLAGS "-DTHREADED -g -Wall")

    add_subdirectory(lib_sha512)
    include_directories("${PROJECT_SOURCE_DIR}/lib_sha512")

    add_executable(ovndb_create ovndb_create.c)
    target_link_libraries(ovndb_create cassandra)

    add_executable(ovn_server ovndb.c ovn_server.c )
    target_link_libraries(ovn_server cassandra jansson czmq lib_sha512)
