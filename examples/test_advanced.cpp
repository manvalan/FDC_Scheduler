#include <iostream>

extern "C" {
    void* fdc_scheduler_create();
    void fdc_scheduler_destroy(void* api);
    const char* fdc_scheduler_load_network(void* api, const char* path);
    const char* fdc_scheduler_detect_conflicts(void* api);
    const char* fdc_scheduler_analyze_network(void* api);
    const char* fdc_scheduler_optimize_platforms(void* api);
    const char* fdc_scheduler_get_schedule_metrics(void* api);
}

void print_section(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

int main() {
    std::cout << "🚂 FDC_Scheduler - Test Funzionalità Avanzate\n\n";
    
    void* api = fdc_scheduler_create();
    
    // Carica rete di test
    print_section("Caricamento Rete Test");
    const char* load_result = fdc_scheduler_load_network(api, "../test_network.json");
    std::cout << load_result << "\n";
    
    // FASE 2: Conflict Detection
    print_section("FASE 2: Rilevamento Conflitti");
    const char* conflicts = fdc_scheduler_detect_conflicts(api);
    std::cout << conflicts << "\n";
    
    // FASE 3: Network Analysis
    print_section("FASE 3: Analisi Topologica Rete");
    const char* analysis = fdc_scheduler_analyze_network(api);
    std::cout << analysis << "\n";
    
    // FASE 4: Schedule Optimization
    print_section("FASE 4a: Metriche Orari");
    const char* metrics = fdc_scheduler_get_schedule_metrics(api);
    std::cout << metrics << "\n";
    
    print_section("FASE 4b: Ottimizzazione Binari");
    const char* optimization = fdc_scheduler_optimize_platforms(api);
    std::cout << optimization << "\n";
    
    print_section("FASE 4c: Conflitti Dopo Ottimizzazione");
    const char* conflicts_after = fdc_scheduler_detect_conflicts(api);
    std::cout << conflicts_after << "\n";
    
    fdc_scheduler_destroy(api);
    
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "✅ Test Fasi 2, 3, 4 completato!\n\n";
    
    return 0;
}
