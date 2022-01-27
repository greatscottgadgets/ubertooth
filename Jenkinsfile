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
                sh './ci-scripts/configure-hubs.sh --off'
                retry(3) {
                    sh './ci-scripts/test.sh'
                }
            }
        }
    }
    post {
        always {
            sh './ci-scripts/configure-hubs.sh --reset'
            cleanWs(cleanWhenNotBuilt: false,
                    deleteDirs: true,
                    disableDeferredWipeout: true,
                    notFailBuild: true)
        }
    }
}