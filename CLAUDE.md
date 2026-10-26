# CLAUDE.md: punto de entrada del proyecto

Leé este archivo entero al empezar cada sesión. Es corto a propósito: dice qué es el proyecto, qué reglas respetar y dónde está cada tema.

## Qué es este proyecto (en 30 segundos)

- **Producto objetivo (confirmado por Sergio, 2026-10-26):** un programa de diseño CAD **de escritorio**, del estilo de SolidWorks, NX o AutoCAD, que hace **piezas 3D y planos 2D**, pensado para **otras personas y empresas**. La primera versión permite **dibujar un boceto 2D con medidas y extruirlo a una pieza 3D**. Detalle en [docs/producto.md](docs/producto.md).
- **Código actual (verificado):** el repo todavía contiene **otra cosa**: una tienda web de demostración de ropa deportiva ("PROSPORT"), con 3 archivos estáticos. No tiene nada de CAD. Detalle en [docs/estado-actual.md](docs/estado-actual.md).

> ⚠️ El código de hoy **no** describe el producto que se quiere construir. No deduzcas requisitos del CAD a partir de la tienda, ni al revés.

## Mapa de la documentación

| Tema | Archivo | Cuándo leerlo |
|---|---|---|
| Qué se construye, para quién, alcance de la v1 | [docs/producto.md](docs/producto.md) | Antes de proponer o implementar cualquier funcionalidad |
| Qué hay realmente en el repo hoy | [docs/estado-actual.md](docs/estado-actual.md) | Antes de tocar código existente |
| Decisiones tomadas y por qué | [docs/decisiones.md](docs/decisiones.md) | Antes de cambiar rumbo, tecnología o alcance |
| Preguntas abiertas y suposiciones | [docs/pendientes.md](docs/pendientes.md) | Antes de decidir algo que todavía no está cerrado |
| Vocabulario CAD en lenguaje simple | [docs/glosario.md](docs/glosario.md) | Al hablar con Sergio o escribir documentación |

## Cómo se clasifica la información

Toda afirmación en `docs/` lleva una de estas etiquetas:

- **[Verificado]**: comprobado leyendo el código o el historial de git. Incluye la fecha de verificación.
- **[Confirmado]**: decisión o requisito que Sergio confirmó. Incluye la fecha.
- **[Supuesto]**: inferencia razonable que **todavía no se validó**. Vive en `docs/pendientes.md` hasta que se confirme o se descarte.

Si algo no tiene etiqueta, tratalo como supuesto.

## Reglas para evitar alucinaciones

1. **Consultá antes de decidir.** Antes de una decisión importante, leé el doc del tema. Si toca código, contrastalo con el código.
2. **No inventes.** No inventes funcionalidades, archivos, módulos, formatos, APIs, tablas ni reglas de negocio. Si no está en el código ni en `docs/`, no existe todavía.
3. **Actual ≠ objetivo.** No confundas lo que el código hace hoy con lo que el producto debería hacer. Ver el aviso de arriba.
4. **Contradicciones:** si `docs/` y el código no coinciden, o dos docs se contradicen, investigá primero. Si no se resuelve leyendo, preguntale a Sergio. No elijas una versión en silencio.
5. **Falta información:** preguntale a Sergio en lugar de asumir. Si tenés que avanzar igual, registrá el supuesto en `docs/pendientes.md` y avisale.
6. **Supuestos visibles:** nunca conviertas un [Supuesto] en [Confirmado] sin una respuesta explícita de Sergio.

## Cómo mantener esta memoria

- Cuando cambie una decisión, una funcionalidad o una regla importante, actualizá el doc del tema **en el mismo cambio** (mismo commit o PR).
- Una decisión nueva va a `docs/decisiones.md` con la fecha, el estado y el porqué. Si reemplaza a otra, marcá la vieja como "reemplazada por D-N"; no la borres.
- Cuando se resuelva un pendiente, sacalo de `docs/pendientes.md` y llevalo a `decisiones.md` o al doc del tema.
- Cuando cambie el código, actualizá `docs/estado-actual.md` con la nueva fecha de verificación.
- Cada dato vive en **un solo lugar**: los demás docs lo enlazan, no lo copian. Este archivo solo resume y apunta.

## Cómo trabajar con Sergio

- Español rioplatense, simple y breve. **Profundidad en el análisis, simplicidad en la comunicación.**
- Investigá todo lo posible antes de preguntar. No le preguntes lo que se puede averiguar leyendo el repo.
- Preguntas de a una, sin tecnicismos innecesarios. Explicá el contexto en una o dos frases y **recomendá siempre una opción** con el porqué.
- Cuando necesites que intervenga, decile qué detectaste, qué tiene que decidir y qué le recomendás.
- Si algo no necesita su intervención, resolvelo siguiendo las decisiones de `docs/decisiones.md`.
- Pedí su visto bueno antes de editar código o configuración, hacer push, abrir PRs o mergear.
