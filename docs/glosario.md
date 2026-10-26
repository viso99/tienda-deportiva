# Glosario

Términos de CAD explicados de forma simple. Usá estas palabras con Sergio y en la documentación.

- **CAD:** diseño asistido por computadora. Programas como SolidWorks, NX o AutoCAD.
- **Boceto (sketch):** dibujo 2D sobre un plano (líneas, arcos, rectángulos) que sirve de base para crear una pieza 3D.
- **Cota:** medida que se le pone a un elemento del boceto o del plano, por ejemplo "40 mm".
- **Contorno (perfil):** línea cerrada del boceto que encierra un área; es lo que se extruye.
- **Ajuste (snap):** al dibujar con el mouse, el punto "salta" al valor redondo más cercano (hoy, al milímetro) para que las medidas queden exactas.
- **Restricción:** regla geométrica del boceto, por ejemplo "estas dos líneas son paralelas" o "esta línea es horizontal".
- **Extrusión:** "estirar" un boceto 2D en una dirección para obtener un sólido 3D. Es el núcleo de la v1 (D-5).
- **Operación (feature):** cada paso que modifica la pieza: extrusión, agujero, redondeo, etc.
- **Corte / agujero:** operación que resta material: un contorno dibujado sobre una cara se "hunde" en la pieza una profundidad. **Pasante** o "a través de todo": atraviesa la pieza entera.
- **Arco:** tramo de circunferencia. En el boceto se dibuja por sus dos extremos y un punto por donde pasa ("arco por tres puntos"), y lleva una cota de **radio** ("R5").
- **Círculo:** contorno redondo; se dibuja por el centro y un punto del borde, y lleva una cota de **diámetro** ("Ø10"). Sirve para agujeros redondos.
- **Ranura:** contorno de dos lados rectos y dos medias vueltas, como un óvalo alargado.
- **Saliente:** operación que suma material: un contorno dibujado sobre una cara se extruye hacia afuera.
- **Paramétrico:** el modelo guarda las medidas y los pasos, así que si cambiás una cota la pieza se recalcula sola. Así funcionan SolidWorks y NX.
- **Motor geométrico (kernel):** la librería que hace las cuentas de los sólidos (unir, restar, redondear). Ejemplos: Parasolid (NX, SolidWorks) y Open CASCADE (libre, usado por FreeCAD).
- **Plano (dibujo técnico):** representación 2D con vistas y cotas, para fabricar. Estilo AutoCAD.
- **STEP:** formato estándar para intercambiar piezas 3D entre programas CAD distintos.
- **DXF / DWG:** formatos de dibujos 2D, asociados a AutoCAD.
