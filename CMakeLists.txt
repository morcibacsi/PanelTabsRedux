cmake_minimum_required(VERSION 3.21)
project(PanelTabsCpp LANGUAGES CXX)

add_library(PanelTabsCpp MODULE PanelTabs.cpp PanelTabs.def)
target_include_directories(PanelTabsCpp PRIVATE ../FarManager/plugins/common/unicode)
target_compile_features(PanelTabsCpp PRIVATE cxx_std_20)
target_compile_definitions(PanelTabsCpp PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(PanelTabsCpp PRIVATE /W4 /permissive- /utf-8)
set_target_properties(PanelTabsCpp PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
