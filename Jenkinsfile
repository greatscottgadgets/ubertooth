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
                sh './ci-scripts/build-host.sh'
            }
        }
        stage('Build (Firmware)') {
            steps {
                sh './ci-scripts/build-firmware.sh'
            }
        }
        stage('Test') {
            steps {
                sh './ci-scripts/test-hub.sh'
                retry(3) {
                    sh './ci-scripts/test-firmware.sh'
                }
                retry(3) {
                    sh './ci-scripts/test-host.sh'
                }
            }
        }
    }
    post {
        always {
            sh 'usbhub --hub D9D1 power state --port 4 --reset'
            cleanWs(cleanWhenNotBuilt: false,
                    deleteDirs: true,
                    disableDeferredWipeout: true,
                    notFailBuild: true)
        }
    }
}