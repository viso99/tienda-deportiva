# Estado actual del código

**[Verificado] 2026-10-26** leyendo el repo `viso99/tienda-deportiva`.

El repo tiene **dos cosas separadas**:

1. **La app CAD (en construcción):** `CMakeLists.txt`, `vcpkg.json` y `src/`.
2. **La tienda web de demostración (código heredado):** `index.html`, `styles.css` y `script.js`. No forma parte del producto. Ver más abajo.

## App CAD

Hechos los pasos 2 (**ventana con vista 3D**) y 3 (**boceto con medidas**). Compilada y probada en Linux; todavía no se probó en Windows (ver [compilar.md](compilar.md)).

| Archivo | Qué hace |
|---|---|
| `src/main.cpp` | Arranca Qt y abre la ventana principal. En Linux fuerza la plataforma `xcb`, porque Open CASCADE dibuja sobre una ventana X11 |
| `src/MainWindow.*` | Ventana principal: menú y barra "Boceto" con la acción "Rectángulo…", la vista 3D y una barra de estado. Guarda el `Sketch` actual |
| `src/Sketch.*` | Modelo del boceto: un único rectángulo en el plano XY con una esquina en el origen (mm). Da el contorno (`Wire`), la cara (`Face`, para extruir en el paso 4) y los lados acotados |
| `src/RectangleDialog.*` | Diálogo que pide ancho y alto en mm (0,01 a 100 000) |
| `src/OccView.*` | Widget que incrusta la vista 3D de Open CASCADE (`V3d_View`) y traduce el mouse con `AIS_ViewController`. `ShowSketch` dibuja el contorno y dos cotas (`PrsDim_LengthDimension`) y pone la vista desde arriba |

Qué hace hoy: muestra una grilla en el plano XY, el cubo de orientación (`AIS_ViewCube`) arriba a la derecha y permite girar (botón izquierdo), mover (botón del medio) y hacer zoom (rueda). Con **Boceto → Rectángulo…** se ingresan ancho y alto, y se dibuja el rectángulo con sus cotas. Volver a usarlo reemplaza el rectángulo (hay un solo perfil). Todavía **no** hay extrusión, dibujo con el mouse, archivos ni deshacer.

## Tienda web (código heredado)

> Este código es una **tienda web de demostración**, no el producto CAD de [producto.md](producto.md).

### Contenido

| Archivo | Qué es |
|---|---|
| `index.html` | Página única de la tienda "PROSPORT": encabezado, 5 secciones de productos, pie y panel de carrito |
| `styles.css` | Estilos (tema oscuro, variables CSS en `:root`, un solo corte responsive en 860px) |
| `script.js` | Catálogo de productos, armado de las tarjetas, carrito y eventos |

No tiene servidor, base de datos ni tests. Se abre directo `index.html` en el navegador. Es el commit original del repo (`790439c`, 2026-08-22).

### Cómo funciona la tienda

- **Catálogo:** objeto `PRODUCTS` en `script.js`, con 5 categorías y 6 productos cada una: `camisetas`, `pantalones`, `zapatillas`, `futbol` y `baloncesto`. Las 3 primeras usan fotos de Unsplash. Las equipaciones se dibujan como SVG generado con los colores del equipo (`teamCrestSVG`).
- **Equipos y ligas ficticios a propósito** ("sin marcas ni escudos reales", según un comentario en `script.js`).
- **Ofertas:** un producto con `oldPrice` muestra el precio tachado y la etiqueta "Oferta". `badge` muestra una etiqueta, por ejemplo "Nuevo".
- **Carrito:** solo en memoria (variable `cart`). Se pierde al recargar la página. Permite sumar, restar y quitar productos.
- **Checkout:** es de demostración. Muestra un `alert` aclarando que no se procesa ningún pago y vacía el carrito.
- **Moneda y formato:** euros, `es-ES`.
- **Enlaces del pie** (Envíos, Devoluciones, Contacto): apuntan a `#` y no llevan a ningún lado.

### Relación con el CAD

Ninguna: la app CAD no usa nada de la tienda. Qué hacer con estos archivos (borrarlos, archivarlos, moverlos a otro repo) está en [pendientes.md](pendientes.md).
