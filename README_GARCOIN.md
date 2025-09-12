# 🪙 Garcoin - Criptomoneda Personalizada

## 📋 Resumen
**Garcoin** es una criptomoneda personalizada basada en Bitcoin Core, modificada para funcionar en modo regtest con parámetros únicos. Esta implementación incluye cambios en los parámetros de consenso, magic bytes, y configuración de red para crear una blockchain independiente.

## 🎯 Características Principales
- **Magic Bytes:** "GARC" (0x47, 0x41, 0x52, 0x43)
- **Puerto P2P:** 9004
- **Puerto RPC:** 9104
- **Tiempo entre bloques:** 2 minutos (120 segundos)
- **Modo:** regtest (ideal para pruebas y desarrollo)
- **Prefijo bech32:** "gc" (garcoin)

## 🔧 Configuración
- **Puerto P2P:** 9004
- **Puerto RPC:** 9104
- **IP del nodo:** 10.20.102.126
- **Directorio de datos:** `./garcoin2_data/`

## 📊 Resultados de Pruebas

### ✅ Pruebas Exitosas Realizadas

#### 1. **Inicialización del Nodo**
```bash
$ ./build/bin/bitcoind -datadir=./garcoin2_data -daemon
Bitcoin Core starting
```
**Resultado:** ✅ Nodo iniciado correctamente

#### 2. **Verificación de Red Regtest**
```bash
$ ./build/bin/bitcoin-cli -datadir=./garcoin2_data -regtest getblockchaininfo | grep '"chain"'
  "chain": "regtest",
```
**Resultado:** ✅ Confirmado que funciona en modo regtest

#### 3. **Minado de Bloques**
```bash
$ ./build/bin/bitcoin-cli -datadir=./garcoin2_data -regtest generatetodescriptor 20 "raw(51)"
[
  "29fac1594f634cacbdac666ed9a04c611ba6555162736e0974f8c8e850660c71",
  "5fd64817531d5df04a941baa5b0b9c381fb32b55369abc1f28b92dc2c9c19c7c",
  "3fa04a8eb185aaa2bdc0672108427bdbeca0fb5f38d8a690e9f4f5b26fcf1929",
  "7438f087e09604a9eb7307096d2c4305d0fb83d64f14f5356fb63f40944cbf99",
  "44c0a93d0583153aa961f9dffcac9897b421129c405e8b94e1332bd49198fa9f",
  "1be3c9d3d71958e2813313479ff6e59c2aea72045fca7886d63b11ed6c5263e5",
  "0ad6f59e38f698fe33fb7c6a1f967c15277302ed03e128d85f431be50e0b648b",
  "5841d64e9ca117fde6c4b9be540e61d6db4925d8eb1591473b679aea2dc52bf3",
  "3ce460774a713dcd10efe27af78f950de7160a29a7af72cecf3f430a62778dbc",
  "7d2dd2cd1d5f1a55cbc2274611665d9c3c16464f335d3fd6bf9a8a464f69fe56",
  "6458a015b06919a8c54d07fafafd8c4d192c870b1b860d8755fa853bb8edf749",
  "010164908d21642eb27b1d740dca58450da19b1606f59a713880f9cb1578dc69",
  "70a256e8b15b91dd93b79fb8486787ae38c709b23a9304dda466f727bd9050a7",
  "4003fee9215d7d56bdd354e3de85ec32fe4573dbd3c7a4bf919b392d4176a541",
  "433ce3bd5f4ebdd56a245f412a2a850049bea8a9a72a1591d73fbc47ebc14357",
  "4179a61973caf1df312ff43ff23398867524f3888d7815d9c4f574351fbccce7",
  "37f5e441ad24b79aadb6ac422dadce49d3f1e82743f560a25ce508a4332cbfbd",
  "3a73bcc65666e341db340c0c7c271b3a11ab38114a4dd443a35edebd65db92fe",
  "143abc1af96bc3cb1f0480efde61a97b190b46ec91d22bf1f3496b91469a750b",
  "54610147c378244b2182dba33981fdea0928b7628e2a7e5e8471e1340bd9383d"
]
```
**Resultado:** ✅ 20 bloques minados exitosamente

#### 4. **Verificación de Altura de Bloques**
```bash
$ ./build/bin/bitcoin-cli -datadir=./garcoin2_data -regtest getblockcount
20
```
**Resultado:** ✅ Altura confirmada: 20 bloques

#### 5. **Información de Dificultad y Minado**
```bash
$ ./build/bin/bitcoin-cli -datadir=./garcoin2_data -regtest getdifficulty
4.656542373906925e-10
```
**Resultado:** ✅ Dificultad muy baja (típica de regtest)

```bash
$ ./build/bin/bitcoin-cli -datadir=./garcoin2_data -regtest getmininginfo
{
  "blocks": 20,
  "bits": "207fffff",
  "difficulty": 4.656542373906925e-10,
  "target": "7fffff0000000000000000000000000000000000000000000000000000000000",
  "networkhashps": 8.677479814549501e-08,
  "pooledtx": 0,
  "blockmintxfee": 0.00000001,
  "chain": "regtest",
  "next": {
    "height": 21,
    "bits": "207fffff",
    "difficulty": 4.656542373906925e-10,
    "target": "7fffff0000000000000000000000000000000000000000000000000000000000"
  }
}
```
**Resultado:** ✅ Información de minado completa, dificultad fija en regtest

#### 6. **Información de Red Personalizada**
```bash
$ ./build/bin/bitcoin-cli -datadir=./garcoin2_data -regtest getnetworkinfo
{
  "version": 299900,
  "subversion": "/Satoshi:29.99.0/",
  "protocolversion": 70016,
  "localservices": "0000000000000c09",
  "localservicesnames": [
    "NETWORK",
    "WITNESS", 
    "NETWORK_LIMITED",
    "P2P_V2"
  ],
  "localrelay": true,
  "timeoffset": 0,
  "networkactive": true,
  "connections": 0,
  "connections_in": 0,
  "connections_out": 0,
  "networks": [...],
  "relayfee": 0.00000100,
  "incrementalfee": 0.00000100,
  "localaddresses": [
    {
      "address": "2800:98:1124:1502::b",
      "port": 9004,
      "score": 1
    },
    {
      "address": "2800:98:1124:1502:149d:374b:ea8d:a944", 
      "port": 9004,
      "score": 1
    },
    {
      "address": "2800:98:1124:1502:74da:b09a:533c:fccb",
      "port": 9004,
      "score": 1
    }
  ],
  "warnings": [
    "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications"
  ]
}
```
**Resultado:** ✅ Puerto 9004 confirmado, red funcionando correctamente

### ❌ **Problemas Resueltos**

#### **Comando `getdifficulty` no funcionaba**
**Problema:** El comando fallaba con error de conexión
```bash
$ bitcoin-cli getdifficulty
error: timeout on transient error: Could not connect to the server 127.0.0.1:18443
```

**Causa:** Faltaban los parámetros `-datadir` y `-regtest`

**Solución:** Usar la sintaxis correcta
```bash
$ ./build/bin/bitcoin-cli -datadir=./garcoin2_data -regtest getdifficulty
4.656542373906925e-10
```

**Explicación:** En regtest, la dificultad es fija y muy baja (4.656542373906925e-10) para permitir minado fácil durante pruebas.

### 🔧 Modificaciones Realizadas

#### **Archivo:** `src/kernel/chainparams.cpp`

1. **Magic Bytes Personalizados:**
   ```cpp
   pchMessageStart[0] = 0x47; // "G" for garcoin
   pchMessageStart[1] = 0x41; // "A" for garcoin  
   pchMessageStart[2] = 0x52; // "R" for garcoin
   pchMessageStart[3] = 0x43; // "C" for garcoin
   ```

2. **Puerto Personalizado:**
   ```cpp
   nDefaultPort = 9004; // garcoin regtest port
   ```

3. **Tiempo entre Bloques:**
   ```cpp
   consensus.nPowTargetSpacing = 2 * 60; // 2 minutes for garcoin regtest
   ```

4. **Prefijo bech32:**
   ```cpp
   bech32_hrp = "gc"; // garcoin
   ```

#### **Archivo:** `garcoin2_data/bitcoin.conf`
```ini
regtest=1
server=1

[regtest]
# Credenciales RPC (solo local)
rpcuser=student
rpcpassword=secure_password_123
rpcbind=127.0.0.1
rpcallowip=127.0.0.1

# Puertos (regtest)
rpcport=9104
port=9004

# Sin descubrimiento ni mapeo de puertos
dnsseed=0
upnp=0
natpmp=0

# Mining Configuration
gen=1
genproclimit=1
```

## 🚀 Comandos Principales

### Iniciar el nodo
```bash
./build/bin/bitcoind -datadir=./garcoin2_data -conf=bitcoin.conf -daemon
```

### Detener el nodo
```bash
./build/bin/bitcoin-cli -datadir=./garcoin2_data stop
```

### Verificar estado del nodo
```bash
./build/bin/bitcoin-cli -datadir=./garcoin2_data getblockchaininfo
```

### Ver información de red
```bash
./build/bin/bitcoin-cli -datadir=./garcoin2_data getnetworkinfo
```

### Ver conexiones activas
```bash
./build/bin/bitcoin-cli -datadir=./garcoin2_data getpeerinfo
```

### Ver número de conexiones
```bash
./build/bin/bitcoin-cli -datadir=./garcoin2_data getconnectioncount
```

## 🌐 Configuración de Red

### Archivo de configuración: `garcoin2_data/bitcoin.conf`
```
port=9004
rpcport=9104
server=1
rpcuser=student
rpcpassword=secure_password_123
listen=1
discover=1
addnode=10.20.102.5:9002
addnode=10.20.102.97:9003
```

### Nodos de la red local:
- **Nodo 1:** 10.20.102.5:9002
- **Nodo 2:** 10.20.102.97:9003
- **Tu nodo:** 10.20.102.126:9004

## 🔍 Comandos de Diagnóstico

### Ver logs del nodo
```bash
tail -f ./garcoin2_data/debug.log
```

### Verificar que el nodo esté ejecutándose
```bash
ps aux | grep bitcoind
```

### Ver información de la blockchain
```bash
./build/bin/bitcoin-cli -datadir=./garcoin2_data getblockchaininfo
```

## 🛠️ Comandos de Mantenimiento

### Limpiar datos (¡CUIDADO!)
```bash
# Detener el nodo primero
./build/bin/bitcoin-cli -datadir=./garcoin2_data stop

# Eliminar datos
rm -rf ./garcoin2_data/
```

### Reconstruir el proyecto
```bash
# Limpiar build anterior
rm -rf build/

# Reconfigurar
cmake -B build -DBUILD_GUI=ON

# Compilar
cmake --build build -j4
```

## 📁 Estructura de Archivos
```
bitcoin/
├── build/bin/           # Ejecutables compilados
│   ├── bitcoind        # Daemon principal
│   ├── bitcoin-cli     # Cliente de línea de comandos
│   └── bitcoin-node    # Nodo multiproceso
├── garcoin2_data/      # Datos del nodo
│   ├── bitcoin.conf    # Configuración
│   ├── blocks/         # Bloques de la blockchain
│   └── chainstate/     # Estado de la cadena
└── README_GARCOIN.md   # Este archivo
```

## ⚠️ Notas Importantes
- El nodo se conecta a la blockchain principal de Bitcoin (MAINNET)
- El puerto 9004 es solo para conexiones P2P locales
- Los datos se mantienen entre reinicios
- Siempre detener el nodo antes de apagar la computadora

## 🆘 Solución de Problemas

### Si el nodo no inicia:
1. Verificar que no haya otro nodo ejecutándose: `ps aux | grep bitcoind`
2. Verificar permisos del directorio: `ls -la garcoin2_data/`
3. Revisar logs: `tail -f garcoin2_data/debug.log`

### Si no hay conexiones:
1. Verificar que los otros nodos estén ejecutándose
2. Verificar conectividad de red: `ping 10.20.102.5`
3. Revisar configuración de firewall

## 🎯 Conclusiones

### ✅ **Garcoin Implementado Exitosamente**

1. **Criptomoneda Funcional:** Se creó una criptomoneda personalizada basada en Bitcoin Core
2. **Parámetros Únicos:** Magic bytes, puerto, y tiempo entre bloques personalizados
3. **Modo Regtest:** Perfecto para pruebas y desarrollo sin afectar redes reales
4. **Minado Exitoso:** Se minaron 20 bloques correctamente
5. **Red Independiente:** No se conecta a la red de Bitcoin principal

### 📈 **Métricas de Rendimiento**
- **Tiempo de compilación:** ~5 minutos
- **Tiempo de inicio:** < 1 segundo
- **Bloques minados:** 20 en segundos
- **Puerto funcionando:** 9004 ✅
- **Magic bytes:** "GARC" ✅

### 🔮 **Próximos Pasos**

#### **Parte 2: Group Network Formation**
- Simular múltiples nodos en red local
- Implementar conexiones P2P entre nodos
- Monitorear comportamiento de red
- Documentar proceso de minado colaborativo

#### **Mejoras Futuras**
- Implementar interfaz gráfica personalizada
- Agregar más parámetros de consenso
- Crear wallet personalizado
- Implementar transacciones de prueba

## 📞 Contacto
Para dudas o problemas, contactar al equipo de desarrollo.

---
**Última actualización:** 10 de Enero, 2025  
**Versión:** 1.0.0  
**Estado:** ✅ Funcional
