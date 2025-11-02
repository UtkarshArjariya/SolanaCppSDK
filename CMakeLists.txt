cmake_minimum_required(VERSION 3.10)
project(SolanaCppSDK)

set(CMAKE_CXX_STANDARD 17)

# Add include directories
include_directories(include)

# Add library directories
link_directories(lib)

# Create the library
file(GLOB SOURCES "src/solana/*.cpp")
add_library(solanacppsdk STATIC ${SOURCES})

# Link against libsodium
target_link_libraries(solanacppsdk sodium)

# Create test executables
file(GLOB TEST_SOURCES "tests/solana/*.cpp")

foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} solanacppsdk sodium)
endforeach()
