build_modules(${CMAKE_CURRENT_SOURCE_DIR})

install(FILES diceserv.example.conf
  DESTINATION ${CONF_DIR}
)
