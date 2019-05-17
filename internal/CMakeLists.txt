add_library(internal SHARED
  src/main.cpp
  src/engine_types.h
  src/engine_types.cpp
  src/memory/modifiable_code.h
  src/memory/wrapper_address_space.h
  src/memory/executable_address_space.h
  src/server/message_builder.h
  src/server/tcp_server.cpp
  src/server/tcp_server.h
  src/logging/log.cpp
  src/logging/log.h
  src/emitters.cpp
  src/emitters.h
  src/bundles.cpp
  src/bundles.h
)

target_include_directories(internal PRIVATE ${PROJECT_SOURCE_DIR}/dependencies/spdlog/include)
target_link_libraries(internal ws2_32)
target_compile_options(internal PRIVATE /MT)
