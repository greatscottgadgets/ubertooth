cmake_minimum_required(VERSION 2.8)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(SYSTEM_IS_LINUX TRUE)
    SET(UDEV_OPTION_DEFAULT ON)
else()
    SET(SYSTEM_IS_LINUX FALSE)
    SET(UDEV_OPTION_DEFAULT OFF)
endif()

option(INSTALL_UDEV_RULES
    "Install udev rules for the Ubertooth"
    ${UDEV_OPTION_DEFAULT}
)

set(UDEV_RULES_PATH
    "/etc/udev/rules.d"
    CACHE STRING
    "Target directory for udev rule installation. Ensure you have permissions to write to this directory."
)

set(UBERTOOTH_GROUP "usb" CACHE STRING "Group to associate Ubertooth devices with in udev rules")

if(SYSTEM_IS_LINUX)
    if(INSTALL_UDEV_RULES)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/40-ubertooth.rules.in
                       ${CMAKE_CURRENT_BINARY_DIR}/40-ubertooth.rules
                       @ONLY
                      )

        message(STATUS  "Ubertooth udev rules will be installed to '${UDEV_RULES_PATH}' upon running 'make install'")
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/40-ubertooth.rules
                DESTINATION ${UDEV_RULES_PATH}
                COMPONENT "udev_rules")
    else()
        message(STATUS
                "Ubertooth udev rules will not be installed because INSTALL_UDEV_RULES=OFF"
               )
    endif(INSTALL_UDEV_RULES)
else()
    if(INSTALL_UDEV_RULES)
        message(STATUS "udev rules not supported on this platform. Hide this message via -DINSTALL_UDEV_RULES=Off")
    endif(INSTALL_UDEV_RULES)
else()
endif()
