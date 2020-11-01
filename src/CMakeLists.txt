add_subdirectory(networking)

add_executable(server server.cpp)
target_link_libraries(server networking)

add_executable(client client.cpp)
target_link_libraries(client networking)
