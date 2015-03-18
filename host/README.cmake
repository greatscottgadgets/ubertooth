CMake Settings
==============
The following are flags that may be of use when configuing this project.

 * INSTALL_UDEV_RULES
  * A boolean flag for installing udev rules to control access to
    Ubertooth hardware.

 * UDEV_RULES_GROUP
  * A variable to specify group name to be used in the udev rules,
    if not set CMake will attempt to determine in usb or plugdev
    are used on the host system.