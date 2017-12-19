#!/usr/bin/env groovy
pipeline {
    agent none
    stages {
        stage('Build') {
            parallel {
                stage('ubuntu17.04') {
                    agent {
                        docker {
                            image 'repo.t0.com/medici/raven-build:v1.0'
                            label 'linux'
                            args  '-v /Users/Shared/Jenkins/Home/raven_depends/build_artifacts:/root/raven'
                        }
                    }
                    steps {
                        sh '''./autogen.sh
                        ./configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX CXXFLAGS="-fPIC" CPPFLAGS="-fPIC"
                        make -j3

                        mkdir -p /root/raven/build-linux/ubuntu17.04
                        cp src/ravend /root/raven/build-linux/ubuntu17.04/
                        cp src/raven-cli /root/raven/build-linux/ubuntu17.04/
                        cp src/qt/raven-qt /root/raven/build-linux/ubuntu17.04/
                        cd /root/raven/build-linux/ubuntu17.04
                        md5sum ravend raven-cli raven-qt > md5sum
                        sha256sum ravend raven-cli raven-qt > sha256sum
                        tar zcf raven-ubuntu-17.04.${BUILD_NUMBER}.tar.gz ravend raven-cli raven-qt md5sum sha256sum'''
                    }
                }
                stage('ubuntu16.04') {
                    agent {
                        docker {
                            image 'repo.t0.com/medici/raven-build:v0.4'
                            args  '-v /Users/Shared/Jenkins/Home/raven_depends/build_artifacts:/root/raven'
                            label 'linux'
                        }
                    }
                    steps {
                        sh '''
                        ./autogen.sh
                        ./configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX CXXFLAGS="-fPIC" CPPFLAGS="-fPIC"
                        make -j3
                        mkdir -p /root/raven/build-linux/ubuntu16.04
                        cp src/ravend /root/raven/build-linux/ubuntu16.04/
                        cp src/raven-cli /root/raven/build-linux/ubuntu16.04/
                        cp src/qt/raven-qt /root/raven/build-linux/ubuntu16.04/
                        cd /root/raven/build-linux/ubuntu16.04
                        md5sum ravend raven-cli raven-qt > md5sum
                        sha256sum ravend raven-cli raven-qt > sha256sum
                        tar zcf raven-ubuntu-16.04.${BUILD_NUMBER}.tar.gz ravend raven-cli raven-qt md5sum sha256sum'''
                    }
                }
                stage('windows_x64') {
                    agent {
                        docker {
                            image 'repo.t0.com/medici/raven-ubuntu-17.04:v0.2'
                            args  '-v /Users/Shared/Jenkins/Home/raven_depends:/root/depends'
                            label 'linux'
                        }
                    }
                    steps {
                        sh '''
                        export HOME="/root/raven-test"
                        mkdir /root/raven-test/.ccache/tmp
                        chmod -Rf 777 /root/raven-test/.ccache
                        rsync -av /root/depends/ $PWD/depends/
                        PATH=$(echo "$PATH" | sed -e "s/:\\/mnt.*//g")

                        ./autogen.sh
                        CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/

                        make -j3

                        mkdir /root/depends/build_artifacts/build-windows
                        make install DESTDIR=/root/depends/build_artifacts/build-windows
                        '''
                    }
                }
                stage('OS_X') {
                    agent {
                        label 'mac'
                    }
                    steps {
                        sh '''
                        ./autogen.sh
                        ./configure
                        make -j3
                        make deploy

                        mkdir /Users/Shared/Jenkins/Home/raven_depends/build_artifacts/build-osx
                        cp -Rf Raven-Core.dmg /Users/Shared/Jenkins/Home/raven_depends/build_artifacts/build-osx/
                        cp src/ravend /Users/Shared/Jenkins/Home/raven_depends/build_artifacts/build-osx/
                        cp src/raven-cli /Users/Shared/Jenkins/Home/raven_depends/build_artifacts/build-osx/
                        cd /Users/Shared/Jenkins/Home/raven_depends/build_artifacts/build-osx
                        md5 ravend raven-cli Raven-Core.dmg > md5sum
                        shasum -a 256 ravend raven-cli Raven-Core.dmg > sha256sum'''
                    }
                }
            }
        }
    }
}
