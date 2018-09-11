#!/usr/bin/env bash
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
# What to do
sign=false
verify=false
build=false

# Systems to build
linux=true
windows=true
osx=true

# Other Basic variables
SIGNER=
VERSION=
commit=false
url=https://github.com/bitcoin/bitcoin
proc=2
mem=2000
lxc=true
osslTarUrl=http://downloads.sourceforge.net/project/osslsigncode/osslsigncode/osslsigncode-1.7.1.tar.gz
osslPatchUrl=https://bitcoincore.org/cfields/osslsigncode-Backports-to-1.7.1.patch
scriptName=$(basename -- "$0")
signProg="gpg --detach-sign"
commitFiles=true

# Help Message
read -d '' usage <<- EOF
Usage: $scriptName [-c|u|v|b|s|B|o|h|j|m|] signer version

Run this script from the directory containing the bitcoin, gitian-builder, gitian.sigs, and bitcoin-detached-sigs.

Arguments:
signer          GPG signer to sign each build assert file
version		Version number, commit, or branch to build. If building a commit or branch, the -c option must be specified

Options:
-c|--commit	Indicate that the version argument is for a commit or branch
-u|--url	Specify the URL of the repository. Default is https://github.com/bitcoin/bitcoin
-v|--verify 	Verify the Gitian build
-b|--build	Do a Gitian build
-s|--sign	Make signed binaries for Windows and Mac OSX
-B|--buildsign	Build both signed and unsigned binaries
-o|--os		Specify which Operating Systems the build is for. Default is lwx. l for linux, w for windows, x for osx
-j		Number of processes to use. Default 2
-m		Memory to allocate in MiB. Default 2000
--kvm           Use KVM instead of LXC
--setup         Set up the Gitian building environment. Uses LXC. If you want to use KVM, use the --kvm option. Only works on Debian-based systems (Ubuntu, Debian)
--detach-sign   Create the assert file for detached signing. Will not commit anything.
--no-commit     Do not commit anything to git
-h|--help	Print this help message
EOF

# Get options and arguments
while :; do
    case $1 in
        # Verify
        -v|--verify)
	    verify=true
            ;;
        # Build
        -b|--build)
	    build=true
            ;;
        # Sign binaries
        -s|--sign)
	    sign=true
            ;;
        # Build then Sign
        -B|--buildsign)
	    sign=true
	    build=true
            ;;
        # PGP Signer
        -S|--signer)
	    if [ -n "$2" ]
	    then
		SIGNER="$2"
		shift
	    else
		echo 'Error: "--signer" requires a non-empty argument.'
		exit 1
	    fi
           ;;
        # Operating Systems
        -o|--os)
	    if [ -n "$2" ]
	    then
		linux=false
		windows=false
		osx=false
		if [[ "$2" = *"l"* ]]
		then
		    linux=true
		fi
		if [[ "$2" = *"w"* ]]
		then
		    windows=true
		fi
		if [[ "$2" = *"x"* ]]
		then
		    osx=true
		fi
		shift
	    else
		echo 'Error: "--os" requires an argument containing an l (for linux), w (for windows), or x (for Mac OSX)'
		exit 1
	    fi
	    ;;
	# Help message
	-h|--help)
	    echo "$usage"
	    exit 0
	    ;;
	# Commit or branch
	-c|--commit)
	    commit=true
	    ;;
	# Number of Processes
	-j)
	    if [ -n "$2" ]
	    then
		proc=$2
		shift
	    else
		echo 'Error: "-j" requires an argument'
		exit 1
	    fi
	    ;;
	# Memory to allocate
	-m)
	    if [ -n "$2" ]
	    then
		mem=$2
		shift
	    else
		echo 'Error: "-m" requires an argument'
		exit 1
	    fi
	    ;;
	# URL
	-u)
	    if [ -n "$2" ]
	    then
		url=$2
		shift
	    else
		echo 'Error: "-u" requires an argument'
		exit 1
	    fi
	    ;;
        # kvm
        --kvm)
            lxc=false
            ;;
        # Detach sign
        --detach-sign)
            signProg="true"
            commitFiles=false
            ;;
        # Commit files
        --no-commit)
            commitFiles=false
            ;;
        # Setup
        --setup)
            setup=true
            ;;
	*)               # Default case: If no more options then break out of the loop.
             break
    esac
    shift
done

# Set up LXC
if [[ $lxc = true ]]
then
    export USE_LXC=1
fi

# Check for OSX SDK
if [[ ! -e "gitian-builder/inputs/MacOSX10.11.sdk.tar.gz" && $osx == true ]]
then
    echo "Cannot build for OSX, SDK does not exist. Will build for other OSes"
    osx=false
fi

# Get signer
if [[ -n "$1" ]]
then
    SIGNER="$1"
    shift
fi

# Get version
if [[ -n "$1" ]]
then
    VERSION=$1
    COMMIT=$VERSION
    shift
fi

# Check that a signer is specified
if [[ "$SIGNER" == "" ]]
then
    echo "$scriptName: Missing signer."
    echo "Try $scriptName --help for more information"
    exit 1
fi

# Check that a version is specified
if [[ $VERSION == "" ]]
then
    echo "$scriptName: Missing version."
    echo "Try $scriptName --help for more information"
    exit 1
fi

# Add a "v" if no -c
if [[ $commit = false ]]
then
	COMMIT="v${VERSION}"
fi
echo ${COMMIT}

# Setup build environment
if [[ $setup = true ]]
then
    sudo apt-get install ruby apache2 git apt-cacher-ng python-vm-builder qemu-kvm qemu-utils
    git clone https://github.com/bitcoin-core/gitian.sigs.git
    git clone https://github.com/bitcoin-core/bitcoin-detached-sigs.git
    git clone https://github.com/devrandom/gitian-builder.git
    pushd ./gitian-builder
    if [[ -n "$USE_LXC" ]]
    then
        sudo apt-get install lxc
        bin/make-base-vm --suite trusty --arch amd64 --lxc
    else
        bin/make-base-vm --suite trusty --arch amd64
    fi
    popd
fi

# Set up build
pushd ./bitcoin
git fetch
git checkout ${COMMIT}
popd

# Build
if [[ $build = true ]]
then
	# Make output folder
	mkdir -p ./bitcoin-binaries/${VERSION}
	
	# Build Dependencies
	echo ""
	echo "Building Dependencies"
	echo ""
	pushd ./gitian-builder	
	mkdir -p inputs
	wget -N -P inputs $osslPatchUrl
	wget -N -P inputs $osslTarUrl
	make -C ../bitcoin/depends download SOURCES_PATH=`pwd`/cache/common

	# Linux
	if [[ $linux = true ]]
	then
            echo ""
	    echo "Compiling ${VERSION} Linux"
	    echo ""
	    ./bin/gbuild -j ${proc} -m ${mem} --commit bitcoin=${COMMIT} --url bitcoin=${url} ../bitcoin/contrib/gitian-descriptors/gitian-linux.yml
	    ./bin/gsign -p "$signProg" --signer "$SIGNER" --release ${VERSION}-linux --destination ../gitian.sigs/ ../bitcoin/contrib/gitian-descriptors/gitian-linux.yml
	    mv build/out/bitcoin-*.tar.gz build/out/src/bitcoin-*.tar.gz ../bitcoin-binaries/${VERSION}
	fi
	# Windows
	if [[ $windows = true ]]
	then
	    echo ""
	    echo "Compiling ${VERSION} Windows"
	    echo ""
	    ./bin/gbuild -j ${proc} -m ${mem} --commit bitcoin=${COMMIT} --url bitcoin=${url} ../bitcoin/contrib/gitian-descriptors/gitian-win.yml
	    ./bin/gsign -p "$signProg" --signer "$SIGNER" --release ${VERSION}-win-unsigned --destination ../gitian.sigs/ ../bitcoin/contrib/gitian-descriptors/gitian-win.yml
	    mv build/out/bitcoin-*-win-unsigned.tar.gz inputs/bitcoin-win-unsigned.tar.gz
	    mv build/out/bitcoin-*.zip build/out/bitcoin-*.exe ../bitcoin-binaries/${VERSION}
	fi
	# Mac OSX
	if [[ $osx = true ]]
	then
	    echo ""
	    echo "Compiling ${VERSION} Mac OSX"
	    echo ""
	    ./bin/gbuild -j ${proc} -m ${mem} --commit bitcoin=${COMMIT} --url bitcoin=${url} ../bitcoin/contrib/gitian-descriptors/gitian-osx.yml
	    ./bin/gsign -p "$signProg" --signer "$SIGNER" --release ${VERSION}-osx-unsigned --destination ../gitian.sigs/ ../bitcoin/contrib/gitian-descriptors/gitian-osx.yml
	    mv build/out/bitcoin-*-osx-unsigned.tar.gz inputs/bitcoin-osx-unsigned.tar.gz
	    mv build/out/bitcoin-*.tar.gz build/out/bitcoin-*.dmg ../bitcoin-binaries/${VERSION}
	fi
	popd

        if [[ $commitFiles = true ]]
        then
	    # Commit to gitian.sigs repo
            echo ""
            echo "Committing ${VERSION} Unsigned Sigs"
            echo ""
            pushd gitian.sigs
            git add ${VERSION}-linux/"${SIGNER}"
            git add ${VERSION}-win-unsigned/"${SIGNER}"
            git add ${VERSION}-osx-unsigned/"${SIGNER}"
            git commit -a -m "Add ${VERSION} unsigned sigs for ${SIGNER}"
            popd
        fi
fi

# Verify the build
if [[ $verify = true ]]
then
	# Linux
	pushd ./gitian-builder
	echo ""
	echo "Verifying v${VERSION} Linux"
	echo ""
	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-linux ../bitcoin/contrib/gitian-descriptors/gitian-linux.yml
	# Windows
	echo ""
	echo "Verifying v${VERSION} Windows"
	echo ""
	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-win-unsigned ../bitcoin/contrib/gitian-descriptors/gitian-win.yml
	# Mac OSX	
	echo ""
	echo "Verifying v${VERSION} Mac OSX"
	echo ""	
	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-unsigned ../bitcoin/contrib/gitian-descriptors/gitian-osx.yml
	# Signed Windows
	echo ""
	echo "Verifying v${VERSION} Signed Windows"
	echo ""
	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-signed ../bitcoin/contrib/gitian-descriptors/gitian-osx-signer.yml
	# Signed Mac OSX
	echo ""
	echo "Verifying v${VERSION} Signed Mac OSX"
	echo ""
	./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-signed ../bitcoin/contrib/gitian-descriptors/gitian-osx-signer.yml	
	popd
fi

# Sign binaries
if [[ $sign = true ]]
then
	
        pushd ./gitian-builder
	# Sign Windows
	if [[ $windows = true ]]
	then
	    echo ""
	    echo "Signing ${VERSION} Windows"
	    echo ""
	    ./bin/gbuild -i --commit signature=${COMMIT} ../bitcoin/contrib/gitian-descriptors/gitian-win-signer.yml
	    ./bin/gsign -p "$signProg" --signer "$SIGNER" --release ${VERSION}-win-signed --destination ../gitian.sigs/ ../bitcoin/contrib/gitian-descriptors/gitian-win-signer.yml
	    mv build/out/bitcoin-*win64-setup.exe ../bitcoin-binaries/${VERSION}
	    mv build/out/bitcoin-*win32-setup.exe ../bitcoin-binaries/${VERSION}
	fi
	# Sign Mac OSX
	if [[ $osx = true ]]
	then
	    echo ""
	    echo "Signing ${VERSION} Mac OSX"
	    echo ""
	    ./bin/gbuild -i --commit signature=${COMMIT} ../bitcoin/contrib/gitian-descriptors/gitian-osx-signer.yml
	    ./bin/gsign -p "$signProg" --signer "$SIGNER" --release ${VERSION}-osx-signed --destination ../gitian.sigs/ ../bitcoin/contrib/gitian-descriptors/gitian-osx-signer.yml
	    mv build/out/bitcoin-osx-signed.dmg ../bitcoin-binaries/${VERSION}/bitcoin-${VERSION}-osx.dmg
	fi
	popd

        if [[ $commitFiles = true ]]
        then
            # Commit Sigs
            pushd gitian.sigs
            echo ""
            echo "Committing ${VERSION} Signed Sigs"
            echo ""
            git add ${VERSION}-win-signed/"${SIGNER}"
            git add ${VERSION}-osx-signed/"${SIGNER}"
            git commit -a -m "Add ${VERSION} signed binary sigs for ${SIGNER}"
            popd
        fi
fi
