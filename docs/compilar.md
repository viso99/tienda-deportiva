# Compilar y probar la app

## En Windows (destino principal)

**[Verificado] 2026-10-26** en la PC de Sergio (Windows 11): compila y arranca, y el flujo completo funciona: boceto de 40 × 20, extrusión de 10, giro con el mouse y recálculo al cambiar a 100 × 30. Versiones: VS 2022 Build Tools 17.14 (MSVC 19.44), SDK de Windows 10.0.26100, Qt 6.11.2, Open CASCADE 8.0.1 y vcpkg 2026-09-26.

Una sola vez:

1. Instalar las **Build Tools de Visual Studio 2022** con C++ (incluyen el compilador, el SDK de Windows y CMake; no hace falta el Visual Studio completo). Windows pide aceptar un aviso de administrador:
   ```
   winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
   ```
2. Instalar **Git**.
3. Instalar vcpkg, el gestor de librerías de C++:
   ```
   git clone https://github.com/microsoft/vcpkg C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   ```

Para compilar y abrir la app, desde "Developer PowerShell for VS 2022", en la carpeta del repo:

```
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
build\vcpkg_installed\x64-windows\tools\Qt6\bin\windeployqt.exe build\Release\CadApp.exe
build\Release\CadApp.exe
```

Notas:

- La primera configuración tardó **1,3 horas**, porque vcpkg compila Qt y Open CASCADE a partir de `vcpkg.json`, y ocupa bastante disco en `C:\vcpkg` (prever de 20 a 30 GB libres). Las siguientes compilaciones son rápidas.
- `windeployqt` es obligatorio: sin él no se copia `platforms\qwindows.dll` y la app no abre. El aviso de que no encuentra traducciones es inofensivo.
- El mensaje "vswhere.exe no se reconoce" al abrir el entorno de desarrollo también es inofensivo.
- Las pruebas se corren con `ctest -C Release --output-on-failure` dentro de `build`. **[Verificado] 2026-10-26:** pasan en Windows con Open CASCADE 8.0.1, y el enlace con `TKDESTEP` (STEP en 7.8+, que `CMakeLists.txt` elige solo) funciona. Los menús de Archivo (Guardar como, Nuevo, Abrir, Exportar a STEP y el aviso de cambios sin guardar) también se probaron en Windows y funcionan. El dibujo con el mouse y las cotas editables (D-9) también se probaron en Windows y funcionan. Los agujeros, cortes y salientes (D-10) también se probaron en Windows y funcionan (bolsillo, saliente, agujero pasante, quitar la última operación, Esc, guardar y abrir la versión 3 del `.cadp` y exportar a STEP); suman las librerías `TKBO`, `TKBool`, `TKShHealing`, `TKG2d` y `TKGeomAlgo` de Open CASCADE, que enlazan sin problemas. Al compilar sale un único aviso: `Graphic3d_Vec2.hxx` está marcado como obsoleto desde Open CASCADE 8.0 (en `MainWindow.cpp`, `OccView.cpp` y el moc). No rompe nada.
- **vcpkg instala Open CASCADE 8.0**, y en Linux se usa la 7.6. En la 8.0 algunos tipos ya no llegan incluidos de rebote por otros headers (por ejemplo `Graphic3d_Vec2i`, que necesita `#include <Graphic3d_Vec2.hxx>`). Incluí siempre de forma explícita los headers de lo que usás, y si un cambio compila en Linux pero toca la API de Open CASCADE, tené en cuenta que en Windows puede fallar.

## En Linux (lo usa Claude para verificar)

**[Verificado] 2026-10-26** en Ubuntu 24.04 (Qt 6.4, Open CASCADE 7.6):

```
sudo apt-get install qt6-base-dev libocct-visualization-dev libocct-modeling-algorithms-dev libocct-data-exchange-dev libtbb-dev libgl-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
(cd build && ctest --output-on-failure)
./build/CadApp
```

Sin pantalla (en la nube): `Xvfb :99 +extension GLX &`, después `DISPLAY=:99 ./build/CadApp`. Para la captura, `xwd -root | convert xwd:- captura.png`.
