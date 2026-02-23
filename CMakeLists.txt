cmake_minimum_required(VERSION 3.8)
project(adore_map VERSION 0.0.1 LANGUAGES CXX)

# C++20 strict mode
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_package(ament_cmake REQUIRED)
find_package(OpenDrive REQUIRED)
find_package(adore_math REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PROJ REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(CURL REQUIRED) 
find_package(caches REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-O3)
endif()

# Library sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")

# Define library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Include paths
target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${OpenCV_INCLUDE_DIRS}
    ${OpenDrive_INCLUDE_DIRS}
)

# Link dependencies (PUBLIC so downstream sees them)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    OpenDrive::OpenDrive
    adore_math::adore_math
    PROJ::proj
    Eigen3::Eigen
    CURL::libcurl
    caches::caches
    # Add ${OpenCV_LIBS} here if you have compiled OpenCV dependencies.
)

# -------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  # Library under test, used in test/CMakeLists.txt
  set(ADORE_TEST_LIB_TARGET ${PROJECT_NAME})

  add_subdirectory(test)
endif()

# -------------------------------------------------------------------
# Install & export
# -------------------------------------------------------------------

# Export target to make find_package(adore_map) work in other packages
install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# Export headers
install(DIRECTORY include/
  DESTINATION include
)

# Export the CMake config for downstream consumers
install(EXPORT export_${PROJECT_NAME}
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION share/${PROJECT_NAME}/cmake
)

# ament exports for ROS 2 packages
ament_export_include_directories(include)
ament_export_dependencies(OpenDrive PROJ adore_math OpenCV Eigen3 caches)
ament_export_targets(export_${PROJECT_NAME})

ament_package()
