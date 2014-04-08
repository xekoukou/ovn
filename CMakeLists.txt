cmake_minimum_required (VERSION 2.8)
    project (nestedGraphView)

    set (CMAKE_CXX_FLAGS "-g -Wall")
    set (CMAKE_C_FLAGS "-DTHREADED -g -Wall")

    
    add_executable (ovn_server
	    ovn_server.c
	    ovndb.c
	    )

    target_link_libraries (ovn_server leveldb jansson czmq )
