project (SampleTeemUsingProject)

cmake_minimum_required(VERSION 3.13...3.15)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.15)
endif()

find_package(Teem REQUIRED)
add_executable(sanity sanity.c)
target_include_directories(sanity PRIVATE ${Teem_INCLUDE_DIRS})
target_link_libraries(sanity teem)
