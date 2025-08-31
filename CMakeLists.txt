cmake_minimum_required(VERSION 3.16)
project(NoIRFacetracking LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Source files
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/apps")

#File for logging
set(NOIR_LOG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/log" CACHE PATH "Log path.")
set(NOIR_SETTINGS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/settings.yaml" CACHE PATH "Settings path.")
set(NOIR_DATA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/data" CACHE PATH "Data path.")

# ===============================================
# options

option(CON_ENABLE_ASAN "if true, enables clang/MSVC address sanitizer" OFF)
option(CON_ENABLE_MSAN "if true, enables clang/MSVC memory sanitizer" OFF)
option(CON_ENABLE_UBSAN "if true, enables clang/MSVC undefined behaviour sanitizer" OFF)
option(CON_ENABLE_TSAN "if true, enables clang/MSVC thread sanitizer" OFF)

if (CON_ENABLE_ASAN AND CON_ENABLE_TSAN)
    message(FATAL_ERROR "Can only enable one of TSan or ASan at a time")
endif()
if (CON_ENABLE_ASAN AND CON_ENABLE_MSAN)
    message(FATAL_ERROR "Can only enable one of ASan or MSan at a time")
endif()

option(CON_ENABLE_WERROR "if true, enables -Werror, /WX" OFF)

set(GLOW_BIN_DIR ${CMAKE_SOURCE_DIR}/bin)

# ========================================================
# add submodules
# Tell CMake where we would install OpenCV if we need to build it
set(OPENCV_CUSTOM_INSTALL_DIR "${CMAKE_SOURCE_DIR}/include/opencv")
list(APPEND CMAKE_PREFIX_PATH "${OPENCV_CUSTOM_INSTALL_DIR}")

# Try to find OpenCV
find_package(OpenCV QUIET)

if(NOT OpenCV_FOUND)
    message(STATUS "OpenCV not found. Running setup script...")

    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/include/opencv_install.sh")
        message(FATAL_ERROR "OpenCV setup script not found at /include/opencv_install.sh")
    endif()

    # Run the bash script
    execute_process(
        COMMAND bash "${CMAKE_SOURCE_DIR}/include/opencv_install.sh"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE res
    )

    if(NOT res EQUAL 0)
        message(FATAL_ERROR "OpenCV setup script failed with exit code ${res}")
    endif()

    # Retry find_package with the install path
    list(APPEND CMAKE_PREFIX_PATH "${OPENCV_CUSTOM_INSTALL_DIR}")
    find_package(OpenCV REQUIRED)
endif()

find_package(yaml-cpp REQUIRED)

message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")


set(GLOW_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})

# ========================================================
# Source directory
file(GLOB_RECURSE ALL_LIB_SOURCE_FILES "src/*.cpp" "src/*.hpp" "src/*.cc" "src/*.hh" "src/*.c" "src/*.h")
# Remove files from src/old
set(LIB_SOURCE_FILES "")
foreach(FILE ${ALL_LIB_SOURCE_FILES})
    if (NOT FILE MATCHES "/src/old/")
        list(APPEND LIB_SOURCE_FILES ${FILE})
    endif()
endforeach()

MESSAGE("SHOW ALLE FILES WHICH WILL BE COMPILED ${LIB_SOURCE_FILES}")

if(LIB_SOURCE_FILES)
    message("Building Lib with sources: ${LIB_SOURCE_FILES}")
    add_library(Lib ${LIB_SOURCE_FILES})
else()
    message("Building Lib as INTERFACE (no sources in src/)")
    add_library(Lib INTERFACE)
endif()

# Apply common properties regardless of whether Lib has sources
if(TARGET Lib)
    if(LIB_SOURCE_FILES)
        # Normal library: set includes/defs normally
        target_include_directories(Lib PUBLIC src ${OpenCV_INCLUDE_DIRS})
        target_link_libraries(Lib PUBLIC ${OpenCV_LIBS} yaml-cpp)
        target_compile_definitions(Lib PUBLIC
            NOIR_LOG_PATH="${NOIR_LOG_PATH}"
            NOIR_SETTINGS_PATH="${NOIR_SETTINGS_PATH}"
            NOIR_DATA_PATH="${NOIR_DATA_PATH}"
        )
    else()
        # Interface library: properties must be INTERFACE
        target_include_directories(Lib INTERFACE src ${OpenCV_INCLUDE_DIRS})
        target_link_libraries(Lib INTERFACE ${OpenCV_LIBS} yaml-cpp)
        target_compile_definitions(Lib INTERFACE
            NOIR_LOG_PATH="${NOIR_LOG_PATH}"
            NOIR_SETTINGS_PATH="${NOIR_SETTINGS_PATH}"
            NOIR_DATA_PATH="${NOIR_DATA_PATH}"
        )
    endif()
endif()


# ========================================================
# configure executable
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cc" "${SOURCE_DIR}/*.cpp")

# Set the output directory for the executables
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")

# Loop through each .cc file and create an executable
foreach(SOURCE_FILE ${SOURCES})
    # Extract the filename without extension
    get_filename_component(EXECUTABLE_NAME ${SOURCE_FILE} NAME_WE)
    message("Executable target: ${EXECUTABLE_NAME}")

    # Add an executable for this file
    add_executable(${EXECUTABLE_NAME} ${SOURCE_FILE})

    # # Always link opencv
    # target_link_libraries(${EXECUTABLE_NAME} PRIVATE opencv)

    # Link Lib if it exists
    if(TARGET Lib)
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE Lib ${OpenCV_LIBS})
    else()
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE Lib ${OpenCV_LIBS})
    endif()

    # Set the output directory for the executable
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")
endforeach()