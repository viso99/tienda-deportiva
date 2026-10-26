# Compilar y probar la app

## En Windows (destino principal)

**[Supuesto]** Estos pasos siguen el procedimiento estándar de vcpkg y Qt, pero **todavía no se probaron en una PC con Windows**. Cuando Sergio los pruebe, actualizá esta sección y marcala como [Verificado].

Una sola vez:

1. Instalar **Visual Studio 2022 Community** con la carga de trabajo "Desarrollo para el escritorio con C++" (incluye CMake).
2. Instalar **Git**.
3. Instalar vcpkg, el gestor de librerías de C++:
   ```
   git clone https://github.com/microsoft/vcpkg C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   ```

Cada vez que quieras probar una versión, desde "Developer PowerShell for VS 2022", en la carpeta del repo:

```
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
build\vcpkg_installed\x64-windows\tools\Qt6\bin\windeployqt.exe build\Release\CadApp.exe
build\Release\CadApp.exe
```

La primera compilación tarda bastante (de 30 a 60 minutos), porque vcpkg compila Qt y Open CASCADE a partir de `vcpkg.json`. Las siguientes son rápidas.

## En Linux (lo usa Claude para verificar)

**[Verificado] 2026-10-26** en Ubuntu 24.04 (Qt 6.4, Open CASCADE 7.6):

```
sudo apt-get install qt6-base-dev libocct-visualization-dev libocct-modeling-algorithms-dev libocct-data-exchange-dev libtbb-dev libgl-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/CadApp
```

Sin pantalla (en la nube): `Xvfb :99 +extension GLX &`, después `DISPLAY=:99 ./build/CadApp`. Para la captura, `xwd -root | convert xwd:- captura.png`.
