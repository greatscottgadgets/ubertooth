pipeline {
    agent {
        dockerfile {
            args '--group-add=46 --device-cgroup-rule="c 189:* rmw" -v /dev/bus/usb:/dev/bus/usb'
        }
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
                sh './ci-scripts/configure-hubs.sh --reset'
            }
        }
    }
    post {
        always {
            cleanWs(cleanWhenNotBuilt: false,
                    deleteDirs: true,
                    disableDeferredWipeout: true,
                    notFailBuild: true)
        }
    }
}
