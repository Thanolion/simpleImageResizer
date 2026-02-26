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
)

target_link_libraries(SimpleImageResizer PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    raw
)

set_target_properties(SimpleImageResizer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.simpleimageresizer.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_NAME "Simple Image Resizer"
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
