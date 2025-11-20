# Python bindings for FDC_Scheduler
pybind11_add_module(pyfdc_scheduler 
    bindings.cpp
)

target_link_libraries(pyfdc_scheduler PRIVATE fdc_scheduler)

# Set output directory
set_target_properties(pyfdc_scheduler PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python
)

# Install Python module
install(TARGETS pyfdc_scheduler
    LIBRARY DESTINATION python
)
