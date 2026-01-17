#!/data/data/com.termux/files/usr/bin/bash
# ===================================================
# YOUPHRN OES - ORQUESTADOR PRINCIPAL (Producción)
# Propósito: Iniciar y gestionar los servicios de RMN-3301
# Uso: ./youphrn_oes-init.prod.sh [start|stop|status|menu]
# ===================================================

# --- CONFIGURACIÓN DEL SISTEMA ---
BASE_DIR="$HOME/rmn-3301"
LOG_DIR="$BASE_DIR/logs"
PID_DIR="$BASE_DIR/run"
CONFIG_DIR="$BASE_DIR/config"

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# --- FUNCIONES DEL SISTEMA ---

initialize_system() {
    echo -e "${BLUE}[RMN-3301]${NC} Inicializando sistema..."

    # 1. Verificar entorno
    check_environment

    # 2. Crear estructura de directorios
    mkdir -p "$LOG_DIR" "$PID_DIR" "$CONFIG_DIR"

    # 3. Iniciar servicios base
    start_service "tor" "Tor Network Daemon"
    start_service "quantumcast-core" "QuantumCast Core System"

    # 4. Configurar GrapheneOS integration
    configure_grapheneos

    echo -e "${GREEN}[✓]${NC} Sistema inicializado. Ejecuta '$0 menu' para opciones."
}

check_environment() {
    echo -e "${BLUE}[*]${NC} Verificando entorno..."

    # Verificar que estamos en Termux
    if [[ ! -d "/data/data/com.termux" ]]; then
        echo -e "${RED}[ERROR]${NC} Este sistema debe ejecutarse en Termux (Android)"
        exit 1
    fi

    # Verificar GrapheneOS (aproximación)
    if [[ $(getprop ro.build.version.security_patch) == "" ]]; then
        echo -e "${YELLOW}[!]${NC} No se detectó GrapheneOS. Algunas funciones pueden no estar disponibles."
    fi

    # Verificar Node.js
    if ! command -v node &> /dev/null; then
        echo -e "${RED}[ERROR]${NC} Node.js no está instalado. Ejecuta: pkg install nodejs"
        exit 1
    fi

    # Verificar módulos compilados
    if [[ ! -d "$BASE_DIR/packages/quantumcast-core/dist" ]]; then
        echo -e "${RED}[ERROR]${NC} Módulos no compilados. Ejecuta 'npm run build' primero."
        exit 1
    fi
}

start_service() {
    local service_name="$1"
    local display_name="$2"
    local pid_file="$PID_DIR/$service_name.pid"

    if [[ -f "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null; then
        echo -e "${YELLOW}[*]${NC} $display_name ya está en ejecución (PID: $(cat "$pid_file"))"
        return
    fi

    case "$service_name" in
        "tor")
            # Iniciar Tor con configuración personalizada
            tor -f "$CONFIG_DIR/torrc.production" \
                --Log "notice file $LOG_DIR/tor.log" \
                --DataDirectory "$BASE_DIR/tor-data" &
            echo $! > "$pid_file"
            ;;
        "quantumcast-core")
            # Iniciar el núcleo del sistema
            node "$BASE_DIR/packages/quantumcast-core/dist/daemon.js" \
                --config "$CONFIG_DIR/core.json" \
                >> "$LOG_DIR/core.log" 2>&1 &
            echo $! > "$pid_file"
            ;;
        "pattern-obfuscator")
            node "$BASE_DIR/packages/pattern-obfuscator/dist/obfuscator.js" \
                --mode "adaptive" \
                --intensity "medium" \
                >> "$LOG_DIR/obfuscator.log" 2>&1 &
            echo $! > "$pid_file"
            ;;
    esac

    sleep 2
    if kill -0 "$(cat "$pid_file")" 2>/dev/null; then
        echo -e "${GREEN}[✓]${NC} $display_name iniciado (PID: $(cat "$pid_file"))"
    else
        echo -e "${RED}[ERROR]${NC} Fallo al iniciar $display_name"
        rm -f "$pid_file"
    fi
}

stop_service() {
    local service_name="$1"
    local display_name="$2"
    local pid_file="$PID_DIR/$service_name.pid"

    if [[ -f "$pid_file" ]]; then
        local pid
        pid=$(cat "$pid_file")
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid"
            echo -e "${GREEN}[✓]${NC} $display_name detenido (PID: $pid)"
        fi
        rm -f "$pid_file"
    else
        echo -e "${YELLOW}[*]${NC} $display_name no está en ejecución"
    fi
}

configure_grapheneos() {
    echo -e "${BLUE}[*]${NC} Configurando integración con GrapheneOS..."

    # Nota: Estas son recomendaciones. GrapheneOS no permite desactivar hardware sin root.
    echo -e "${YELLOW}[!] RECOMENDACIONES MANUALES:${NC}"
    echo "  1. Ve a Ajustes > Apps > Termux > Permisos: DESACTIVA Cámara, Micrófono, Ubicación"
    echo "  2. Crea un 'Perfil de trabajo' para aislar aplicaciones sensibles"
    echo "  3. Usa el firewall de GrapheneOS para bloquear conexiones no esenciales"
    echo "  4. Desactiva 'Sensores' en los permisos de cada app bunkerizada"

    # Configuración real que SÍ podemos hacer:
    # 1. Configurar proxy para todas las apps via Termux (requiere root, solo anotamos)
    echo "# Configuración proxy para Tor (requiere configuración manual)" > "$CONFIG_DIR/proxy-settings.txt"
    echo "SOCKS5: localhost:9050" >> "$CONFIG_DIR/proxy-settings.txt"
}

launch_bunker() {
    local bunker_name="$1"

    case "$bunker_name" in
        "instagram")
            echo -e "${BLUE}[*]${NC} Iniciando Bunker Instagram..."
            node "$BASE_DIR/packages/bunker-instagram/dist/cli.js" \
                --tor-proxy "localhost:9050" \
                --isolation-level "strict"
            ;;
        "protonmail")
            echo -e "${BLUE}[*]${NC} Iniciando Bunker ProtonMail..."
            node "$BASE_DIR/packages/bunker-protonmail/dist/cli.js" \
                --tor-proxy "localhost:9050" \
                --encryption "end-to-end"
            ;;
        "electrum")
            echo -e "${BLUE}[*]${NC} Iniciando Bunker Electrum..."
            node "$BASE_DIR/packages/bunker-electrum/dist/cli.js" \
                --tor-proxy "localhost:9050" \
                --wallet "isolated"
            ;;
        "x")
            echo -e "${BLUE}[*]${NC} Iniciando Bunker X/Twitter..."
            node "$BASE_DIR/packages/bunker-x/dist/cli.js" \
                --tor-proxy "localhost:9050" \
                --obfuscation "high"
            ;;
        *)
            echo -e "${RED}[ERROR]${NC} Bunker desconocido: $bunker_name"
            return 1
            ;;
    esac
}

show_menu() {
    while true; do
        clear
        echo -e "${BLUE}========================================${NC}"
        echo -e "${GREEN}    YOUPHRN OES - RMN-3301 MENU${NC}"
        echo -e "${BLUE}========================================${NC}"
        echo -e "1) ${YELLOW}Iniciar todos los servicios${NC}"
        echo -e "2) ${YELLOW}Detener todos los servicios${NC}"
        echo -e "3) ${YELLOW}Estado del sistema${NC}"
        echo -e "4) ${YELLOW}Bunker Instagram${NC}"
        echo -e "5) ${YELLOW}Bunker ProtonMail${NC}"
        echo -e "6) ${YELLOW}Bunker Electrum${NC}"
        echo -e "7) ${YELLOW}Bunker X/Twitter${NC}"
        echo -e "8) ${YELLOW}Generar tráfico de cobertura${NC}"
        echo -e "9) ${YELLOW}Limpiar logs y temporales${NC}"
        echo -e "0) ${RED}Salir${NC}"
        echo -e "${BLUE}========================================${NC}"

        read -r -p "Selecciona una opción (0-9): " choice

        case $choice in
            1) initialize_system ;;
            2) stop_all_services ;;
            3) show_status ;;
            4) launch_bunker "instagram" ;;
            5) launch_bunker "protonmail" ;;
            6) launch_bunker "electrum" ;;
            7) launch_bunker "x" ;;
            8) start_service "pattern-obfuscator" "Pattern Obfuscator" ;;
            9) cleanup_system ;;
            0) exit 0 ;;
            *) echo -e "${RED}Opción inválida${NC}"; sleep 1 ;;
        esac

        echo -e "\nPresiona Enter para continuar..."
        read -r
    done
}

stop_all_services() {
    echo -e "${BLUE}[*]${NC} Deteniendo todos los servicios..."
    stop_service "pattern-obfuscator" "Pattern Obfuscator"
    stop_service "quantumcast-core" "QuantumCast Core"
    stop_service "tor" "Tor Network"
    echo -e "${GREEN}[✓]${NC} Todos los servicios detenidos"
}

show_status() {
    echo -e "${BLUE}[*]${NC} Estado del sistema RMN-3301:"
    echo -e "${BLUE}----------------------------------------${NC}"

    for pid_file in "$PID_DIR"/*.pid; do
        if [[ -f "$pid_file" ]]; then
            service_name=$(basename "$pid_file" .pid)
            pid=$(cat "$pid_file")
            if kill -0 "$pid" 2>/dev/null; then
                echo -e "${GREEN}[ACTIVO]${NC} $service_name (PID: $pid)"
            else
                echo -e "${RED}[INACTIVO]${NC} $service_name (PID muerto: $pid)"
                rm -f "$pid_file"
            fi
        fi
    done

    # Mostrar uso de recursos
    echo -e "\n${BLUE}[*]${NC} Uso de recursos:"
    echo "Tor Log: $(tail -1 "$LOG_DIR/tor.log" 2>/dev/null || echo "No disponible")"
    echo "Conexiones activas: $(netstat -an 2>/dev/null | grep ":9050" | wc -l)"
}

cleanup_system() {
    echo -e "${BLUE}[*]${NC} Limpiando sistema..."

    # 1. Detener servicios
    stop_all_services

    # 2. Cifrar logs antiguos
    for log_file in "$LOG_DIR"/*.log; do
        if [[ -f "$log_file" ]]; then
            echo "[*] Cifrando $(basename "$log_file")..."
            openssl enc -aes-256-cbc -salt -in "$log_file" \
                -out "$log_file.enc" \
                -pass pass:"$(date +%s)$RANDOM" 2>/dev/null
            shred -u "$log_file" 2>/dev/null
        fi
    done

    # 3. Limpiar datos temporales de Tor
    if [[ -d "$BASE_DIR/tor-data" ]]; then
        find "$BASE_DIR/tor-data" -type f -exec shred -u {} \; 2>/dev/null
    fi

    echo -e "${GREEN}[✓]${NC} Limpieza completada. No hay rastros."
}

# --- MANEJO DE ARGUMENTOS ---
case "$1" in
    "start")
        initialize_system
        ;;
    "stop")
        stop_all_services
        ;;
    "status")
        show_status
        ;;
    "menu")
        show_menu
        ;;
    "clean")
        cleanup_system
        ;;
    *)
        echo -e "${BLUE}Uso: $0 [start|stop|status|menu|clean]${NC}"
        echo "  start   - Iniciar todos los servicios"
        echo "  stop    - Detener todos los servicios"
        echo "  status  - Mostrar estado del sistema"
        echo "  menu    - Mostrar menú interactivo"
        echo "  clean   - Limpieza segura y cifrado de logs"
        exit 1
        ;;
esac
