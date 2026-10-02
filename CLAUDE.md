# Proyecto Carro DC

## Regla de documentación: mantener `README.md` al día

Después de cada decisión tomada o modificación de código, actualiza `README.md` en el mismo turno:

- **Si cambia una decisión:** actualiza solo `## Decisiones tomadas` y, si afecta a materiales, `## Checklist de materiales`. Sobrescribe lo que quede obsoleto y añade lo nuevo. Nunca dejes alternativas descartadas ni discusiones.
- **Si cambia el código:** actualiza solo `## Código`. Hay una fila por archivo con su estado y lo que hace, y se sobrescribe; no es un historial. Anota ahí los pines y valores clave cuando se definan.
- **Si cambian las dos cosas:** actualiza ambas secciones.
- Mantén el README limpio y compacto. Da prioridad a lo que el usuario pida guardar y a lo necesario para retomar el proyecto.
- Escribe en español.
- **Estado actual:** al cerrar cada bloque de trabajo, sobrescribe `## Estado actual` con la fecha, lo último que se hizo y los próximos pasos o pendientes. Como mucho, unas 5 líneas.

## Palabra clave `contexto`

Cuando el usuario escriba **contexto**, lee `README.md` y resume brevemente:
1. En qué se quedó el trabajo la última vez (`## Estado actual`).
2. Los próximos pasos y pendientes.
3. Las decisiones clave, solo si ayudan a retomar.

No hagas cambios hasta que el usuario lo pida.

## Convenciones

- Respeta la estructura de `README.md` (`## Estructura`). Solo se crean subcarpetas nuevas cuando haya contenido que lo justifique.
- Cada `.ino` va en una carpeta con su mismo nombre (lo exige el Arduino IDE).
- El firmware principal va en `firmware/carro_dc/`. Las pruebas sueltas van en `pruebas/<nombre>/<nombre>.ino`.
- El proyecto de Unity 6 va en `unity/CarroDC/`.
- Es un repo git (`github.com/COKII5/Auto_DC`). No hagas commit ni push sin que el usuario lo pida.
