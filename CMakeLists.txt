cmake_minimum_required(VERSION 3.16)
project(CadApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCASCADE REQUIRED)

add_executable(CadApp WIN32
  src/main.cpp
  src/MainWindow.cpp
  src/MainWindow.h
  src/OccView.cpp
  src/OccView.h
  src/RectangleDialog.cpp
  src/RectangleDialog.h
  src/Sketch.cpp
  src/Sketch.h
)

target_include_directories(CadApp PRIVATE ${OpenCASCADE_INCLUDE_DIR})

target_link_libraries(CadApp PRIVATE
  Qt6::Widgets
  TKernel TKMath TKService TKV3d TKOpenGl
  TKBRep TKG3d TKGeomBase TKTopAlgo TKPrim
)
