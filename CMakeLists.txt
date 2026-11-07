cmake_minimum_required(VERSION 3.21)
set(PANELTABS_VERSION "1.0.0" CACHE STRING "Panel Tabs Redux release version")
if(NOT PANELTABS_VERSION MATCHES "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)$")
    message(FATAL_ERROR "PANELTABS_VERSION must have the form MAJOR.MINOR.PATCH")
endif()
project(PanelTabsCpp VERSION "${PANELTABS_VERSION}" LANGUAGES CXX)

set(LANGUAGE_FILES PanelTabsReduxEng.lng PanelTabsReduxRus.lng)
set(HELP_FILES PanelTabsReduxEng.hlf PanelTabsReduxRus.hlf)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${LANGUAGE_FILES})
set(message_id_pattern "^//\\[([A-Za-z][A-Za-z0-9]*)\\]$")
set(message_text_pattern "^\".*\"$")
file(STRINGS PanelTabsReduxEng.lng message_ids REGEX "${message_id_pattern}" ENCODING UTF-8)
file(STRINGS PanelTabsReduxEng.lng english_messages REGEX "${message_text_pattern}" ENCODING UTF-8)
file(STRINGS PanelTabsReduxRus.lng russian_ids REGEX "${message_id_pattern}" ENCODING UTF-8)
file(STRINGS PanelTabsReduxRus.lng russian_messages REGEX "${message_text_pattern}" ENCODING UTF-8)
list(LENGTH message_ids message_count)
list(LENGTH english_messages english_count)
list(LENGTH russian_messages russian_count)
if(NOT message_ids STREQUAL russian_ids OR NOT message_count EQUAL english_count
   OR NOT message_count EQUAL russian_count)
    message(FATAL_ERROR "Language files must contain the same message IDs and one string per ID")
endif()

# Embed English strings so the DLL also works without language files.
list(TRANSFORM message_ids REPLACE "${message_id_pattern}" "\\1")
list(TRANSFORM english_messages PREPEND "L")
list(JOIN message_ids ",\n    " message_ids)
list(JOIN english_messages ",\n    " english_messages)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/PanelTabsMessages.hpp" CONTENT
"#pragma once

enum class Message
{
    ${message_ids}
};

inline constexpr const wchar_t* EnglishMessages[]{
    ${english_messages}
};
")

set(plugin_output_dir "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
foreach(language_file IN LISTS LANGUAGE_FILES HELP_FILES)
    file(GENERATE OUTPUT "${plugin_output_dir}/${language_file}" INPUT "${language_file}")
endforeach()

add_library(PanelTabsCpp MODULE PanelTabs.cpp PanelTabs.def)
target_include_directories(PanelTabsCpp PRIVATE ../FarManager/plugins/common/unicode "${CMAKE_CURRENT_BINARY_DIR}")
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
    LIBRARY_OUTPUT_DIRECTORY "${plugin_output_dir}"
)

install(TARGETS PanelTabsCpp LIBRARY DESTINATION .)
install(FILES ${LANGUAGE_FILES} ${HELP_FILES} PanelTabs.lua RenewalCustom.xml README.md DESTINATION .)

set(package_arch "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
if(package_arch STREQUAL "X86")
    set(package_arch x86)
endif()
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME PanelTabsRedux)
set(CPACK_PACKAGE_FILE_NAME "PanelTabsRedux-${PROJECT_VERSION}-${package_arch}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
