cmake_minimum_required(VERSION 3.5)

project(catchthecat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-lGLEW -lglfw -lGL -lpthread -lSOIL -lopenal")
set(SOURCES main.cpp shader.cpp map.cpp sound.cpp)
set(SHADERS vs.glsl fs.glsl)

add_executable(catchthecat ${SOURCES})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${SHADERS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
