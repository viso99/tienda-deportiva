# Pendientes y supuestos

Lo que **todavía no está decidido**. Nada de esta página es un hecho. Cuando algo se resuelve, se saca de acá y se registra en [decisiones.md](decisiones.md).

## Supuestos en uso

| # | Supuesto | Por qué se asumió | Cómo validarlo |
|---|---|---|---|
| S-1 | El sistema operativo inicial es **Windows** | Es donde trabajan casi todas las empresas que usan SolidWorks (que solo existe para Windows) | Preguntarle a Sergio. La compilación en Windows ya está verificada ([compilar.md](compilar.md)) |
| S-2 | El CAD se desarrolla **en este mismo repo** (`tienda-deportiva`) | Es el único repo del proyecto | Preguntarle a Sergio si prefiere un repo nuevo con otro nombre |

## Preguntas abiertas (para decidir con Sergio)

1. **Qué hacer con el código de la tienda:** ¿se borra, se archiva o se mueve a otro repo? No tocarlo hasta que Sergio decida.
2. **Formatos de archivo:** el formato propio y STEP ya están (D-8). Faltan importar STEP y los formatos 2D (DXF/DWG) para cuando haya planos.
3. **Alcance exacto de la v1** más allá de boceto → extrusión: ¿guardar y abrir?, ¿deshacer?, ¿cotas editables?, ¿exportar?
4. **Modelo de negocio y licencia:** venta, suscripción, gratis o código abierto. Influye en qué librerías se pueden usar (por sus licencias).
5. **Nombre del producto:** el repo se llama `tienda-deportiva` y la tienda dice "PROSPORT", y ninguno de los dos aplica. La extensión `.cadp` también es provisoria.
