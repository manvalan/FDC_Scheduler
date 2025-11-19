cmake_minimum_required(VERSION 3.15)

# Examples
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE fdc_scheduler)

add_executable(json_api_example json_api_example.cpp)
target_link_libraries(json_api_example PRIVATE fdc_scheduler)

add_executable(conflict_detection_example conflict_detection_example.cpp)
target_link_libraries(conflict_detection_example PRIVATE fdc_scheduler)

# Install examples
install(TARGETS 
    basic_example 
    json_api_example 
    conflict_detection_example
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)
