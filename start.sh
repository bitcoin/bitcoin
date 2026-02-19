#!/usr/bin/env bash

# Bitcoin Core - Quick Start Script
# This script builds the WASM module and starts the frontend development server
# 
# Usage: ./start.sh

set -euo pipefail  # Exit on error, undefined variables, and pipe failures

# Color codes for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

# Log functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

log_step() {
    echo -e "${BLUE}[STEP]${NC} $*"
}

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WASM_DIR="${SCRIPT_DIR}/pdf-utils/wasm"
APP_DIR="${SCRIPT_DIR}/app"

# Check prerequisites
check_prerequisites() {
    log_step "Checking prerequisites..."
    
    local missing_tools=()
    
    # Check for Node.js
    if ! command -v node &> /dev/null; then
        missing_tools+=("Node.js (https://nodejs.org/)")
    else
        local node_version
        node_version=$(node --version | sed 's/v//')
        log_info "Node.js version: $node_version"
    fi
    
    # Check for npm or yarn
    if ! command -v npm &> /dev/null && ! command -v yarn &> /dev/null; then
        missing_tools+=("npm or yarn")
    else
        if command -v yarn &> /dev/null; then
            local yarn_version
            yarn_version=$(yarn --version)
            log_info "Yarn version: $yarn_version"
        elif command -v npm &> /dev/null; then
            local npm_version
            npm_version=$(npm --version)
            log_info "npm version: $npm_version"
        fi
    fi
    
    # Check for Rust (optional for WASM build)
    if ! command -v cargo &> /dev/null; then
        log_warn "Rust/Cargo is not installed. WASM build will be skipped."
        log_warn "To install Rust: https://rustup.rs/"
    else
        local rust_version
        rust_version=$(rustc --version)
        log_info "Rust: $rust_version"
    fi
    
    # Check for wasm-pack (optional for WASM build)
    if ! command -v wasm-pack &> /dev/null && command -v cargo &> /dev/null; then
        log_warn "wasm-pack is not installed. WASM build will be skipped."
        log_warn "To install wasm-pack: cargo install wasm-pack"
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        log_error "Missing required tools:"
        for tool in "${missing_tools[@]}"; do
            echo "  - $tool"
        done
        exit 1
    fi
    
    log_info "All required prerequisites satisfied"
}

# Build WASM module
build_wasm() {
    log_step "Building WASM module..."
    
    if ! command -v cargo &> /dev/null || ! command -v wasm-pack &> /dev/null; then
        log_warn "Skipping WASM build (Rust or wasm-pack not installed)"
        log_warn "The app will run without WASM functionality"
        return 0
    fi
    
    if [ ! -d "$WASM_DIR" ]; then
        log_warn "WASM directory not found at $WASM_DIR"
        log_warn "Skipping WASM build"
        return 0
    fi
    
    cd "$WASM_DIR"
    
    if [ -f "./generate_wasm.sh" ]; then
        log_info "Running WASM build script..."
        bash ./generate_wasm.sh
        log_info "WASM module built successfully"
    else
        log_warn "WASM build script not found, skipping WASM build"
    fi
    
    cd "$SCRIPT_DIR"
}

# Install frontend dependencies
install_dependencies() {
    log_step "Installing frontend dependencies..."
    
    if [ ! -d "$APP_DIR" ]; then
        log_error "App directory not found at $APP_DIR"
        exit 1
    fi
    
    cd "$APP_DIR"
    
    if [ -f "yarn.lock" ] && command -v yarn &> /dev/null; then
        log_info "Using yarn to install dependencies..."
        yarn install
    elif command -v npm &> /dev/null; then
        log_info "Using npm to install dependencies..."
        npm install
    else
        log_error "No package manager available"
        exit 1
    fi
    
    log_info "Dependencies installed successfully"
    
    cd "$SCRIPT_DIR"
}

# Start development server
start_dev_server() {
    log_step "Starting development server..."
    
    cd "$APP_DIR"
    
    echo ""
    log_info "============================================"
    log_info "Starting Bitcoin Core Frontend"
    log_info "============================================"
    log_info "The application will be available at:"
    log_info "  http://localhost:3000"
    log_info ""
    log_info "Press Ctrl+C to stop the server"
    log_info "============================================"
    echo ""
    
    # Prefer yarn if available, otherwise use npm
    if [ -f "yarn.lock" ] && command -v yarn &> /dev/null; then
        yarn dev
    else
        npm run dev
    fi
}

# Display welcome message
display_welcome() {
    echo ""
    echo "============================================"
    echo "  Bitcoin Core - Quick Start Script"
    echo "============================================"
    echo ""
}

# Main execution
main() {
    display_welcome
    check_prerequisites
    build_wasm
    install_dependencies
    start_dev_server
}

# Run main function
main "$@"
