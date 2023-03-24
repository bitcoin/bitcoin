#!/bin/bash

# Clonar o repositório
git clone https://github.com/defiminds/bitcoin.git

# Entrar no diretório do projeto
cd bitcoin

# Instalar as dependências necessárias
sudo apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils
sudo apt-get install libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev
sudo apt-get install libzmq3-dev

# Configurar o projeto
./autogen.sh
./configure

# Construir o projeto
make

# Executar os testes
make check

# Instalar o software (opcional)
sudo make install

# Desinstalar as dependências instaladas
sudo apt-get remove build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils
sudo apt-get remove libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev
sudo apt-get remove libzmq3-dev

# Remover arquivos de configuração e caches
sudo rm -rf /var/cache/apt/archives/* /var/lib/apt/lists/*
sudo apt-get autoremove
sudo apt-get autoclean

# Instalar as dependências necessárias para executar o software
sudo apt-get install libssl1.1 libevent-2.1-6 libboost-system1.71.0 libboost-filesystem1.71.0 libboost-chrono1.71.0 libboost-program-options1.71.0 libboost-test1.71.0 libboost-thread1.71.0 libzmq5
