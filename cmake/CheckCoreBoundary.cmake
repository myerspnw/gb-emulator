# Asserts that nothing under CORE_DIR transitively reaches the frontend,
# the SDL3 toolkit, or a logging library. The architecture's "core does
# no I/O" invariant (see docs/architecture.md and ADR-002) is enforced
# here as a build-system check so that an accidental cross-boundary
# include fails locally and in CI rather than just compiling because
# the offending header happens to be string-only today.
#
# Invoke as a CMake script:
#   cmake -DCORE_DIR=<path> -P CheckCoreBoundary.cmake

if(NOT DEFINED CORE_DIR)
    message(FATAL_ERROR "CheckCoreBoundary.cmake: -DCORE_DIR=<path> required")
endif()

file(GLOB_RECURSE _core_sources
    LIST_DIRECTORIES FALSE
    "${CORE_DIR}/*.cpp"
    "${CORE_DIR}/*.hpp"
)

# Headers the core is forbidden to include. Extend deliberately —
# every entry is a load-bearing piece of the architecture's
# dependency graph, not a style preference.
set(_forbidden_patterns
    "frontend/"      # cross-boundary into the SDL/host shell
    "SDL3/"          # windowing / audio / input toolkit
    "spdlog/"        # logging library; core uses std::expected instead
)

set(_violations "")
foreach(_file IN LISTS _core_sources)
    foreach(_pattern IN LISTS _forbidden_patterns)
        # Match  #include "frontend/..."  or  #include <SDL3/...>
        file(STRINGS "${_file}" _matches
            REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"]${_pattern}")
        if(_matches)
            foreach(_match IN LISTS _matches)
                list(APPEND _violations "  ${_file}: ${_match}")
            endforeach()
        endif()
    endforeach()
endforeach()

if(_violations)
    list(JOIN _violations "\n" _violation_text)
    message(FATAL_ERROR
        "Core boundary violation: gbe_core must not include frontend, "
        "SDL3, or logging headers. Move the offending code to the "
        "frontend, or surface a value through std::expected.\n"
        "Violations:\n${_violation_text}")
endif()

message(STATUS "Core boundary check: ${CORE_DIR} is clean")
