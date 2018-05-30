#!/usr/bin/env groovy
pipeline {
    agent none
    stages {
        stage('Build') {
            parallel {
                stage('ubuntu17.04') {
                    awsCodeBuild credentialsType: 'keys',
                                 awsAccessKey: 'AKIAI3V66NTSVTDPDKOA',
                                 awsSecretKey: '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',
                                 sourceControlType: 'jenkins',
                                 envParameters: '',
                                 envVariables: '[ { BRANCH, $BRANCH } ]', 
                                 projectName: 'raven-ubuntu-17_04',
                                 region: 'us-west-2'
                }
                stage('ubuntu16.04') {
                    awsCodeBuild credentialsType: 'keys',
                                 awsAccessKey: 'AKIAI3V66NTSVTDPDKOA',
                                 awsSecretKey: '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',
                                 sourceControlType: 'jenkins',
                                 envParameters: '',
                                 envVariables: '[ { BRANCH, $BRANCH } ]', 
                                 projectName: 'raven-ubuntu-16_04',
                                 region: 'us-west-2'
                }
                stage('centos7') {
                    awsCodeBuild credentialsType: 'keys',
                                 awsAccessKey: 'AKIAI3V66NTSVTDPDKOA',
                                 awsSecretKey: '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',
                                 sourceControlType: 'jenkins',
                                 envParameters: '',
                                 envVariables: '[ { BRANCH, $BRANCH } ]', 
                                 projectName: 'raven-centos-7',
                                 region: 'us-west-2'
                }
                stage('fedora') {
                    awsCodeBuild credentialsType: 'keys',
                                 awsAccessKey: 'AKIAI3V66NTSVTDPDKOA',
                                 awsSecretKey: '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',
                                 sourceControlType: 'jenkins',
                                 envParameters: '',
                                 envVariables: '[ { BRANCH, $BRANCH } ]', 
                                 projectName: 'raven-fedora',
                                 region: 'us-west-2'
                }
                stage('windows_x64') {
                    awsCodeBuild credentialsType: 'keys',
                                 awsAccessKey: 'AKIAI3V66NTSVTDPDKOA',
                                 awsSecretKey: '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',
                                 sourceControlType: 'jenkins',
                                 envParameters: '',
                                 envVariables: '[ { BRANCH, $BRANCH } ]', 
                                 projectName: 'raven-windows-build',
                                 region: 'us-west-2'
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
