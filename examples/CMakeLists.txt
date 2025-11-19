# Simple example using FDC_Scheduler wrapper
add_executable(simple_example simple_example.cpp)
add_executable(full_example full_example.cpp)
target_link_libraries(simple_example PRIVATE fdc_scheduler)
target_link_libraries(full_example PRIVATE fdc_scheduler)
