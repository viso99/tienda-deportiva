cmake_minimum_required(VERSION 3.16)
project(CadApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCASCADE REQUIRED)

# Open CASCADE 7.8+ (vcpkg en Windows) junta STEP en TKDESTEP; la 7.6 (Linux) lo reparte en varias librerías.
if(TARGET TKDESTEP)
  set(OCCT_STEP_LIBS TKDESTEP TKXSBase)
else()
  set(OCCT_STEP_LIBS TKSTEP TKSTEPBase TKSTEPAttr TKSTEP209 TKXSBase)
endif()

# Núcleo sin interfaz: modelo de la pieza, geometría y archivos. Lo usan la app y las pruebas.
add_library(CadCore STATIC
  src/Extrude.cpp
  src/Extrude.h
  src/PartDocument.cpp
  src/PartDocument.h
  src/Sketch.cpp
  src/Sketch.h
)
target_include_directories(CadCore PUBLIC src ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(CadCore PUBLIC
  Qt6::Core
  TKernel TKMath TKBRep TKG3d TKGeomBase TKTopAlgo TKPrim
  ${OCCT_STEP_LIBS}
)

add_executable(CadApp WIN32
  src/main.cpp
  src/MainWindow.cpp
  src/MainWindow.h
  src/OccView.cpp
  src/OccView.h
  src/RectangleDialog.cpp
  src/RectangleDialog.h
)
target_link_libraries(CadApp PRIVATE
  CadCore
  Qt6::Widgets
  TKService TKV3d TKOpenGl
)

enable_testing()
add_executable(PartDocumentTest tests/PartDocumentTest.cpp)
target_link_libraries(PartDocumentTest PRIVATE CadCore)
add_test(NAME PartDocumentTest COMMAND PartDocumentTest)
