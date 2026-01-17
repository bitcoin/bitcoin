# ZKP Peer Auth (diseño preliminar)

## Objetivos de privacidad

* **Autenticación sin revelar identidad**: permitir que dos pares demuestren pertenencia a un conjunto de autorizados sin exponer claves públicas ni metadatos duraderos.
* **Minimización de correlación**: la prueba debe ser válida sólo para la sesión, evitando que observadores externos vinculen conexiones entre sí.
* **No filtración de política local**: la negociación no debe revelar explícitamente listas de permitidos ni criterios de admisión; sólo un bit de capacidad y un resultado (aceptar/rechazar).

## Modelo de amenazas

* **Observador pasivo**: intenta correlacionar conexiones por datos persistentes en `version/verack` o mensajes nuevos.
* **Atacante activo**: abre muchas conexiones para extraer información sobre el conjunto autorizado o provocar trabajo excesivo.
* **MitM**: intenta reusar pruebas fuera de contexto o suplantar a un par.

Fuera de alcance: defensa contra comprometimiento total de la clave de autenticación del conjunto o contra adversarios con control del endpoint.

## Mensajes nuevos o extensiones a `version/verack`

Se proponen dos alternativas, con preferencia por la segunda:

1. **Mensaje nuevo `zpk`** (post-`verack`):
   * `verack` mantiene semántica actual y se introduce un mensaje `zpk` que contiene la prueba y un contexto de sesión.
   * Ventaja: evita sobrecargar `version` y permite rechazar silenciosamente si no se negocia el bit.

2. **Extensión a `version` con TLV opcional**:
   * Añadir un campo opcional codificado como TLV (solo si se negocia la capacidad) para transportar un compromiso de prueba.
   * La prueba completa se enviaría tras `verack` para evitar que peers antiguos fallen el parsing del `version`.

En ambos casos, la negociación debe realizarse por un **feature bit** anunciado en `version` dentro del campo de servicios (ver propuesta de `NODE_ZKP_AUTH` en `src/protocol.h`) y validado en el handshake.

## Compatibilidad hacia atrás

* **Peers antiguos**: si no anuncian el bit, no se envía `zpk` ni TLV, y se continúa con el handshake estándar.
* **Peers mixtos**: si un peer anuncia el bit pero no provee prueba válida, se desconecta con un motivo claro, preservando la compatibilidad con peers que nunca anunciaron el bit.
* **Implementaciones parciales**: el bit sólo debe anunciarse cuando el nodo esté preparado para validar pruebas, evitando estados ambiguos.

## Consideraciones de DoS

* **Early exit**: si no se negocia el bit, omitir validación y cualquier verificación costosa.
* **Rate limiting**: limitar intentos de autenticación fallida por IP/ASN, reutilizando contadores existentes del handshake.
* **Coste acotado**: la prueba debe verificarse en tiempo acotado y con tamaño máximo estricto para evitar parseos costosos.
* **Orden del trabajo**: validar primero tamaño/estructura y sólo luego ejecutar verificación criptográfica.

## Puntos de integración propuestos

* **`src/protocol.h`**: definir un nuevo bit de servicios (p. ej., `NODE_ZKP_AUTH`) y, si se usa mensaje nuevo, añadir el nombre del mensaje `zpk` a la lista de tipos válidos. Esto centraliza la negociación en el campo `services` de `version`.
* **`src/net_processing.cpp`**:
  * Durante el procesamiento de `version`, registrar si el peer anuncia `NODE_ZKP_AUTH` para decidir si exigir prueba.
  * Tras `verack`, aceptar `zpk` y validar la prueba antes de marcar al peer como completamente conectado.
  * Rechazar de forma temprana (y con ban score apropiado) pruebas inválidas o fuera de contexto de sesión.

Estos puntos permiten que la negociación del feature bit ocurra en el handshake estándar y que la validación de la prueba se ubique en el flujo de mensajes existente, sin romper compatibilidad con peers que no soporten la extensión.
