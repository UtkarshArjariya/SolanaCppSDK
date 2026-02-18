cmake_minimum_required(VERSION 3.10)
project(SolanaCppSDK VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add include directories
include_directories(include)

# Add library directories
link_directories(lib)

# ── OpenSSL / TLS support ────────────────────────────────────────────────────
# Try to find OpenSSL. If not found, build without TLS.
find_package(OpenSSL QUIET)
if(OPENSSL_FOUND)
  message(STATUS "OpenSSL found: ${OPENSSL_VERSION} — TLS support enabled")
else()
  message(STATUS "OpenSSL not found — building without TLS (SOLANA_SDK_NO_TLS)")
  add_definitions(-DSOLANA_SDK_NO_TLS)
endif()

# ── Library ──────────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES "src/solana/*.cpp")
add_library(solanacppsdk STATIC ${SOURCES})
target_link_libraries(solanacppsdk sodium)

if(OPENSSL_FOUND)
  target_include_directories(solanacppsdk PRIVATE ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(solanacppsdk ${OPENSSL_LIBRARIES})
endif()

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()
file(GLOB TEST_SOURCES "tests/solana/*.cpp")
foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} solanacppsdk sodium)
    if(OPENSSL_FOUND)
      target_link_libraries(${test_name} ${OPENSSL_LIBRARIES})
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# ── Examples ─────────────────────────────────────────────────────────────────
file(GLOB EXAMPLE_SOURCES "examples/*.cpp")
foreach(example_file ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_file} NAME_WE)
    add_executable(${example_name} ${example_file})
    target_link_libraries(${example_name} solanacppsdk sodium)
    if(OPENSSL_FOUND)
      target_link_libraries(${example_name} ${OPENSSL_LIBRARIES})
    endif()
endforeach()
