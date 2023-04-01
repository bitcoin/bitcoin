#!/bin/bash
build() {
echo "If you already the folder please, exec this inside of folder";
read -p "You already have the folder of bitcoin? y/n " choose;
if [ $choose == "n" ]; then
read -p "1) bitcoin/bitcoin repo | 2) defiminds/bitcoin fork? " choose;
  if [ $choose == "1" ]; then
   git clone https://github.com/bitcoin/bitcoin.git
  else
   git clone https://github.com/defiminds/bitcoin.git
  fi
cd bitcoin

# Instalar as dependências necessárias
sudo apt-get update
sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils python3 libssl-dev libevent-dev libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-test-dev libboost-thread-dev libzmq3-dev libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev -y

# Configurar o projeto
./autogen.sh
read -p "Você deseja instalar a Bitcoin-Qt? [y/n] " install_qt
if [ "$install_qt" == "y" ]; then
./configure --enable-gui
# Construir o projeto
make
cd ..
echo "[Desktop Entry]
Name=Bitcoin Core
Exec=$PWD/bitcoin/src/qt/bitcoin-qt
Icon=$PWD/bitcoin/src/qt/res/icons/bitcoin.png
Terminal=false
Type=Application
Categories=Office;Finance;" > bitcoin-qt.desktop
chmod +x bitcoin-qt.desktop
mv bitcoin-qt.desktop ~/Desktop/
echo "Compilation Sucessfully!"
# Executar os testes
make check
# Instalar o software (opcional)
sudo make install
else
./configure
# Construir o projeto
make
# Instalar o software (opcional)
sudo make install
fi

# Desinstalar as dependências instaladas
sudo apt-get remove build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils
sudo apt-get remove libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev
sudo apt-get remove libzmq3-dev

# Remover arquivos de configuração e caches
sudo rm -rf /var/cache/apt/archives/* /var/lib/apt/lists/*
sudo apt-get autoremove
sudo apt-get autoclean

# Instalar as dependências necessárias para executar o software
sudo apt-get install libssl1.1 libevent-2.1-6 libboost-system1.71.0 libboost-filesystem1.71.0 libboost-chrono1.71.0 libboost-program-options1.71.0 libboost-test1.71.0 libboost-thread1.71.0 libzmq5 libevent-pthreads-2.1-7

# Criar arquivo bitcoin.conf
read -p "You need create the bitcoin.conf? y/n " q
if [ $q == "y" ]; then
if ! command -v basez &> /dev/null; then
    sudo apt-get update && sudo apt-get install basez
    echo "Basez installed, BASE16/HEX Avaible on kernel now!"
fi
a="2320426974636f696e20436f726520636f6e66696775726174696f6e2066696c650a2320466f72206d6f726520696e666f726d6174696f6e3a2068747470733a2f2f626974636f696e2e6f72672f656e2f66756c6c2d6e6f646523636f6e66696775726174696f6e2d66696c650a0a23204e6574776f726b0a746573746e65743d300a0a23205365727665720a7365727665723d310a72706362696e643d3132372e302e302e310a727063616c6c6f7769703d3132372e302e302e310a727063757365723d3c796f75722072706320757365726e616d653e0a72706370617373776f72643d3c796f7572207270632070617373776f72643e0a0a23205072756e696e670a7072756e653d3535300a646263616368653d313032340a6d61786d656d706f6f6c3d3530300a0a2320503250204e6574776f726b696e670a6d6178636f6e6e656374696f6e733d34300a62616e73636f72653d31303030300a62616e74696d653d3630343830300a77686974656c6973743d3132372e302e302e310a0a2320507269766163790a64697361626c6577616c6c65743d310a6c697374656e6f6e696f6e3d300a6f6e6c796e65743d697076340a70726f78793d3132372e302e302e313a393035300a0a232053656375726974790a646263726173687665726966793d310a77616c6c657464697361626c65707269766b6579733d310a6d6178736967636163686573697a653d310a6d617873637269707476616c69646174696f6e746872656164733d310a6d6178737461636b6d656d6f72797573616765636f6e73656e7375733d313030303030300a636865636b706f696e74733d300a6d617875706c6f61647461726765743d353030300a0a2320446562756767696e670a64656275673d300a6c6f676970733d310a6c6f6774696d657374616d70733d310a75706e703d300a"
mkdir -p ~/.bitcoin && echo -ne -n -e "${a}" > ~/.bitcoin/bitcoin.conf
b=$(base16 -d ~/.bitcoin/bitcoin.conf) && echo -ne -n -e "${b}" > ~/.bitcoin/bitcoin.conf
else
exit;
fi
}
build
