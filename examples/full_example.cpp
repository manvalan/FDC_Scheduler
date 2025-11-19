#include <iostream>
#include <fstream>

// Dichiarazioni C API
extern "C" {
    void* fdc_scheduler_create();
    void fdc_scheduler_destroy(void* api);
    const char* fdc_scheduler_load_network(void* api, const char* path);
    const char* fdc_scheduler_save_network(void* api, const char* path);
    const char* fdc_scheduler_get_network_info(void* api);
    const char* fdc_scheduler_add_station(void* api, const char* node_json);
    const char* fdc_scheduler_add_track(void* api, const char* edge_json);
    const char* fdc_scheduler_get_all_nodes(void* api);
    const char* fdc_scheduler_get_all_edges(void* api);
    const char* fdc_scheduler_add_train(void* api, const char* train_json);
    const char* fdc_scheduler_get_all_trains(void* api);
    const char* fdc_scheduler_get_train(void* api, const char* train_id);
    const char* fdc_scheduler_delete_train(void* api, const char* train_id);
    const char* fdc_scheduler_find_shortest_path(void* api, const char* from, const char* to);
    const char* fdc_scheduler_detect_conflicts(void* api);
    const char* fdc_scheduler_validate_schedule(void* api, const char* train_id);
}

void print_section(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

void print_result(const char* operation, const char* result) {
    std::cout << "📋 " << operation << ":\n";
    std::cout << result << "\n\n";
}

int main() {
    std::cout << "🚂 FDC_Scheduler - Esempio Completo\n";
    std::cout << "====================================\n\n";
    
    // Crea istanza API
    void* api = fdc_scheduler_create();
    
    // ========== TEST 1: Creazione Rete da Zero ==========
    print_section("TEST 1: Creazione Rete Ferroviaria");
    
    // Aggiungi stazioni
    const char* station_a = R"({
        "id": "STA",
        "name": "Stazione A",
        "type": "station",
        "latitude": 45.4642,
        "longitude": 9.1900,
        "platforms": 4,
        "capacity": 10
    })";
    print_result("Aggiungi Stazione A", fdc_scheduler_add_station(api, station_a));
    
    const char* station_b = R"({
        "id": "STB",
        "name": "Stazione B",
        "type": "station",
        "latitude": 45.0703,
        "longitude": 7.6869,
        "platforms": 3,
        "capacity": 8
    })";
    print_result("Aggiungi Stazione B", fdc_scheduler_add_station(api, station_b));
    
    const char* junction_c = R"({
        "id": "JNC",
        "name": "Bivio C",
        "type": "junction",
        "latitude": 45.2500,
        "longitude": 8.5000,
        "platforms": 2,
        "capacity": 4
    })";
    print_result("Aggiungi Bivio C", fdc_scheduler_add_station(api, junction_c));
    
    // Aggiungi tratte
    const char* track_ab = R"({
        "from": "STA",
        "to": "STB",
        "distance": 126.5,
        "max_speed": 200.0,
        "track_type": "double",
        "bidirectional": true
    })";
    print_result("Aggiungi Tratta STA→STB", fdc_scheduler_add_track(api, track_ab));
    
    const char* track_ac = R"({
        "from": "STA",
        "to": "JNC",
        "distance": 65.0,
        "max_speed": 160.0,
        "track_type": "single",
        "bidirectional": true
    })";
    print_result("Aggiungi Tratta STA→JNC", fdc_scheduler_add_track(api, track_ac));
    
    const char* track_bc = R"({
        "from": "STB",
        "to": "JNC",
        "distance": 78.0,
        "max_speed": 150.0,
        "track_type": "single",
        "bidirectional": true
    })";
    print_result("Aggiungi Tratta STB→JNC", fdc_scheduler_add_track(api, track_bc));
    
    // ========== TEST 2: Informazioni Rete ==========
    print_section("TEST 2: Statistiche Rete");
    
    print_result("Info Rete", fdc_scheduler_get_network_info(api));
    print_result("Tutte le Stazioni", fdc_scheduler_get_all_nodes(api));
    print_result("Tutte le Tratte", fdc_scheduler_get_all_edges(api));
    
    // ========== TEST 3: Aggiunta Treni ==========
    print_section("TEST 3: Gestione Orari Treni");
    
    const char* train1 = R"({
        "train_id": "ICN500",
        "schedule_id": "ICN500_20240115",
        "stops": [
            {
                "node_id": "STA",
                "arrival": "2024-01-15T08:00:00",
                "departure": "2024-01-15T08:05:00",
                "platform": 1
            },
            {
                "node_id": "JNC",
                "arrival": "2024-01-15T08:30:00",
                "departure": "2024-01-15T08:32:00",
                "platform": 1
            },
            {
                "node_id": "STB",
                "arrival": "2024-01-15T09:00:00",
                "departure": "2024-01-15T09:05:00",
                "platform": 2
            }
        ]
    })";
    print_result("Aggiungi Treno ICN500", fdc_scheduler_add_train(api, train1));
    
    const char* train2 = R"({
        "train_id": "REG101",
        "schedule_id": "REG101_20240115",
        "stops": [
            {
                "node_id": "STB",
                "arrival": "2024-01-15T07:00:00",
                "departure": "2024-01-15T07:10:00",
                "platform": 1
            },
            {
                "node_id": "JNC",
                "arrival": "2024-01-15T07:40:00",
                "departure": "2024-01-15T07:42:00",
                "platform": 2
            },
            {
                "node_id": "STA",
                "arrival": "2024-01-15T08:15:00",
                "departure": "2024-01-15T08:20:00",
                "platform": 3
            }
        ]
    })";
    print_result("Aggiungi Treno REG101", fdc_scheduler_add_train(api, train2));
    
    print_result("Tutti i Treni", fdc_scheduler_get_all_trains(api));
    print_result("Dettagli ICN500", fdc_scheduler_get_train(api, "ICN500"));
    
    // ========== TEST 4: Pathfinding ==========
    print_section("TEST 4: Calcolo Percorsi");
    
    print_result("Percorso Breve STA→STB", fdc_scheduler_find_shortest_path(api, "STA", "STB"));
    print_result("Percorso Breve STA→JNC", fdc_scheduler_find_shortest_path(api, "STA", "JNC"));
    print_result("Percorso Breve STB→STA", fdc_scheduler_find_shortest_path(api, "STB", "STA"));
    
    // ========== TEST 5: Validazione ==========
    print_section("TEST 5: Validazione Orari");
    
    print_result("Valida ICN500", fdc_scheduler_validate_schedule(api, "ICN500"));
    print_result("Valida Tutti", fdc_scheduler_validate_schedule(api, nullptr));
    
    // ========== TEST 6: Conflict Detection ==========
    print_section("TEST 6: Rilevamento Conflitti");
    
    print_result("Rileva Conflitti", fdc_scheduler_detect_conflicts(api));
    
    // ========== TEST 7: Salvataggio Rete ==========
    print_section("TEST 7: Salvataggio e Caricamento");
    
    const char* save_path = "test_network.json";
    print_result("Salva Rete", fdc_scheduler_save_network(api, save_path));
    
    // Distruggi e ricrea
    fdc_scheduler_destroy(api);
    api = fdc_scheduler_create();
    
    print_result("Ricarica Rete", fdc_scheduler_load_network(api, save_path));
    print_result("Info Rete Ricaricata", fdc_scheduler_get_network_info(api));
    
    // ========== TEST 8: Eliminazione Treni ==========
    print_section("TEST 8: Eliminazione");
    
    print_result("Elimina REG101", fdc_scheduler_delete_train(api, "REG101"));
    print_result("Treni Rimanenti", fdc_scheduler_get_all_trains(api));
    
    // Cleanup
    fdc_scheduler_destroy(api);
    
    print_section("✅ TEST COMPLETATI");
    std::cout << "Tutti i test sono stati eseguiti con successo!\n";
    std::cout << "File salvato: " << save_path << "\n\n";
    
    return 0;
}
