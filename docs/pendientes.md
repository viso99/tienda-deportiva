# Pendientes y supuestos

Lo que **todavía no está decidido**. Nada de esta página es un hecho. Cuando algo se resuelve, se saca de acá y se registra en [decisiones.md](decisiones.md).

## Supuestos en uso

| # | Supuesto | Por qué se asumió | Cómo validarlo |
|---|---|---|---|
| S-1 | El sistema operativo inicial es **Windows** | Es donde trabajan casi todas las empresas que usan SolidWorks (que solo existe para Windows) | Preguntarle a Sergio antes de elegir tecnología |
| S-2 | El CAD se desarrolla **en este mismo repo** (`tienda-deportiva`) | Es el único repo del proyecto | Preguntarle a Sergio si prefiere un repo nuevo con otro nombre |

## Preguntas abiertas (para decidir con Sergio)

1. **Qué hacer con el código de la tienda:** ¿se borra, se archiva o se mueve a otro repo? No tocarlo hasta que Sergio decida.
2. **Tecnología del programa:** no está elegida. Hay que decidirla antes de escribir código. *Recomendación a proponerle, todavía no decidida:* C++ con Qt para la interfaz y **Open CASCADE** como motor geométrico. Es la base de FreeCAD, que es software libre y probado. Hacer el motor geométrico desde cero es inviable.
3. **Formatos de archivo:** formato propio y compatibilidad (por ejemplo STEP para 3D y DXF/DWG para 2D). Importa porque los clientes son empresas (D-3).
4. **Alcance exacto de la v1** más allá de boceto → extrusión: ¿guardar y abrir?, ¿deshacer?, ¿cotas editables?, ¿exportar?
5. **Modelo de negocio y licencia:** venta, suscripción, gratis o código abierto. Influye en qué librerías se pueden usar (por sus licencias).
6. **Nombre del producto:** el repo se llama `tienda-deportiva` y el código dice "PROSPORT", y ninguno de los dos aplica.
