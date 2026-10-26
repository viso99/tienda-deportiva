# Registro de decisiones

Una entrada por decisión. No se borran: si una cambia, se marca como **reemplazada por D-N** y se agrega la nueva al final.

Formato: **ID · fecha · estado** (vigente / reemplazada), qué se decidió y por qué.

---

**D-1 · 2026-10-26 · vigente**
El proyecto deja de ser una tienda de ropa deportiva y pasa a ser una **aplicación de diseño CAD** (estilo NX, Autodesk, SolidWorks).
*Por qué:* decisión de rumbo de Sergio.

**D-2 · 2026-10-26 · vigente**
La app hace **piezas 3D y planos 2D**.
*Por qué:* Sergio quiere cubrir los dos tipos de diseño. Se le recomendó arrancar solo con 3D y prefirió las dos cosas. D-5 ordena cómo se arranca.

**D-3 · 2026-10-26 · vigente**
Es un **producto para otras personas y empresas**, no un proyecto personal.
*Por qué:* decisión de Sergio. Consecuencia: la precisión, la confiabilidad de los archivos y la instalación importan desde el diseño.

**D-4 · 2026-10-26 · vigente**
Es un **programa de escritorio instalable**, no una app web.
*Por qué:* decisión de Sergio, en línea con SolidWorks y NX. Se le recomendó web (estilo Onshape) y eligió escritorio. No reabrir sin que él lo pida.

**D-5 · 2026-10-26 · vigente**
La **primera versión** se centra en **boceto 2D con medidas → extrusión a pieza 3D**.
*Por qué:* cubre 2D y 3D con una sola función y es el núcleo de cualquier CAD paramétrico.

**D-6 · 2026-10-26 · vigente**
Tecnología: **C++ con Qt** para la interfaz y **Open CASCADE** como motor geométrico. Build con **CMake**; dependencias en Windows con **vcpkg** (`vcpkg.json`).
*Por qué:* Sergio eligió C++ por rendimiento. Se le recomendó Python + Qt + Open CASCADE, que llega antes a una versión funcionando, y prefirió C++. Open CASCADE porque hacer un motor geométrico propio es inviable.
