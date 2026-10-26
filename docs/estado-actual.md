# Estado actual del código

**[Verificado] 2026-10-26** leyendo el repo `viso99/tienda-deportiva`, rama `main`, commit `790439c`.

> El código actual es una **tienda web de demostración**, no el producto CAD de [producto.md](producto.md). No hay nada de CAD implementado.

## Contenido del repo

| Archivo | Qué es |
|---|---|
| `index.html` | Página única de la tienda "PROSPORT": encabezado, 5 secciones de productos, pie y panel de carrito |
| `styles.css` | Estilos (tema oscuro, variables CSS en `:root`, un solo corte responsive en 860px) |
| `script.js` | Catálogo de productos, armado de las tarjetas, carrito y eventos |

No hay README, licencia, `package.json`, dependencias, servidor, base de datos, tests ni configuración de build. Se abre directo `index.html` en el navegador.

**Historial:** un solo commit (`790439c`, 2026-08-22, autor `viso99`).

## Cómo funciona la tienda

- **Catálogo:** objeto `PRODUCTS` en `script.js`, con 5 categorías y 6 productos cada una: `camisetas`, `pantalones`, `zapatillas`, `futbol` y `baloncesto`. Las 3 primeras usan fotos de Unsplash. Las equipaciones se dibujan como SVG generado con los colores del equipo (`teamCrestSVG`).
- **Equipos y ligas ficticios a propósito** ("sin marcas ni escudos reales", según un comentario en `script.js`).
- **Ofertas:** un producto con `oldPrice` muestra el precio tachado y la etiqueta "Oferta". `badge` muestra una etiqueta, por ejemplo "Nuevo".
- **Carrito:** solo en memoria (variable `cart`). Se pierde al recargar la página. Permite sumar, restar y quitar productos.
- **Checkout:** es de demostración. Muestra un `alert` aclarando que no se procesa ningún pago y vacía el carrito.
- **Moneda y formato:** euros, `es-ES`.
- **Enlaces del pie** (Envíos, Devoluciones, Contacto): apuntan a `#` y no llevan a ningún lado.

## Qué se puede reutilizar para el CAD

Prácticamente nada. El producto objetivo es un programa de escritorio y este código es una página web de tienda. Qué hacer con estos archivos (borrarlos, archivarlos, moverlos a otro repo) está en [pendientes.md](pendientes.md).
