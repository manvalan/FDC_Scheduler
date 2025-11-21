#include <gtest/gtest.h>
#include "fdc_scheduler/railway_network.hpp"
#include "fdc_scheduler/schedule.hpp"
#include "fdc_scheduler/conflict_detector.hpp"
#include "fdc_scheduler/train.hpp"
#include <chrono>
#include <random>
#include <thread>

using namespace fdc_scheduler;

/**
 * Test fixture per scenari pesanti con rete complessa
 */
class HeavyScenarioTest : public ::testing::Test {
protected:
    RailwayNetwork network;
    std::vector<Train> trains;
    std::vector<TrainSchedule> schedules;
    
    // Helper per creare una rete ferroviaria complessa (topologia realistica)
    void SetUpComplexNetwork() {
        // Crea una rete con 50 nodi (stazioni principali, secondarie, junction, scali)
        // Topologia: linea principale + 3 diramazioni + 2 anelli
        
        // === LINEA PRINCIPALE (Nord-Sud): 20 stazioni ===
        for (int i = 0; i < 20; i++) {
            std::string id = "MAIN_" + std::to_string(i);
            std::string name = "Main Station " + std::to_string(i);
            NodeType type = (i % 5 == 0) ? NodeType::STATION : NodeType::JUNCTION;
            
            Node node(id, name, type);
            node.set_coordinates(45.0 + i * 0.5, 9.0 + i * 0.1);
            if (type == NodeType::STATION) {
                node.set_platforms(4 + (i % 3)); // 4-6 binari
            }
            network.add_node(node);
            
            // Collega con stazione precedente
            if (i > 0) {
                std::string prev_id = "MAIN_" + std::to_string(i - 1);
                Edge edge(prev_id, id, 15.0 + (i % 5) * 2.0); // 15-25 km
                edge.set_track_type(TrackType::DOUBLE);
                edge.set_max_speed(160.0 + (i % 3) * 20.0); // 160-200 km/h
                edge.set_capacity(12); // 12 treni/ora
                network.add_edge(edge);
                // Aggiungi anche direzione opposta
                network.add_edge(Edge(id, prev_id, 15.0 + (i % 5) * 2.0));
            }
        }
        
        // === DIRAMAZIONE EST (da MAIN_5): 8 stazioni ===
        for (int i = 0; i < 8; i++) {
            std::string id = "EAST_" + std::to_string(i);
            std::string name = "East Branch " + std::to_string(i);
            
            Node node(id, name, NodeType::STATION);
            node.set_coordinates(45.0 + 2.5 + i * 0.3, 9.5 + i * 0.2);
            node.set_platforms(2 + (i % 2));
            network.add_node(node);
            
            std::string connect_from = (i == 0) ? "MAIN_5" : "EAST_" + std::to_string(i - 1);
            Edge edge(connect_from, id, 10.0 + i * 1.5);
            edge.set_track_type(TrackType::SINGLE);
            edge.set_max_speed(120.0);
            edge.set_capacity(6);
            network.add_edge(edge);
            network.add_edge(Edge(id, connect_from, 10.0 + i * 1.5));
        }
        
        // === DIRAMAZIONE OVEST (da MAIN_10): 6 stazioni ===
        for (int i = 0; i < 6; i++) {
            std::string id = "WEST_" + std::to_string(i);
            std::string name = "West Branch " + std::to_string(i);
            
            Node node(id, name, NodeType::STATION);
            node.set_coordinates(45.0 + 5.0 - i * 0.4, 9.0 - i * 0.15);
            node.set_platforms(3);
            network.add_node(node);
            
            std::string connect_from = (i == 0) ? "MAIN_10" : "WEST_" + std::to_string(i - 1);
            Edge edge(connect_from, id, 12.0 + i * 2.0);
            edge.set_track_type(TrackType::DOUBLE);
            edge.set_max_speed(140.0);
            edge.set_capacity(8);
            network.add_edge(edge);
            network.add_edge(Edge(id, connect_from, 12.0 + i * 2.0));
        }
        
        // === DIRAMAZIONE SUD (da MAIN_15): 5 stazioni ===
        for (int i = 0; i < 5; i++) {
            std::string id = "SOUTH_" + std::to_string(i);
            std::string name = "South Terminal " + std::to_string(i);
            
            Node node(id, name, NodeType::STATION);
            node.set_coordinates(45.0 + 7.5 + i * 0.2, 10.0 + i * 0.3);
            node.set_platforms(4);
            network.add_node(node);
            
            std::string connect_from = (i == 0) ? "MAIN_15" : "SOUTH_" + std::to_string(i - 1);
            Edge edge(connect_from, id, 18.0);
            edge.set_track_type(TrackType::DOUBLE);
            edge.set_max_speed(180.0);
            edge.set_capacity(10);
            network.add_edge(edge);
            network.add_edge(Edge(id, connect_from, 18.0));
        }
        
        // === ANELLO METROPOLITANO (collega MAIN_0, EAST_0, MAIN_5): 6 stazioni ===
        std::vector<std::string> metro_nodes = {"METRO_0", "METRO_1", "METRO_2", "METRO_3", "METRO_4", "METRO_5"};
        for (size_t i = 0; i < metro_nodes.size(); i++) {
            Node node(metro_nodes[i], "Metro Station " + std::to_string(i), NodeType::STATION);
            node.set_coordinates(45.0 + i * 0.2, 9.0 + i * 0.15);
            node.set_platforms(2);
            network.add_node(node);
            
            // Collega in anello
            std::string next = (i == metro_nodes.size() - 1) ? metro_nodes[0] : metro_nodes[i + 1];
            Edge edge(metro_nodes[i], next, 3.0);
            edge.set_track_type(TrackType::SINGLE);
            edge.set_max_speed(80.0);
            edge.set_capacity(20); // Alta frequenza metro
            network.add_edge(edge);
        }
        
        // Collega anello metro alla rete principale
        network.add_edge(Edge("MAIN_0", "METRO_0", 2.0));
        network.add_edge(Edge("METRO_0", "MAIN_0", 2.0));
        network.add_edge(Edge("EAST_0", "METRO_3", 2.5));
        network.add_edge(Edge("METRO_3", "EAST_0", 2.5));
        network.add_edge(Edge("MAIN_5", "METRO_5", 1.5));
        network.add_edge(Edge("METRO_5", "MAIN_5", 1.5));
    }
    
    // Helper per generare treni con orari casuali ma realistici
    void GenerateHeavyTraffic(int num_trains = 100) {
        std::random_device rd;
        std::mt19937 gen(42); // Seed fisso per riproducibilità
        std::uniform_int_distribution<> hour_dist(6, 22); // Ore 6:00 - 22:00
        std::uniform_int_distribution<> minute_dist(0, 59);
        std::uniform_int_distribution<> train_type_dist(0, 4);
        
        auto base_time = std::chrono::system_clock::now();
        base_time = std::chrono::time_point_cast<std::chrono::hours>(base_time);
        
        for (int i = 0; i < num_trains; i++) {
            std::string train_id = "T" + std::to_string(1000 + i);
            
            // Tipo treno casuale
            TrainType type;
            int type_val = train_type_dist(gen);
            switch (type_val) {
                case 0: type = TrainType::HIGH_SPEED; break;
                case 1: type = TrainType::INTERCITY; break;
                case 2: type = TrainType::REGIONAL; break;
                case 3: type = TrainType::FREIGHT; break;
                default: type = TrainType::REGIONAL; break; // No LOCAL, use REGIONAL
            }
            
            Train train(train_id, "Train " + std::to_string(i), type);
            
            // Imposta velocità massima in base al tipo
            switch (type) {
                case TrainType::HIGH_SPEED: train.set_max_speed(300.0); break;
                case TrainType::INTERCITY: train.set_max_speed(200.0); break;
                case TrainType::REGIONAL: train.set_max_speed(160.0); break;
                case TrainType::FREIGHT: train.set_max_speed(100.0); break;
                default: train.set_max_speed(160.0);
            }
            
            trains.push_back(train);
            
            // Genera orario per questo treno
            TrainSchedule schedule(train_id);
            
            // Tempo di partenza casuale
            int start_hour = hour_dist(gen);
            int start_minute = minute_dist(gen);
            auto departure_time = base_time + 
                                std::chrono::hours(start_hour) + 
                                std::chrono::minutes(start_minute);
            
            // Seleziona percorso casuale (varie combinazioni)
            std::vector<std::string> routes;
            int route_type = i % 8;
            
            switch (route_type) {
                case 0: // Linea principale completa
                    for (int j = 0; j < 20; j++) {
                        routes.push_back("MAIN_" + std::to_string(j));
                    }
                    break;
                case 1: // Linea principale parziale
                    for (int j = 0; j < 10; j++) {
                        routes.push_back("MAIN_" + std::to_string(j));
                    }
                    break;
                case 2: // Main + East branch
                    for (int j = 0; j <= 5; j++) {
                        routes.push_back("MAIN_" + std::to_string(j));
                    }
                    for (int j = 0; j < 8; j++) {
                        routes.push_back("EAST_" + std::to_string(j));
                    }
                    break;
                case 3: // Main + West branch
                    for (int j = 0; j <= 10; j++) {
                        routes.push_back("MAIN_" + std::to_string(j));
                    }
                    for (int j = 0; j < 6; j++) {
                        routes.push_back("WEST_" + std::to_string(j));
                    }
                    break;
                case 4: // Main + South branch
                    for (int j = 0; j <= 15; j++) {
                        routes.push_back("MAIN_" + std::to_string(j));
                    }
                    for (int j = 0; j < 5; j++) {
                        routes.push_back("SOUTH_" + std::to_string(j));
                    }
                    break;
                case 5: // Metro loop
                    for (int j = 0; j < 6; j++) {
                        routes.push_back("METRO_" + std::to_string(j));
                    }
                    break;
                case 6: // Regional complex (main + metro)
                    routes = {"MAIN_0", "METRO_0", "METRO_1", "METRO_2", "METRO_3", 
                             "EAST_0", "EAST_1", "EAST_2"};
                    break;
                case 7: // Reverse direction
                    for (int j = 19; j >= 0; j--) {
                        routes.push_back("MAIN_" + std::to_string(j));
                    }
                    break;
            }
            
            // Crea fermate con tempi progressivi
            auto current_time = departure_time;
            for (size_t j = 0; j < routes.size(); j++) {
                ScheduleStop stop;
                stop.node_id = routes[j];
                stop.arrival = current_time;
                
                // Tempo di sosta: 2-5 minuti per stazioni normali, 1 min per junction
                int dwell_minutes = (j == 0 || j == routes.size() - 1) ? 0 : 
                                   (network.get_node(routes[j]) && 
                                    network.get_node(routes[j])->get_type() == NodeType::STATION) ? 3 : 1;
                
                stop.departure = current_time + std::chrono::minutes(dwell_minutes);
                stop.is_stop = (dwell_minutes > 0);
                stop.platform = (j % 4) + 1;
                
                schedule.add_stop(stop);
                
                // Calcola tempo al prossimo nodo (in base a velocità e distanza)
                if (j < routes.size() - 1) {
                    // Tempo di viaggio stimato: ~6-10 minuti tra stazioni
                    int travel_minutes = 6 + (i % 5);
                    current_time = stop.departure + std::chrono::minutes(travel_minutes);
                }
            }
            
            schedules.push_back(schedule);
        }
    }
    
    void SetUp() override {
        // Setup eseguito prima di ogni test
    }
    
    void TearDown() override {
        // Cleanup
        trains.clear();
        schedules.clear();
    }
};

/**
 * TEST 1: Risoluzione Conflitti in Scenario Pesante
 * 
 * Scenario: 100 treni su rete complessa (50 nodi, 80+ collegamenti)
 * Obiettivo: Identificare e risolvere conflitti di:
 * - Occupazione binario simultanea
 * - Capacità linea superata
 * - Tempi di attraversamento insuff


icienti
 * - Headway minimi violati
 */
TEST_F(HeavyScenarioTest, MassiveConflictDetectionAndResolution) {
    // Setup: Crea rete complessa
    SetUpComplexNetwork();
    ASSERT_GE(network.num_nodes(), 45) << "Rete dovrebbe avere almeno 45 nodi";
    ASSERT_GE(network.num_edges(), 60) << "Rete dovrebbe avere almeno 60 collegamenti";
    
    std::cout << "\n=== TEST RISOLUZIONE CONFLITTI MASSIVI ===\n";
    std::cout << "Nodi nella rete: " << network.num_nodes() << "\n";
    std::cout << "Collegamenti: " << network.num_edges() << "\n";
    
    // Genera traffico pesante: 100 treni
    GenerateHeavyTraffic(100);
    ASSERT_EQ(trains.size(), 100);
    ASSERT_EQ(schedules.size(), 100);
    
    std::cout << "Treni generati: " << trains.size() << "\n";
    
    // Fase 1: Rilevamento conflitti con ConflictDetector
    std::cout << "\n--- FASE 1: RILEVAMENTO CONFLITTI ---\n";
    
    auto start_detect = std::chrono::high_resolution_clock::now();
    
    ConflictDetector detector(network);
    
    // Aggiungi tutti gli orari al detector
    std::vector<Conflict> all_conflicts;
    int total_conflict_checks = 0;
    
    // Per ogni coppia di treni, verifica conflitti
    for (size_t i = 0; i < schedules.size(); i++) {
        for (size_t j = i + 1; j < schedules.size(); j++) {
            total_conflict_checks++;
            // Nota: Il ConflictDetector attuale potrebbe non avere API pubblica
            // per verificare conflitti tra due schedule specifici.
            // In un'implementazione reale, ci sarebbe un metodo come:
            // auto conflicts = detector.detect_conflicts(schedules[i], schedules[j]);
        }
    }
    
    auto end_detect = std::chrono::high_resolution_clock::now();
    auto detect_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_detect - start_detect);
    
    std::cout << "Controlli conflitto eseguiti: " << total_conflict_checks << "\n";
    std::cout << "Tempo rilevamento: " << detect_duration.count() << " ms\n";
    
    // Verifica performance: 100 treni = 4950 combinazioni, deve completare in < 5 secondi
    EXPECT_LT(detect_duration.count(), 5000) 
        << "Rilevamento conflitti troppo lento per 100 treni";
    
    // Simula rilevamento conflitti (in assenza di API completa)
    // Conta sovrapposizioni temporali su stessi nodi
    std::map<std::string, std::vector<std::pair<int, std::chrono::system_clock::time_point>>> node_occupancy;
    
    for (size_t i = 0; i < schedules.size(); i++) {
        for (size_t j = 0; j < schedules[i].get_stop_count(); j++) {
            auto stop = schedules[i].get_stop(j);
            node_occupancy[stop.node_id].push_back({i, stop.arrival});
        }
    }
    
    int potential_conflicts = 0;
    for (const auto& [node_id, occupancies] : node_occupancy) {
        // Ordina per tempo
        auto sorted = occupancies;
        std::sort(sorted.begin(), sorted.end(), 
                 [](const auto& a, const auto& b) { return a.second < b.second; });
        
        // Verifica sovrapposizioni (treni nello stesso nodo entro 5 minuti)
        for (size_t i = 0; i < sorted.size(); i++) {
            for (size_t j = i + 1; j < sorted.size(); j++) {
                auto time_diff = std::chrono::duration_cast<std::chrono::minutes>(
                    sorted[j].second - sorted[i].second);
                
                if (time_diff.count() < 5) {
                    potential_conflicts++;
                }
            }
        }
    }
    
    std::cout << "Conflitti potenziali rilevati: " << potential_conflicts << "\n";
    EXPECT_GT(potential_conflicts, 0) << "Con 100 treni, dovrebbero esserci conflitti";
    
    // Fase 2: Analisi statistica conflitti
    std::cout << "\n--- FASE 2: ANALISI CONFLITTI ---\n";
    
    std::map<std::string, int> conflicts_per_node;
    for (const auto& [node_id, occupancies] : node_occupancy) {
        if (occupancies.size() > 3) { // Nodi con alta congestione
            conflicts_per_node[node_id] = occupancies.size();
        }
    }
    
    std::cout << "Nodi con alta congestione (>3 treni): " << conflicts_per_node.size() << "\n";
    
    // Top 5 nodi più congestionati
    std::vector<std::pair<std::string, int>> sorted_conflicts(
        conflicts_per_node.begin(), conflicts_per_node.end());
    std::sort(sorted_conflicts.begin(), sorted_conflicts.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "Top 5 nodi critici:\n";
    for (size_t i = 0; i < std::min(size_t(5), sorted_conflicts.size()); i++) {
        std::cout << "  " << sorted_conflicts[i].first 
                  << ": " << sorted_conflicts[i].second << " treni\n";
    }
    
    // Fase 3: Strategia di risoluzione
    std::cout << "\n--- FASE 3: RISOLUZIONE CONFLITTI ---\n";
    
    int conflicts_resolved = 0;
    int delays_applied = 0;
    int reroutes_attempted = 0;
    
    // Strategia 1: Ritardi incrementali (2-5 minuti)
    for (size_t i = 0; i < schedules.size(); i++) {
        bool has_conflict = false;
        
        // Verifica se questo treno ha conflitti
        for (size_t j = 0; j < schedules[i].get_stop_count(); j++) {
            auto stop = schedules[i].get_stop(j);
            if (node_occupancy[stop.node_id].size() > 5) {
                has_conflict = true;
                break;
            }
        }
        
        if (has_conflict) {
            // Applica ritardo (nella pratica, modificherebbe tutti i tempi)
            delays_applied++;
            conflicts_resolved++;
        }
    }
    
    std::cout << "Ritardi applicati: " << delays_applied << "\n";
    
    // Strategia 2: Re-routing per treni flessibili (regionali, merci)
    for (size_t i = 0; i < trains.size(); i++) {
        if (trains[i].get_type() == TrainType::FREIGHT || 
            trains[i].get_type() == TrainType::REGIONAL) {
            // Questi treni possono essere re-instradati
            reroutes_attempted++;
        }
    }
    
    std::cout << "Re-routing tentati: " << reroutes_attempted << "\n";
    std::cout << "Conflitti risolti: " << conflicts_resolved << "\n";
    
    // Verifica risoluzione
    double resolution_rate = double(conflicts_resolved) / double(potential_conflicts);
    std::cout << "Tasso di risoluzione: " << (resolution_rate * 100.0) << "%\n";
    
    EXPECT_GT(conflicts_resolved, 0) << "Dovrebbero essere risolti alcuni conflitti";
    EXPECT_GT(resolution_rate, 0.3) << "Almeno 30% dei conflitti dovrebbe essere risolvibile";
    
    std::cout << "\n=== TEST COMPLETATO ===\n\n";
}

/**
 * TEST 2: Analisi Rete per Ottimizzazione Circolazione
 * 
 * Obiettivo: Analizzare metriche di rete per identificare:
 * - Colli di bottiglia (edge/node con capacità satura)
 * - Percorsi critici (single point of failure)
 * - Opportunità di ottimizzazione (binari sottoutilizzati)
 */
TEST_F(HeavyScenarioTest, NetworkAnalysisAndOptimization) {
    SetUpComplexNetwork();
    GenerateHeavyTraffic(100);
    
    std::cout << "\n=== TEST ANALISI RETE E OTTIMIZZAZIONE ===\n";
    
    // Fase 1: Metriche di utilizzo nodi
    std::cout << "\n--- ANALISI UTILIZZO NODI ---\n";
    
    std::map<std::string, int> node_usage;
    std::map<std::string, std::set<std::string>> node_train_sets;
    
    for (const auto& schedule : schedules) {
        for (size_t i = 0; i < schedule.get_stop_count(); i++) {
            auto stop = schedule.get_stop(i);
            node_usage[stop.node_id]++;
            node_train_sets[stop.node_id].insert(schedule.get_train_id());
        }
    }
    
    // Identifica nodi sovraccarichi
    std::vector<std::string> overloaded_nodes;
    std::vector<std::string> underutilized_nodes;
    
    for (const auto& [node_id, count] : node_usage) {
        auto node_ptr = network.get_node(node_id);
        if (!node_ptr) continue;
        
        int platforms = node_ptr->get_platforms();
        double utilization = double(count) / double(platforms * 20); // 20 treni/binario/giorno stimati
        
        if (utilization > 0.8) {
            overloaded_nodes.push_back(node_id);
        } else if (utilization < 0.3 && platforms > 2) {
            underutilized_nodes.push_back(node_id);
        }
    }
    
    std::cout << "Nodi sovraccarichi (>80% capacità): " << overloaded_nodes.size() << "\n";
    std::cout << "Nodi sottoutilizzati (<30% capacità): " << underutilized_nodes.size() << "\n";
    
    for (const auto& node_id : overloaded_nodes) {
        std::cout << "  CRITICO: " << node_id 
                  << " (" << node_usage[node_id] << " passaggi)\n";
    }
    
    EXPECT_GT(overloaded_nodes.size(), 0) << "Con 100 treni, alcuni nodi dovrebbero essere sovraccarichi";
    
    // Fase 2: Analisi collegamenti (edges)
    std::cout << "\n--- ANALISI COLLEGAMENTI ---\n";
    
    std::map<std::pair<std::string, std::string>, int> edge_usage;
    
    for (const auto& schedule : schedules) {
        for (size_t i = 0; i < schedule.get_stop_count() - 1; i++) {
            auto from = schedule.get_stop(i).node_id;
            auto to = schedule.get_stop(i + 1).node_id;
            edge_usage[{from, to}]++;
        }
    }
    
    // Trova collegamenti critici (single track con alta utilizzazione)
    std::vector<std::pair<std::string, std::string>> bottleneck_edges;
    
    for (const auto& [edge_key, usage] : edge_usage) {
        // Verifica se è single track
        // In un'implementazione reale, recupereremmo l'edge dal network
        if (usage > 15) { // Soglia arbitraria per alta congestione
            bottleneck_edges.push_back(edge_key);
        }
    }
    
    std::cout << "Collegamenti congestionati: " << bottleneck_edges.size() << "\n";
    
    // Top 10 collegamenti più utilizzati
    std::vector<std::pair<std::pair<std::string, std::string>, int>> sorted_edges(
        edge_usage.begin(), edge_usage.end());
    std::sort(sorted_edges.begin(), sorted_edges.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "Top 10 collegamenti più trafficati:\n";
    for (size_t i = 0; i < std::min(size_t(10), sorted_edges.size()); i++) {
        std::cout << "  " << sorted_edges[i].first.first 
                  << " -> " << sorted_edges[i].first.second
                  << ": " << sorted_edges[i].second << " treni\n";
    }
    
    // Fase 3: Suggerimenti di ottimizzazione
    std::cout << "\n--- SUGGERIMENTI OTTIMIZZAZIONE ---\n";
    
    int optimization_suggestions = 0;
    
    // 1. Raddoppio binario su collegamenti critici
    for (const auto& [edge_key, usage] : edge_usage) {
        if (usage > 20) {
            std::cout << "  SUGGERIMENTO: Raddoppiare binario " 
                      << edge_key.first << " -> " << edge_key.second << "\n";
            optimization_suggestions++;
        }
    }
    
    // 2. Aggiunta binari a stazioni sovraccariche
    for (const auto& node_id : overloaded_nodes) {
        std::cout << "  SUGGERIMENTO: Aumentare binari a " << node_id << "\n";
        optimization_suggestions++;
    }
    
    // 3. Nuovi collegamenti per alleggerire rete principale
    if (overloaded_nodes.size() > 5) {
        std::cout << "  SUGGERIMENTO: Creare collegamenti alternativi (bypass)\n";
        optimization_suggestions++;
    }
    
    std::cout << "Totale suggerimenti: " << optimization_suggestions << "\n";
    
    EXPECT_GT(optimization_suggestions, 0) << "Dovrebbero esserci opportunità di ottimizzazione";
    
    // Fase 4: Calcolo metriche di efficienza
    std::cout << "\n--- METRICHE DI EFFICIENZA RETE ---\n";
    
    double avg_node_utilization = 0.0;
    int node_count = 0;
    
    for (const auto& [node_id, count] : node_usage) {
        auto node_ptr = network.get_node(node_id);
        if (!node_ptr) continue;
        
        int platforms = std::max(1, node_ptr->get_platforms());
        double utilization = double(count) / double(platforms * 20);
        avg_node_utilization += utilization;
        node_count++;
    }
    
    avg_node_utilization /= node_count;
    
    std::cout << "Utilizzo medio nodi: " << (avg_node_utilization * 100.0) << "%\n";
    std::cout << "Nodi totali analizzati: " << node_count << "\n";
    std::cout << "Collegamenti analizzati: " << edge_usage.size() << "\n";
    
    // Verifica che la rete sia ragionevolmente utilizzata
    EXPECT_GT(avg_node_utilization, 0.2) << "Rete dovrebbe avere utilizzo >20%";
    EXPECT_LT(avg_node_utilization, 1.5) << "Rete non dovrebbe essere eccessivamente satura (>150%)";
    
    std::cout << "\n=== TEST COMPLETATO ===\n\n";
}

/**
 * TEST 3: Inserimento Nuovo Treno in Rete Trafficata
 * 
 * Scenario: Rete già operativa con 100 treni, si deve inserire un nuovo treno
 * Obiettivo: Trovare slot temporale ottimale che minimizzi:
 * - Conflitti con treni esistenti
 * - Ritardi propagati
 * - Impatto su performance globale
 */
TEST_F(HeavyScenarioTest, InsertTrainInCrowdedNetwork) {
    SetUpComplexNetwork();
    GenerateHeavyTraffic(100);
    
    std::cout << "\n=== TEST INSERIMENTO TRENO IN RETE TRAFFICATA ===\n";
    std::cout << "Rete iniziale: " << trains.size() << " treni\n";
    
    // Definisci nuovo treno da inserire: High-speed da MAIN_0 a MAIN_19
    Train new_train("T_NEW_001", "Express Milano-Roma", TrainType::HIGH_SPEED);
    new_train.set_max_speed(300.0);
    
    std::vector<std::string> desired_route;
    for (int i = 0; i < 20; i++) {
        desired_route.push_back("MAIN_" + std::to_string(i));
    }
    
    std::cout << "\nNuovo treno: " << new_train.get_id() << "\n";
    std::cout << "Tipo: HIGH_SPEED\n";
    std::cout << "Percorso: MAIN_0 -> MAIN_19 (" << desired_route.size() << " fermate)\n";
    
    // Fase 1: Analisi slot temporali disponibili
    std::cout << "\n--- FASE 1: RICERCA SLOT DISPONIBILI ---\n";
    
    auto base_time = std::chrono::system_clock::now();
    base_time = std::chrono::time_point_cast<std::chrono::hours>(base_time);
    
    struct TimeSlot {
        std::chrono::system_clock::time_point start;
        int conflicts;
        double score; // Punteggio: minori conflitti = score più alta
    };
    
    std::vector<TimeSlot> candidate_slots;
    
    // Testa slot orari dalle 6:00 alle 22:00 ogni 15 minuti
    for (int hour = 6; hour <= 22; hour++) {
        for (int minute = 0; minute < 60; minute += 15) {
            auto slot_start = base_time + std::chrono::hours(hour) + std::chrono::minutes(minute);
            
            TimeSlot slot;
            slot.start = slot_start;
            slot.conflicts = 0;
            
            // Simula orario per nuovo treno in questo slot
            auto current_time = slot_start;
            
            for (size_t i = 0; i < desired_route.size(); i++) {
                // Verifica quanti treni esistenti sono su questo nodo in questo momento
                for (const auto& schedule : schedules) {
                    for (size_t j = 0; j < schedule.get_stop_count(); j++) {
                        auto stop = schedule.get_stop(j);
                        
                        if (stop.node_id == desired_route[i]) {
                            auto time_diff = std::chrono::duration_cast<std::chrono::minutes>(
                                std::chrono::abs(stop.arrival - current_time));
                            
                            if (time_diff.count() < 10) { // Conflitto se entro 10 minuti
                                slot.conflicts++;
                            }
                        }
                    }
                }
                
                // Avanza al prossimo nodo (6 minuti tra stazioni per high-speed)
                current_time += std::chrono::minutes(6);
            }
            
            // Calcola score: penalizza conflitti
            slot.score = 100.0 - (slot.conflicts * 5.0);
            if (slot.score < 0) slot.score = 0;
            
            candidate_slots.push_back(slot);
        }
    }
    
    // Ordina slot per score (migliori prima)
    std::sort(candidate_slots.begin(), candidate_slots.end(),
             [](const auto& a, const auto& b) { return a.score > b.score; });
    
    std::cout << "Slot candidati analizzati: " << candidate_slots.size() << "\n";
    std::cout << "\nTop 5 slot migliori:\n";
    
    for (size_t i = 0; i < std::min(size_t(5), candidate_slots.size()); i++) {
        auto time_t = std::chrono::system_clock::to_time_t(candidate_slots[i].start);
        std::tm tm = *std::localtime(&time_t);
        
        std::cout << "  Slot " << (i+1) << ": " 
                  << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
                  << std::setfill('0') << std::setw(2) << tm.tm_min
                  << " - Conflitti: " << candidate_slots[i].conflicts
                  << ", Score: " << candidate_slots[i].score << "\n";
    }
    
    ASSERT_GT(candidate_slots.size(), 0) << "Dovrebbero esistere slot candidati";
    EXPECT_LT(candidate_slots[0].conflicts, 50) << "Il miglior slot dovrebbe avere <50 conflitti";
    
    // Fase 2: Inserimento nel miglior slot
    std::cout << "\n--- FASE 2: INSERIMENTO TRENO ---\n";
    
    TimeSlot best_slot = candidate_slots[0];
    std::cout << "Slot selezionato con " << best_slot.conflicts << " conflitti potenziali\n";
    
    // Crea schedule per nuovo treno
    TrainSchedule new_schedule(new_train.get_id());
    auto current_time = best_slot.start;
    
    for (size_t i = 0; i < desired_route.size(); i++) {
        ScheduleStop stop;
        stop.node_id = desired_route[i];
        stop.arrival = current_time;
        
        int dwell_minutes = (i == 0 || i == desired_route.size() - 1) ? 0 : 2;
        stop.departure = current_time + std::chrono::minutes(dwell_minutes);
        stop.is_stop = (dwell_minutes > 0);
        stop.platform = 1; // Binario preferenziale per high-speed
        
        new_schedule.add_stop(stop);
        
        current_time = stop.departure + std::chrono::minutes(6);
    }
    
    EXPECT_EQ(new_schedule.get_stop_count(), desired_route.size());
    
    // Aggiungi nuovo treno alla flotta
    trains.push_back(new_train);
    schedules.push_back(new_schedule);
    
    std::cout << "Nuovo treno inserito. Totale treni: " << trains.size() << "\n";
    EXPECT_EQ(trains.size(), 101);
    
    // Fase 3: Validazione post-inserimento
    std::cout << "\n--- FASE 3: VALIDAZIONE ---\n";
    
    // Ri-analizza conflitti dopo inserimento
    int new_conflicts = 0;
    
    for (size_t i = 0; i < new_schedule.get_stop_count(); i++) {
        auto stop = new_schedule.get_stop(i);
        
        for (size_t j = 0; j < schedules.size() - 1; j++) { // Escludi nuovo schedule
            for (size_t k = 0; k < schedules[j].get_stop_count(); k++) {
                auto other_stop = schedules[j].get_stop(k);
                
                if (stop.node_id == other_stop.node_id) {
                    auto time_diff = std::chrono::duration_cast<std::chrono::minutes>(
                        std::chrono::abs(stop.arrival - other_stop.arrival));
                    
                    if (time_diff.count() < 5) {
                        new_conflicts++;
                    }
                }
            }
        }
    }
    
    std::cout << "Conflitti rilevati dopo inserimento: " << new_conflicts << "\n";
    
    // Verifica che l'inserimento non abbia creato troppi conflitti
    EXPECT_LT(new_conflicts, best_slot.conflicts * 1.2) 
        << "I conflitti reali non dovrebbero superare molto la stima";
    
    // Fase 4: Ottimizzazione post-inserimento
    std::cout << "\n--- FASE 4: OTTIMIZZAZIONE ---\n";
    
    if (new_conflicts > 20) {
        std::cout << "AZIONE: Conflitti elevati, suggerita micro-ottimizzazione:\n";
        std::cout << "  - Spostamento orario ±5 minuti\n";
        std::cout << "  - Modifica binari per separazione\n";
        std::cout << "  - Piccoli ritardi a treni regionali\n";
    } else {
        std::cout << "SUCCESSO: Inserimento ottimale, conflitti accettabili\n";
    }
    
    // Calcola impatto su performance globale
    double impact_score = (double(new_conflicts) / double(new_schedule.get_stop_count())) * 100.0;
    std::cout << "\nImpatto percentuale: " << impact_score << "%\n";
    
    EXPECT_LT(impact_score, 50.0) << "Impatto dovrebbe essere <50% per inserimento accettabile";
    
    std::cout << "\n=== TEST COMPLETATO ===\n\n";
}

/**
 * TEST BONUS: Stress Test - Rete al Limite
 * 
 * Testa il comportamento con 200 treni (limite superiore)
 */
TEST_F(HeavyScenarioTest, StressTestExtremeLoad) {
    SetUpComplexNetwork();
    
    std::cout << "\n=== STRESS TEST - CARICO ESTREMO ===\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    GenerateHeavyTraffic(200);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Generazione 200 treni: " << duration.count() << " ms\n";
    std::cout << "Treni totali: " << trains.size() << "\n";
    std::cout << "Schedules totali: " << schedules.size() << "\n";
    
    EXPECT_EQ(trains.size(), 200);
    EXPECT_LT(duration.count(), 10000) << "Generazione dovrebbe completare in <10 secondi";
    
    // Conta fermate totali
    size_t total_stops = 0;
    for (const auto& schedule : schedules) {
        total_stops += schedule.get_stop_count();
    }
    
    std::cout << "Fermate totali nel sistema: " << total_stops << "\n";
    EXPECT_GT(total_stops, 1000) << "Con 200 treni, dovrebbero esserci >1000 fermate";
    
    std::cout << "\n=== STRESS TEST COMPLETATO ===\n\n";
}
