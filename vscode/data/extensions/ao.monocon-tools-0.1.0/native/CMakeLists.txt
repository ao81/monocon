cmake_minimum_required(VERSION 3.21)
project(monocon_native CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT NODE_INCLUDE_DIR OR NOT NODE_LIBRARY)
    message(FATAL_ERROR "NODE_INCLUDE_DIR and NODE_LIBRARY are required")
endif()

set(DAEMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../daemon")

add_library(monocon_native SHARED
    addon.cpp
    win_delay_load_hook.cc
    "${DAEMON_DIR}/src/builder.cpp"
    "${DAEMON_DIR}/src/daemon_state.cpp"
    "${DAEMON_DIR}/src/port_scanner.cpp"
    "${DAEMON_DIR}/src/stk500v2.cpp"
    "${DAEMON_DIR}/src/utils.cpp"
)

target_include_directories(monocon_native PRIVATE
    "${NODE_INCLUDE_DIR}"
    "${DAEMON_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
)
target_compile_definitions(monocon_native PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    NAPI_VERSION=8
    HOST_BINARY="node.exe"
)
target_link_libraries(monocon_native PRIVATE
    "${NODE_LIBRARY}"
    advapi32
    setupapi
    shell32
    delayimp
)

if(MSVC)
    # VSIX単体でクリーンなWindows環境でも読み込めるよう、Visual C++
    # Runtimeをネイティブアドオンへ静的リンクする。
    set_property(TARGET monocon_native PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    target_compile_options(monocon_native PRIVATE
        /O2 /Oi /GL /EHsc /utf-8 /W4 /guard:cf
    )
    target_link_options(monocon_native PRIVATE
        /LTCG
        /OPT:REF
        /OPT:ICF
        /INCREMENTAL:NO
        /guard:cf
        /DELAYLOAD:node.exe
    )
endif()

if(NOT MONOCON_NATIVE_OUTPUT_DIR)
    set(MONOCON_NATIVE_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/win32-x64")
endif()

set_target_properties(monocon_native PROPERTIES
    OUTPUT_NAME "monocon_native_v170"
    PREFIX ""
    SUFFIX ".node"
    RUNTIME_OUTPUT_DIRECTORY "${MONOCON_NATIVE_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MONOCON_NATIVE_OUTPUT_DIR}"
)
