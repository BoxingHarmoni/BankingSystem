cmake_minimum_required(VERSION 3.20)
project(BankingSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(BankingSystem ${SOURCES})

# Windows specific flags if needed
if(WIN32)
    target_link_libraries(BankingSystem PRIVATE)
endif()
