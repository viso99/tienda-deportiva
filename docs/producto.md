# Producto: aplicación de diseño CAD

Qué se quiere construir. Todo lo de esta página fue **[Confirmado]** por Sergio el 2026-10-26, salvo lo que diga otra cosa. Las decisiones y su porqué están en [decisiones.md](decisiones.md).

## Visión

Un programa de diseño asistido por computadora (CAD) en la línea de **SolidWorks, Siemens NX y AutoCAD**, que permite:

- **Modelar piezas en 3D** a partir de medidas (estilo SolidWorks / NX).
- **Dibujar planos en 2D** (estilo AutoCAD).

## Para quién

**Otras personas y empresas.** No es un proyecto personal ni de portfolio. Implica que con el tiempo hacen falta precisión real en las medidas, guardar y abrir archivos de forma confiable, y que el programa se pueda instalar y actualizar. Lo que todavía no está definido (modelo de negocio, formatos de intercambio, etc.) está en [pendientes.md](pendientes.md).

## Plataforma

**Programa de escritorio que se instala**, como SolidWorks o NX. No es una app web.

- Sistema operativo: **[Supuesto]** Windows primero. Ver [pendientes.md](pendientes.md).

## Primera versión (v1)

Objetivo: **dibujar un boceto 2D con medidas y convertirlo en una pieza 3D por extrusión.**

Ejemplo del flujo esperado: el usuario dibuja un rectángulo de 40 × 20 mm, lo extruye 10 mm y obtiene un bloque 3D que puede ver y girar.

Qué entra en la v1 y qué no: solo está confirmado el flujo boceto → extrusión. Cualquier otra funcionalidad (agujeros, redondeos, ensamblajes, planos con cotas, exportación, guardado) **no está definida**. Preguntale a Sergio antes de sumarla al alcance.

## Fuera de alcance (por ahora)

- Todo lo que tenga que ver con la tienda de ropa que hoy está en el repo. Ver [estado-actual.md](estado-actual.md).
