#include <iostream>

// C API forward declarations
extern "C" {
    void* fdc_scheduler_create();
    void fdc_scheduler_destroy(void* api);
    const char* fdc_scheduler_get_network_info(void* api);
}

int main() {
    std::cout << "FDC_Scheduler - Simple Example\n";
    std::cout << "================================\n\n";
    
    // Create API instance
    void* api = fdc_scheduler_create();
    
    // Get network info
    const char* info = fdc_scheduler_get_network_info(api);
    std::cout << "Network info:\n" << info << "\n\n";
    
    // Cleanup
    fdc_scheduler_destroy(api);
    
    std::cout << "✓ Example completed successfully\n";
    return 0;
}
