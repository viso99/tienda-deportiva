cmake_minimum_required(VERSION 3.22)
# La versión tiene que coincidir con la de vcpkg.json.
project(CadApp VERSION 0.1.0 LANGUAGES CXX)

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
  src/Format.cpp
  src/Format.h
  src/History.cpp
  src/History.h
  src/Part.cpp
  src/Part.h
  src/PartDocument.cpp
  src/PartDocument.h
  src/Sketch.cpp
  src/Sketch.h
)
target_include_directories(CadCore PUBLIC src ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(CadCore PUBLIC
  Qt6::Core
  TKernel TKMath TKBRep TKG2d TKG3d TKGeomBase TKGeomAlgo TKTopAlgo TKPrim TKBO TKBool TKShHealing
  ${OCCT_STEP_LIBS}
)

add_executable(CadApp WIN32
  src/main.cpp
  src/FeatureDialog.cpp
  src/FeatureDialog.h
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

# --- Instalación y paquetes (ver docs/compilar.md, "Armar el instalador") ---
# El nombre del producto todavía no está decidido: "Diseño CAD" es provisorio (docs/pendientes.md).
set(CAD_PRODUCT_NAME "Diseño CAD")
set(CAD_PACKAGE_NAME "DisenoCAD") # para el nombre de los archivos, sin acentos

if(WIN32)
  # Todo va en una sola carpeta: CadApp.exe y las DLL al lado.
  # Las DLL de Open CASCADE y sus dependencias se buscan en la carpeta de vcpkg (solo la
  # versión Release: el instalador se arma con --config Release). Las de Windows no se copian.
  set(CAD_VCPKG_BIN "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
  install(TARGETS CadApp
    RUNTIME DESTINATION .
    RUNTIME_DEPENDENCIES
      PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
      POST_EXCLUDE_REGEXES "[/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\]" "[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\]"
      DIRECTORIES "${CAD_VCPKG_BIN}"
  )

  # Qt necesita además sus plugins (por ejemplo platforms/qwindows.dll): eso lo copia windeployqt,
  # el mismo paso que se hace a mano en la carpeta build\Release.
  find_program(CAD_WINDEPLOYQT windeployqt
    HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/Qt6/bin"
    REQUIRED)
  install(CODE "
    execute_process(
      COMMAND \"${CAD_WINDEPLOYQT}\" --no-compiler-runtime \"\${CMAKE_INSTALL_PREFIX}/CadApp.exe\"
      COMMAND_ERROR_IS_FATAL ANY)
  ")

  # Runtime de Visual C++ (msvcp140.dll, vcruntime140.dll...), para PCs que no lo tienen.
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
  include(InstallRequiredSystemLibraries)

  # Instalador .exe (NSIS) y .zip portátil.
  set(CPACK_GENERATOR NSIS ZIP)
  set(CPACK_PACKAGE_FILE_NAME "${CAD_PACKAGE_NAME}-${PROJECT_VERSION}-win64")
  set(CPACK_NSIS_PACKAGE_NAME "${CAD_PRODUCT_NAME}")
  set(CPACK_NSIS_DISPLAY_NAME "${CAD_PRODUCT_NAME} ${PROJECT_VERSION}")
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "CadApp.exe")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
  set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
  # Sin página de licencia hasta que se decida una (si no, CPack muestra la de CMake).
  set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)
else()
  install(TARGETS CadApp RUNTIME DESTINATION bin)
  set(CPACK_GENERATOR TGZ)
  set(CPACK_PACKAGE_FILE_NAME "${CAD_PACKAGE_NAME}-${PROJECT_VERSION}-linux")
endif()

set(CPACK_PACKAGE_NAME "${CAD_PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "${CAD_PRODUCT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CAD_PRODUCT_NAME}: piezas 3D a partir de bocetos 2D")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CAD_PRODUCT_NAME}")
# Acceso directo en el menú Inicio: ejecutable y texto que se ve.
set(CPACK_PACKAGE_EXECUTABLES "CadApp" "${CAD_PRODUCT_NAME}")
include(CPack)
