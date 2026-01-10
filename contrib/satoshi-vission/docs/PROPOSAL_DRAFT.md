# Borrador de propuesta: Satoshi Vission

## Resumen
Satoshi Vission es un conjunto de herramientas experimentales para facilitar pruebas mínimas, inspección de directorios y documentación complementaria alrededor de Bitcoin Core. No propone cambios en reglas de consenso ni en validación.

## Motivación
- Estandarizar verificaciones mínimas locales.
- Facilitar reportes rápidos de estado del directorio de trabajo.
- Mantener un espacio de experimentación aislado en `contrib/`.

## Alcance
- Scripts utilitarios que no afectan la lógica de consenso.
- Documentación de objetivos y uso.

## Fuera de alcance
- Cambios de consenso o de comportamiento crítico del nodo.
- Modificación de reglas de validación de bloques o transacciones.

## Diseño propuesto
- `bin/sv_smoke.py`: ejecuta comprobaciones mínimas (existencia de directorios, presencia de archivos clave, salida estandarizada).
- `bin/sv_report.py`: genera un reporte local del árbol de `contrib/satoshi-vission`.

## Consideraciones de seguridad
Estos scripts deben operar en modo solo lectura y evitar ejecutar comandos externos con entradas no confiables.

## Plan de implementación
1. Añadir estructura de directorios bajo `contrib/satoshi-vission`.
2. Implementar scripts mínimos y documentarlos.
3. Mantener un README con advertencias explícitas de no-consenso.

## Estado
Borrador inicial para revisión interna.
