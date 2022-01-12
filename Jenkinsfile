pipeline {
    agent { 
        dockerfile {
            args '--group-add=46 --privileged -v /dev/bus/usb:/dev/bus/usb'
        }
    }
    options {
        throttleJobProperty(
            categories: ['single-build-throttle'],
            throttleEnabled: true,
            throttleOption: 'category'
        )
    }
    environment {
        GIT_COMMITER_NAME = 'CI Person'
        GIT_COMMITER_EMAIL = 'ci@greatscottgadgets.com'
    }
    stages {
        stage('Build (Host)') {
            steps {
                sh '''#!/bin/bash
                    mkdir host/build
                    cd host/build
                    cmake ..
                    make
                    cd ../..'''
            }
        }
        stage('Build (Firmware)') {
            steps {
                sh '''#!/bin/bash
                    cd firmware/bluetooth_rxtx
                    make
                    cd ../..'''
            }
        }
        stage('Test') {
            steps {
                sh '''#!/bin/bash
                    host/build/ubertooth-tools/src/ubertooth-util -b -p -s'''
                sh '''#!/bin/bash
                    usbhub power state --port 4 --reset
                    host/build/ubertooth-tools/src/ubertooth-dfu -d firmware/bluetooth_rxtx/bluetooth_rxtx.dfu -r'''
            }
        }
    }
    post {
        always {
            echo 'One way or another, I have finished'
            sh 'rm -rf testing-venv/'
            // Clean after build
            cleanWs(cleanWhenNotBuilt: false,
                    deleteDirs: true,
                    disableDeferredWipeout: true,
                    notFailBuild: true)
        }
    }
}