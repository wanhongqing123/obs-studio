project(win-mf-camera)

add_library(win-mf-camera MODULE)
add_library(OBS::win-mf-camera ALIAS win-mf-camera)

target_sources(win-mf-camera PRIVATE mf-camera-plugin.cpp mf-camera.cpp mf-camera.hpp)

set(MODULE_DESCRIPTION "OBS Media Foundation module")

configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in win-mf-camera.rc)

target_sources(win-mf-camera PRIVATE win-mf-camera.rc)

target_link_libraries(win-mf-camera PRIVATE OBS::libobs Mfplat Mf mfuuid Mfreadwrite)

set_target_properties(win-mf-camera PROPERTIES FOLDER "plugins")

setup_plugin_target(win-mf-camera)
