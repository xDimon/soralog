# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_SANITIZE_MEMORY_CMAKE_)
    return()
else ()
    set(POLLY_FLAGS_SANITIZE_MEMORY_CMAKE_ 1)
endif ()

set(FLAGS
    -fsanitize=memory
    -fsanitize-memory-track-origins
    -g
    )

foreach(FLAG IN LISTS FLAGS)
    add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
    add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()
