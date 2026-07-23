cmake_minimum_required(VERSION 3.15)
project(ArduinoUploader CXX)

# ----- C++ 標準 -----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----- ビルドタイプのデフォルト -----
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ----- 最適化フラグ -----
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /Ot /GL")
    # /GL を使うので LTCG をリンカに渡す
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# ----- ターゲット -----
add_executable(ArduinoUploader
    main.cpp
    utils.cpp
    uploader.cpp
)

# ----- Windows 固有 -----
if(WIN32)
    if(MSVC)
        # コンソールアプリ
        set_target_properties(ArduinoUploader PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:CONSOLE")

        target_link_libraries(ArduinoUploader PRIVATE
            wbemuuid
            ole32
            oleaut32
            comsuppw
        )
    else()
        # MinGW
        target_link_libraries(ArduinoUploader PRIVATE
            ole32
            oleaut32
            uuid
            wbemuuid
        )
    endif()
endif()

# ----- 出力先 -----
set_target_properties(ArduinoUploader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)