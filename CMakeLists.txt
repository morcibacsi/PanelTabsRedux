cmake_minimum_required(VERSION 3.21)
set(PANELTABS_VERSION "1.0.0" CACHE STRING "Panel Tabs Redux release version")
if(NOT PANELTABS_VERSION MATCHES "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)$")
    message(FATAL_ERROR "PANELTABS_VERSION must have the form MAJOR.MINOR.PATCH")
endif()
project(PanelTabsCpp VERSION "${PANELTABS_VERSION}" LANGUAGES CXX)

add_library(PanelTabsCpp MODULE PanelTabs.cpp PanelTabs.def)
target_include_directories(PanelTabsCpp PRIVATE ../FarManager/plugins/common/unicode)
target_compile_features(PanelTabsCpp PRIVATE cxx_std_20)
target_link_libraries(PanelTabsCpp PRIVATE ole32 crypt32)
target_compile_definitions(PanelTabsCpp PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_definitions(PanelTabsCpp PRIVATE
    PANELTABS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PANELTABS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PANELTABS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
target_compile_options(PanelTabsCpp PRIVATE /W4 /permissive- /utf-8)
set_target_properties(PanelTabsCpp PROPERTIES
    OUTPUT_NAME PanelTabsRedux
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS PanelTabsCpp LIBRARY DESTINATION .)
install(FILES PanelTabs.lua RenewalCustom.xml README.md DESTINATION .)

set(package_arch "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
if(package_arch STREQUAL "X86")
    set(package_arch x86)
endif()
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME PanelTabsRedux)
set(CPACK_PACKAGE_FILE_NAME "PanelTabsRedux-${PROJECT_VERSION}-${package_arch}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
