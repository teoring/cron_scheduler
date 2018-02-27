cmake_minimum_required(VERSION 2.8)

#========================================
# SECTION:  requirements
#========================================

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    if(!CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.5 AND !CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.7)
         error( "Project requires 4.7 or 5.4 GNU compiler version" )
    endif()
endif()

#========================================
# SECTION: project setup
#========================================

project(securitymatterstask)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -gdwarf-4 -O0  --coverage -fprofile-arcs -ftest-coverage -Wall")
set(DEFAULT_BUILD_TYPE "Release")
set(EXECUTABLE_OUTPUT_PATH   ${PROJECT_SOURCE_DIR}/bin)

#========================================
# SECTION: sources
#========================================

include_directories("src")

set(SOURCE_TESTER_FILES
    src/CronScheduler.cpp
    src/CronScheduler.h
    tests/CronSchedulerTests.cpp
    tests/CronSchedulerTestFixture.h
)

#========================================
# SECTION: dependecies and definitions
#========================================

find_package(Threads)
find_package(Boost 1.58.0 COMPONENTS "unit_test_framework" REQUIRED)

add_definitions(-DBOOST_TEST_DYN_LINK) 
add_executable (run_tests ${SOURCE_TESTER_FILES})

target_link_libraries (run_tests
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    pthread
    gcov
)

#========================================
# SECTION: test execution 
#========================================

add_custom_command(TARGET run_tests
  POST_BUILD
  COMMAND ./run_tests
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
  )