#include <iostream>

extern "C" {
    void* fdc_scheduler_create();
    void fdc_scheduler_destroy(void* api);
    const char* fdc_scheduler_add_station(void* api, const char* node_json);
    const char* fdc_scheduler_add_track(void* api, const char* edge_json);
    const char* fdc_scheduler_find_shortest_path(void* api, const char* from, const char* to);
    const char* fdc_scheduler_find_k_shortest_paths(void* api, const char* from, const char* to, int k, int use_distance);
}

void print_section(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

int main() {
    std::cout << "🚂 FDC_Scheduler - Test K-Shortest Paths\n\n";
    
    void* api = fdc_scheduler_create();
    
    // Crea rete con percorsi alternativi
    print_section("Creazione Rete con Percorsi Multipli");
    
    // Nodi: A --- B --- C
    //        \   /     /
    //         \ /     /
    //          D --- E
    
    const char* nodes[] = {
        R"({"id":"A","name":"Nodo A","type":"station","platforms":2})",
        R"({"id":"B","name":"Nodo B","type":"station","platforms":2})",
        R"({"id":"C","name":"Nodo C","type":"station","platforms":2})",
        R"({"id":"D","name":"Nodo D","type":"junction","platforms":1})",
        R"({"id":"E","name":"Nodo E","type":"junction","platforms":1})"
    };
    
    for (const char* node : nodes) {
        fdc_scheduler_add_station(api, node);
    }
    std::cout << "✓ Aggiunti 5 nodi\n";
    
    // Percorsi possibili da A a C:
    // 1) A → B → C (diretto, veloce ma lungo)
    // 2) A → D → E → C (via bivio, più lento)
    // 3) A → D → B → C (misto)
    
    const char* edges[] = {
        R"({"from":"A","to":"B","distance":100,"max_speed":200,"track_type":"double"})",
        R"({"from":"B","to":"C","distance":80,"max_speed":200,"track_type":"double"})",
        R"({"from":"A","to":"D","distance":60,"max_speed":120,"track_type":"single"})",
        R"({"from":"D","to":"B","distance":70,"max_speed":120,"track_type":"single"})",
        R"({"from":"D","to":"E","distance":50,"max_speed":100,"track_type":"single"})",
        R"({"from":"E","to":"C","distance":90,"max_speed":100,"track_type":"single"})"
    };
    
    for (const char* edge : edges) {
        fdc_scheduler_add_track(api, edge);
    }
    std::cout << "✓ Aggiunte 6 tratte\n\n";
    
    // Test 1: Percorso singolo (shortest)
    print_section("TEST 1: Percorso Più Breve (Singolo)");
    std::cout << fdc_scheduler_find_shortest_path(api, "A", "C") << "\n\n";
    
    // Test 2: 3 percorsi alternativi per distanza
    print_section("TEST 2: 3 Percorsi Alternativi (Distanza)");
    std::cout << fdc_scheduler_find_k_shortest_paths(api, "A", "C", 3, 1) << "\n\n";
    
    // Test 3: 5 percorsi (se esistono)
    print_section("TEST 3: Ricerca 5 Percorsi (Tempo)");
    std::cout << fdc_scheduler_find_k_shortest_paths(api, "A", "C", 5, 0) << "\n\n";
    
    // Test 4: Percorso impossibile
    print_section("TEST 4: Percorso Inesistente");
    const char* isolato = R"({"id":"Z","name":"Isolato","type":"station","platforms":1})";
    fdc_scheduler_add_station(api, isolato);
    std::cout << fdc_scheduler_find_k_shortest_paths(api, "A", "Z", 3, 1) << "\n\n";
    
    // Test 5: Parametro k non valido
    print_section("TEST 5: Parametro k Non Valido");
    std::cout << fdc_scheduler_find_k_shortest_paths(api, "A", "C", 15, 1) << "\n\n";
    
    fdc_scheduler_destroy(api);
    
    std::cout << std::string(70, '=') << "\n";
    std::cout << "✅ Test K-Shortest Paths completato!\n\n";
    
    return 0;
}
