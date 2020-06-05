find_program(PYTHON3 "python3")
set(SETUP_PY_IN	${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in)
set(SETUP_PY    ${CMAKE_CURRENT_BINARY_DIR}/setup.py)
set(DEPS        ${CMAKE_CURRENT_SOURCE_DIR}/ubtbr/__init__.py
                ${CMAKE_CURRENT_SOURCE_DIR}/ubtbr/btctl.py
                ${CMAKE_CURRENT_SOURCE_DIR}/ubtbr/lmp.py)
set(OUTPUT      ${CMAKE_CURRENT_BINARY_DIR}/build)

configure_file(${SETUP_PY_IN} ${SETUP_PY})

add_custom_command(OUTPUT ${OUTPUT}/timestamp
                   COMMAND ${PYTHON3} setup.py build
                   COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}/timestamp
                   DEPENDS ${DEPS})

add_custom_target(ubtbr ALL DEPENDS ${OUTPUT}/timestamp)
install(CODE "execute_process(COMMAND ${PYTHON3} ${SETUP_PY} build -b ${OUTPUT} install -f --prefix=${CMAKE_INSTALL_PREFIX})")

add_custom_target(ubertooth-btbr ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth-btbr)
install(PROGRAMS ubertooth-btbr DESTINATION ${INSTALL_DEFAULT_BINDIR})
