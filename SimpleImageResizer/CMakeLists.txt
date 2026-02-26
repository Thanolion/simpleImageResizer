# CMakeList.txt : CMake project for SimpleImageResizer

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

# Fetch LibRaw source for RAW camera image support
include(FetchContent)
FetchContent_Declare(libraw
    GIT_REPOSITORY https://github.com/LibRaw/LibRaw.git
    GIT_TAG        0.21.3
)
FetchContent_Populate(libraw)

# Build LibRaw as a static library (no CMakeLists.txt in 0.21.x)
file(GLOB_RECURSE LIBRAW_SOURCES "${libraw_SOURCE_DIR}/src/*.cpp")
# Remove integration stubs that require external SDKs
list(FILTER LIBRAW_SOURCES EXCLUDE REGEX "dngsdk_glue\\.cpp$")
list(FILTER LIBRAW_SOURCES EXCLUDE REGEX "rawspeed_glue\\.cpp$")
# Remove placeholder stubs that conflict with real implementations
list(FILTER LIBRAW_SOURCES EXCLUDE REGEX "_ph\\.cpp$")

add_library(raw STATIC ${LIBRAW_SOURCES})
target_include_directories(raw PUBLIC "${libraw_SOURCE_DIR}")
target_compile_definitions(raw PUBLIC LIBRAW_NODLL)
if(WIN32)
    target_compile_definitions(raw PUBLIC LIBRAW_WIN32_CALLS)
endif()
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(raw PRIVATE Threads::Threads)
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(raw PRIVATE m)
endif()
if(MSVC)
    target_compile_options(raw PRIVATE /W0)
else()
    target_compile_options(raw PRIVATE -w)
endif()

# Fetch libavif for AVIF image support (with internal libaom AV1 codec)
set(AVIF_CODEC_AOM "LOCAL" CACHE STRING "" FORCE)
set(AVIF_CODEC_AOM_DECODE ON CACHE BOOL "" FORCE)
set(AVIF_CODEC_AOM_ENCODE ON CACHE BOOL "" FORCE)
set(AVIF_LIBYUV "OFF" CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(AVIF_BUILD_APPS OFF CACHE BOOL "" FORCE)
set(AVIF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(AVIF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# Disable unnecessary libaom components for faster builds
set(ENABLE_DOCS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_TOOLS OFF CACHE BOOL "" FORCE)
# Fall back to generic C if NASM/YASM not available (SIMD optimizations require NASM)
find_program(NASM_EXE nasm)
find_program(YASM_EXE yasm)
if(NOT NASM_EXE AND NOT YASM_EXE)
    set(AOM_TARGET_CPU "generic" CACHE STRING "" FORCE)
    message(STATUS "NASM/YASM not found — building libaom without SIMD optimizations")
endif()
FetchContent_Declare(libavif
    GIT_REPOSITORY https://github.com/AOMediaCodec/libavif.git
    GIT_TAG        v1.1.1
)
FetchContent_MakeAvailable(libavif)

# Suppress warnings on libavif and its dependencies
if(TARGET avif)
    if(MSVC)
        target_compile_options(avif PRIVATE /W0)
    else()
        target_compile_options(avif PRIVATE -w)
    endif()
endif()
if(TARGET aom)
    if(MSVC)
        target_compile_options(aom PRIVATE /W0)
    else()
        target_compile_options(aom PRIVATE -w)
    endif()
endif()

qt_add_executable(SimpleImageResizer
    SimpleImageResizer.cpp
    SimpleImageResizer.h
    MainWindow.h
    MainWindow.cpp
    ProcessingJob.h
    ProcessingResult.h
    SettingsManager.h
    SettingsManager.cpp
    ImageProcessor.h
    ImageProcessor.cpp
    FormatGuideDialog.h
    FormatGuideDialog.cpp
    resources.qrc
)

# Windows: embed icon in executable
if(WIN32)
    target_sources(SimpleImageResizer PRIVATE app_icon.rc)
endif()

# macOS: bundle icon
if(APPLE)
    set(MACOSX_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.icns")
    set_source_files_properties(${MACOSX_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(SimpleImageResizer PRIVATE ${MACOSX_ICON})
endif()

target_link_libraries(SimpleImageResizer PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    raw
    avif
)

set_target_properties(SimpleImageResizer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.simpleimageresizer.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_NAME "Simple Image Resizer"
    MACOSX_BUNDLE_ICON_FILE "app_icon.icns"
)

# Platform-specific deployment
if(WIN32)
    find_program(WINDEPLOYQT windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET SimpleImageResizer POST_BUILD
            COMMAND "${WINDEPLOYQT}" "$<TARGET_FILE:SimpleImageResizer>"
            COMMENT "Running windeployqt..."
        )
    endif()
elseif(APPLE)
    find_program(MACDEPLOYQT macdeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
    if(MACDEPLOYQT)
        add_custom_command(TARGET SimpleImageResizer POST_BUILD
            COMMAND "${MACDEPLOYQT}" "$<TARGET_BUNDLE_DIR:SimpleImageResizer>"
            COMMENT "Running macdeployqt..."
        )
    endif()
elseif(UNIX)
    # Optional: linuxdeployqt for creating self-contained AppImage bundles.
    # Install linuxdeployqt and uncomment below if needed:
    # find_program(LINUXDEPLOYQT linuxdeployqt)
    # if(LINUXDEPLOYQT)
    #     add_custom_command(TARGET SimpleImageResizer POST_BUILD
    #         COMMAND "${LINUXDEPLOYQT}" "$<TARGET_FILE:SimpleImageResizer>"
    #         COMMENT "Running linuxdeployqt..."
    #     )
    # endif()
endif()

# Copy license files to output directory
if(APPLE)
    set(LICENSE_DIR "$<TARGET_BUNDLE_DIR:SimpleImageResizer>/Contents/Resources/licenses")
else()
    set(LICENSE_DIR "$<TARGET_FILE_DIR:SimpleImageResizer>/licenses")
endif()
add_custom_command(TARGET SimpleImageResizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LICENSE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libraw_SOURCE_DIR}/LICENSE.LGPL" "${LICENSE_DIR}/LibRaw-LICENSE.LGPL"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libraw_SOURCE_DIR}/LICENSE.CDDL" "${LICENSE_DIR}/LibRaw-LICENSE.CDDL"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libraw_SOURCE_DIR}/COPYRIGHT" "${LICENSE_DIR}/LibRaw-COPYRIGHT"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libavif_SOURCE_DIR}/LICENSE" "${LICENSE_DIR}/libavif-LICENSE"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/licenses/LGPL-3.0.txt" "${LICENSE_DIR}/LGPL-3.0.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/licenses/LGPL-2.1.txt" "${LICENSE_DIR}/LGPL-2.1.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/licenses/GPL-3.0.txt" "${LICENSE_DIR}/GPL-3.0.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE" "${LICENSE_DIR}/LICENSE"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/THIRD-PARTY-LICENSES.txt" "${LICENSE_DIR}/THIRD-PARTY-LICENSES.txt"
    COMMENT "Copying license files..."
)

# Install targets
include(GNUInstallDirs)
install(TARGETS SimpleImageResizer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
