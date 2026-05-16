# Sanitizer configuration. Apply via target_link_libraries(<target> PRIVATE gbe_sanitizers)

add_library(gbe_sanitizers INTERFACE)

set(_sanitizers "")

if(GBE_ENABLE_ASAN)
    list(APPEND _sanitizers "address")
endif()

if(GBE_ENABLE_UBSAN)
    list(APPEND _sanitizers "undefined")
endif()

if(GBE_ENABLE_TSAN)
    if(GBE_ENABLE_ASAN)
        message(FATAL_ERROR "TSan cannot be combined with ASan")
    endif()
    list(APPEND _sanitizers "thread")
endif()

if(_sanitizers)
    list(JOIN _sanitizers "," _sanitizers_arg)
    target_compile_options(gbe_sanitizers INTERFACE
        -fsanitize=${_sanitizers_arg}
        -fno-omit-frame-pointer
        -g
    )
    target_link_options(gbe_sanitizers INTERFACE
        -fsanitize=${_sanitizers_arg}
    )
endif()
