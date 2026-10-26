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

**D-7 · 2026-10-26 · vigente**
Después de la v1, lo primero que se suma es **guardar, abrir y exportar a STEP**.
*Por qué:* Sergio lo eligió entre tres opciones (también estaban dibujar con el mouse y más operaciones). Sin guardar, el trabajo se pierde al cerrar la app, y una empresa necesita pasar la pieza a otros programas CAD.

**D-8 · 2026-10-26 · vigente (decisión técnica, se puede revisar)**
Formato propio **`.cadp`**: JSON con los **parámetros** de la pieza (boceto y extrusión), no la geometría. Al abrir, la geometría se reconstruye. Lleva `format` y `version` para poder evolucionar. Para intercambio, **STEP AP214 en milímetros**.
*Por qué:* guardar parámetros mantiene la pieza editable (paramétrica), y JSON es legible y fácil de migrar. STEP es el estándar que abren SolidWorks, NX y casi cualquier CAD. La extensión `.cadp` es provisoria hasta que haya nombre de producto.

**D-9 · 2026-10-26 · vigente**
Siguiente paso después de D-7: **dibujar el boceto con el mouse**. Se hace en dos partes: primero líneas y rectángulos dibujados en la vista (contorno cerrado de cualquier forma, extruible); después, cotas editables.
*Por qué:* Sergio lo eligió entre tres opciones (también estaban más operaciones y pulir lo que ya hay). Es lo que hace que se sienta un CAD de verdad, y las operaciones rinden más cuando el boceto puede tener cualquier forma.
Detalles técnicos (se pueden revisar): el perfil se guarda como lista de vértices (formato `.cadp` versión 2, que sigue abriendo la versión 1 de D-8); los puntos se ajustan al milímetro; un contorno que se cruza consigo mismo se rechaza. Al cambiar una cota, queda fijo el vértice inicial del lado y se arrastran los vértices siguientes unidos por lados perpendiculares; sin restricciones, es la regla que mantiene una L como L. Cuando haya restricciones, esta regla se reemplaza.

**D-10 · 2026-10-26 · vigente**
Siguiente paso después de D-9: **agujeros, cortes y salientes**. Se hace clic en una cara plana, se dibuja el contorno sobre ella y se elige la profundidad (o "a través de todo") o la altura; el material se resta o se suma.
*Por qué:* Sergio lo eligió entre cuatro opciones (también estaban círculos y arcos en el boceto, pulir lo que ya hay y planos 2D). Con varias operaciones la app pasa a hacer piezas reales.
Detalles técnicos (se pueden revisar): la pieza guarda la lista de operaciones en orden, y cada una guarda el **plano** de la cara (origen, normal y eje X), no la cara, para no depender de cómo Open CASCADE numera las caras al recalcular. Por eso, si cambia el boceto base, el plano no lo sigue. El formato `.cadp` pasa a la versión 3 y sigue abriendo la 1 y la 2. Una operación que no cambia la pieza se rechaza al crearla y se saltea al recalcular.

**D-11 · 2026-10-26 · vigente**
Siguiente paso después de D-10: **círculos y arcos en el boceto**. Se suman las herramientas Círculo (centro y un punto del borde) y Arco (extremo final y un punto de paso), que se mezcla con Línea en el mismo contorno. Los arcos llevan cota de radio y los círculos de diámetro, editables con doble clic. Valen para el boceto base y para los cortes y salientes (agujeros redondos).
*Por qué:* Sergio lo eligió entre cuatro opciones (también estaban redondeos y chaflanes, plano 2D con cotas y deshacer). Casi toda pieza real tiene agujeros redondos.
Detalles técnicos (se pueden revisar): el contorno sigue siendo una lista de vértices, y cada lado guarda un **bulge** (tan(ángulo / 4), el mismo dato de los archivos DXF): 0 es recto. Un círculo son dos medias vueltas. Para validar que el contorno no se cruce, los arcos se aproximan con tramos de 5° como máximo. Al cambiar el radio de un arco, sus extremos quedan fijos. El formato `.cadp` pasa a la versión 4 (`"bulges"` en el contorno y `"type": "circle"` con centro y radio) y sigue abriendo la 1, la 2 y la 3. El ajuste del mouse sigue siendo de 1 mm (en el círculo, el radio también se ajusta al milímetro).
