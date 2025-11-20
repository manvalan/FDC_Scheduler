/**
 * @file railway_ai_integration_example.cpp
 * @brief Esempio completo di risoluzione conflitti con RailwayAI
 * 
 * Dimostra:
 * - Rilevamento conflitti su binario doppio, singolo e stazioni
 * - Risoluzione automatica tramite RailwayAI
 * - Gestione priorità treni
 * - Ottimizzazione piattaforme
 */

#include <fdc_scheduler/railway_network.hpp>
#include <fdc_scheduler/schedule.hpp>
#include <fdc_scheduler/conflict_detector.hpp>
#include <fdc_scheduler/railway_ai_resolver.hpp>
#include <fdc_scheduler/train.hpp>
#include <iostream>
#include <iomanip>
#include <memory>

using namespace fdc_scheduler;

// Helper per stampare timestamp
std::string format_time(std::chrono::system_clock::time_point tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&tt), "%H:%M:%S");
    return oss.str();
}

// Helper per creare timestamp da ore:minuti
std::chrono::system_clock::time_point make_time(int hour, int minute) {
    auto now = std::chrono::system_clock::now();
    auto today = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&today);
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = 0;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

void print_section_header(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

void print_schedule(const std::shared_ptr<TrainSchedule>& schedule) {
    std::cout << "Train: " << schedule->get_train_id() << "\n";
    
    const auto& stops = schedule->get_stops();
    for (const auto& stop : stops) {
        std::cout << "  " << stop.get_node_id() 
                  << " | Arr: " << format_time(stop.arrival)
                  << " | Dep: " << format_time(stop.departure);
        
        if (stop.platform > 0) {
            std::cout << " | Platform: " << stop.platform;
        }
        std::cout << "\n";
    }
}

void print_conflict(const Conflict& conflict) {
    std::cout << "⚠️  CONFLICT [" << conflict_type_to_string(conflict.type) << "]\n";
    std::cout << "   Trains: " << conflict.train1_id << " ↔ " << conflict.train2_id << "\n";
    std::cout << "   Location: " << conflict.location << "\n";
    std::cout << "   Time: " << format_time(conflict.conflict_time) << "\n";
    std::cout << "   Severity: " << conflict.severity << "/10\n";
    std::cout << "   Details: " << conflict.description << "\n\n";
}

void print_resolution(const ResolutionResult& result) {
    std::cout << "✓ RESOLUTION: " << (result.success ? "SUCCESS" : "FAILED") << "\n";
    std::cout << "  Strategy: " << strategy_to_string(result.strategy_used) << "\n";
    std::cout << "  Description: " << result.description << "\n";
    std::cout << "  Total delay: " << result.total_delay.count() << " seconds\n";
    std::cout << "  Quality score: " << std::fixed << std::setprecision(2) 
              << result.quality_score << "\n";
    std::cout << "  Modified trains: ";
    for (const auto& train : result.modified_trains) {
        std::cout << train << " ";
    }
    std::cout << "\n\n";
}

/**
 * Scenario 1: Conflitto su binario doppio
 * Due treni viaggiano nella stessa direzione con headway insufficiente
 */
void demo_double_track_conflict() {
    print_section_header("SCENARIO 1: Conflitto su Binario Doppio");
    
    // Crea rete con binario doppio
    RailwayNetwork network;
    network.add_node(Node("MILANO", "Milano Centrale", NodeType::STATION, 12));
    network.add_node(Node("BOLOGNA", "Bologna Centrale", NodeType::STATION, 16));
    network.add_node(Node("FIRENZE", "Firenze SMN", NodeType::STATION, 16));
    
    // Linea ad alta velocità - binario doppio
    network.add_edge(Edge("MILANO", "BOLOGNA", 218.0, TrackType::DOUBLE, 300.0));
    network.add_edge(Edge("BOLOGNA", "FIRENZE", 78.0, TrackType::DOUBLE, 300.0));
    
    std::cout << "Network: Milano → Bologna → Firenze (Double track, 300 km/h)\n\n";
    
    // Crea due treni rapidi che viaggiano vicini
    auto train1 = std::make_shared<TrainSchedule>("FR9615");
    train1->add_stop(ScheduleStop("MILANO", make_time(8, 0), make_time(8, 0), true));
    train1->get_stop(0).platform = 5;
    train1->add_stop(ScheduleStop("BOLOGNA", make_time(8, 45), make_time(8, 47), true));
    train1->get_stop(1).platform = 3;
    train1->add_stop(ScheduleStop("FIRENZE", make_time(9, 15), make_time(9, 15), true));
    train1->get_stop(2).platform = 7;
    
    auto train2 = std::make_shared<TrainSchedule>("FR9617");
    train2->add_stop(ScheduleStop("MILANO", make_time(8, 5), make_time(8, 5), true));  // Solo 5 min dopo
    train2->get_stop(0).platform = 6;
    train2->add_stop(ScheduleStop("BOLOGNA", make_time(8, 50), make_time(8, 52), true));
    train2->get_stop(1).platform = 4;
    train2->add_stop(ScheduleStop("FIRENZE", make_time(9, 20), make_time(9, 20), true));
    train2->get_stop(2).platform = 8;
    
    std::cout << "PRIMA della risoluzione:\n";
    print_schedule(train1);
    std::cout << "\n";
    print_schedule(train2);
    std::cout << "\n";
    
    // Rileva conflitti
    ConflictDetector detector(network);
    std::vector<std::shared_ptr<TrainSchedule>> schedules = {train1, train2};
    auto conflicts = detector.detect_all(schedules);
    
    std::cout << "Conflitti rilevati: " << conflicts.size() << "\n\n";
    for (const auto& conflict : conflicts) {
        print_conflict(conflict);
    }
    
    if (!conflicts.empty()) {
        // Risolvi con RailwayAI
        RailwayAIConfig ai_config;
        ai_config.min_headway_seconds = 120;  // 2 minuti minimo tra treni
        
        RailwayAIResolver resolver(network, ai_config);
        auto result = resolver.resolve_conflicts(schedules, conflicts);
        
        print_resolution(result);
        
        std::cout << "DOPO la risoluzione:\n";
        print_schedule(train1);
        std::cout << "\n";
        print_schedule(train2);
    }
}

/**
 * Scenario 2: Conflitto su binario singolo
 * Due treni viaggiano in direzioni opposte su binario unico
 */
void demo_single_track_conflict() {
    print_section_header("SCENARIO 2: Conflitto su Binario Singolo");
    
    // Crea rete con binario singolo
    RailwayNetwork network;
    network.add_node(Node("STAZIONE_A", "Stazione A", NodeType::STATION, 2));
    network.add_node(Node("STAZIONE_B", "Stazione B", NodeType::STATION, 3));
    network.add_node(Node("STAZIONE_C", "Stazione C", NodeType::STATION, 2));
    
    // Linea secondaria - binario singolo
    network.add_edge(Edge("STAZIONE_A", "STAZIONE_B", 35.0, TrackType::SINGLE, 100.0));
    network.add_edge(Edge("STAZIONE_B", "STAZIONE_C", 28.0, TrackType::SINGLE, 100.0));
    
    std::cout << "Network: A ↔ B ↔ C (Single track, 100 km/h)\n";
    std::cout << "Station B has 3 platforms (passing loop)\n\n";
    
    // Treno 1: A → C
    auto train1 = std::make_shared<TrainSchedule>("REG8024");
    train1->add_stop(ScheduleStop("STAZIONE_A", make_time(10, 0), make_time(10, 0), true));
    train1->get_stop(0).platform = 1;
    train1->add_stop(ScheduleStop("STAZIONE_B", make_time(10, 25), make_time(10, 27), true));
    train1->get_stop(1).platform = 1;
    train1->add_stop(ScheduleStop("STAZIONE_C", make_time(10, 45), make_time(10, 45), true));
    train1->get_stop(2).platform = 1;
    
    // Treno 2: C → A (direzione opposta!)
    auto train2 = std::make_shared<TrainSchedule>("REG8025");
    train2->add_stop(ScheduleStop("STAZIONE_C", make_time(10, 0), make_time(10, 0), true));
    train2->get_stop(0).platform = 1;
    train2->add_stop(ScheduleStop("STAZIONE_B", make_time(10, 20), make_time(10, 22), true));
    train2->get_stop(1).platform = 2;
    train2->add_stop(ScheduleStop("STAZIONE_A", make_time(10, 45), make_time(10, 45), true));
    train2->get_stop(2).platform = 1;
    
    std::cout << "PRIMA della risoluzione:\n";
    print_schedule(train1);
    std::cout << "\n";
    print_schedule(train2);
    std::cout << "\n";
    
    // Rileva conflitti
    ConflictDetector detector(network);
    std::vector<std::shared_ptr<TrainSchedule>> schedules = {train1, train2};
    auto conflicts = detector.detect_all(schedules);
    
    std::cout << "Conflitti rilevati: " << conflicts.size() << "\n\n";
    for (const auto& conflict : conflicts) {
        print_conflict(conflict);
    }
    
    if (!conflicts.empty()) {
        // Risolvi con RailwayAI
        RailwayAIConfig ai_config;
        ai_config.allow_single_track_meets = true;
        ai_config.single_track_meet_buffer = 300;  // 5 minuti di buffer
        
        RailwayAIResolver resolver(network, ai_config);
        auto result = resolver.resolve_conflicts(schedules, conflicts);
        
        print_resolution(result);
        
        std::cout << "DOPO la risoluzione:\n";
        print_schedule(train1);
        std::cout << "\n";
        print_schedule(train2);
    }
}

/**
 * Scenario 3: Conflitto di piattaforma in stazione
 * Due treni cercano di usare la stessa piattaforma contemporaneamente
 */
void demo_station_conflict() {
    print_section_header("SCENARIO 3: Conflitto di Piattaforma");
    
    // Crea rete
    RailwayNetwork network;
    network.add_node(Node("ROMA", "Roma Termini", NodeType::STATION, 24));
    network.add_node(Node("NAPOLI", "Napoli Centrale", NodeType::STATION, 18));
    network.add_node(Node("SALERNO", "Salerno", NodeType::STATION, 8));
    
    network.add_edge(Edge("ROMA", "NAPOLI", 225.0, TrackType::HIGH_SPEED, 250.0));
    network.add_edge(Edge("NAPOLI", "SALERNO", 52.0, TrackType::DOUBLE, 200.0));
    
    std::cout << "Network: Roma → Napoli → Salerno\n";
    std::cout << "Napoli has 18 platforms\n\n";
    
    // Due treni che arrivano a Napoli quasi contemporaneamente
    auto train1 = std::make_shared<TrainSchedule>("FA8522");
    train1->add_stop(ScheduleStop("ROMA", make_time(14, 0), make_time(14, 0), true));
    train1->get_stop(0).platform = 12;
    train1->add_stop(ScheduleStop("NAPOLI", make_time(15, 5), make_time(15, 25), true));  // Platform 5
    train1->get_stop(1).platform = 5;
    train1->add_stop(ScheduleStop("SALERNO", make_time(15, 45), make_time(15, 45), true));
    train1->get_stop(2).platform = 3;
    
    auto train2 = std::make_shared<TrainSchedule>("IC608");
    train2->add_stop(ScheduleStop("ROMA", make_time(14, 10), make_time(14, 10), true));
    train2->get_stop(0).platform = 8;
    train2->add_stop(ScheduleStop("NAPOLI", make_time(15, 10), make_time(15, 30), true));  // Stessa platform!
    train2->get_stop(1).platform = 5;
    train2->add_stop(ScheduleStop("SALERNO", make_time(15, 50), make_time(15, 50), true));
    train2->get_stop(2).platform = 2;
    
    std::cout << "PRIMA della risoluzione:\n";
    print_schedule(train1);
    std::cout << "\n";
    print_schedule(train2);
    std::cout << "\n";
    
    // Rileva conflitti
    ConflictDetector detector(network);
    std::vector<std::shared_ptr<TrainSchedule>> schedules = {train1, train2};
    auto conflicts = detector.detect_all(schedules);
    
    std::cout << "Conflitti rilevati: " << conflicts.size() << "\n\n";
    for (const auto& conflict : conflicts) {
        print_conflict(conflict);
    }
    
    if (!conflicts.empty()) {
        // Risolvi con RailwayAI
        RailwayAIConfig ai_config;
        ai_config.allow_platform_reassignment = true;
        ai_config.optimize_platform_usage = true;
        ai_config.platform_buffer_seconds = 180;  // 3 minuti tra treni
        
        RailwayAIResolver resolver(network, ai_config);
        auto result = resolver.resolve_conflicts(schedules, conflicts);
        
        print_resolution(result);
        
        std::cout << "DOPO la risoluzione:\n";
        print_schedule(train1);
        std::cout << "\n";
        print_schedule(train2);
    }
}

/**
 * Scenario 4: Scenario complesso con conflitti multipli
 */
void demo_complex_scenario() {
    print_section_header("SCENARIO 4: Scenario Complesso Multi-Conflitto");
    
    // Rete mista con binario singolo e doppio
    RailwayNetwork network;
    network.add_node(Node("TORINO", "Torino Porta Nuova", NodeType::STATION, 16));
    network.add_node(Node("ALESSANDRIA", "Alessandria", NodeType::STATION, 8));
    network.add_node(Node("GENOVA", "Genova Piazza Principe", NodeType::STATION, 12));
    network.add_node(Node("LA_SPEZIA", "La Spezia Centrale", NodeType::STATION, 6));
    
    // Torino-Alessandria: doppio binario
    network.add_edge(Edge("TORINO", "ALESSANDRIA", 88.0, TrackType::DOUBLE, 200.0));
    // Alessandria-Genova: doppio binario
    network.add_edge(Edge("ALESSANDRIA", "GENOVA", 82.0, TrackType::DOUBLE, 160.0));
    // Genova-La Spezia: binario singolo (costa)
    network.add_edge(Edge("GENOVA", "LA_SPEZIA", 93.0, TrackType::SINGLE, 120.0));
    
    std::cout << "Network: Torino → Alessandria → Genova → La Spezia\n";
    std::cout << "  Torino-Genova: Double track\n";
    std::cout << "  Genova-La Spezia: Single track (coastal line)\n\n";
    
    // Crea 4 treni con vari conflitti
    auto train1 = std::make_shared<TrainSchedule>("IC650");  // Intercity
    train1->add_stop(ScheduleStop("TORINO", make_time(12, 0), make_time(12, 0), true));
    train1->get_stop(0).platform = 10;
    train1->add_stop(ScheduleStop("ALESSANDRIA", make_time(12, 30), make_time(12, 32), true));
    train1->get_stop(1).platform = 3;
    train1->add_stop(ScheduleStop("GENOVA", make_time(13, 15), make_time(13, 20), true));
    train1->get_stop(2).platform = 5;
    train1->add_stop(ScheduleStop("LA_SPEZIA", make_time(14, 10), make_time(14, 10), true));
    train1->get_stop(3).platform = 2;
    
    auto train2 = std::make_shared<TrainSchedule>("REG10234");  // Regionale lento
    train2->add_stop(ScheduleStop("TORINO", make_time(12, 10), make_time(12, 10), true));
    train2->get_stop(0).platform = 8;
    train2->add_stop(ScheduleStop("ALESSANDRIA", make_time(12, 55), make_time(12, 57), true));
    train2->get_stop(1).platform = 2;
    train2->add_stop(ScheduleStop("GENOVA", make_time(14, 0), make_time(14, 5), true));  // Platform conflict!
    train2->get_stop(2).platform = 5;
    train2->add_stop(ScheduleStop("LA_SPEZIA", make_time(15, 15), make_time(15, 15), true));
    train2->get_stop(3).platform = 3;
    
    auto train3 = std::make_shared<TrainSchedule>("IC652");  // Altro IC
    train3->add_stop(ScheduleStop("LA_SPEZIA", make_time(13, 0), make_time(13, 0), true));  // Opposta direzione!
    train3->get_stop(0).platform = 1;
    train3->add_stop(ScheduleStop("GENOVA", make_time(13, 50), make_time(13, 55), true));
    train3->get_stop(1).platform = 8;
    train3->add_stop(ScheduleStop("ALESSANDRIA", make_time(14, 40), make_time(14, 42), true));
    train3->get_stop(2).platform = 4;
    train3->add_stop(ScheduleStop("TORINO", make_time(15, 15), make_time(15, 15), true));
    train3->get_stop(3).platform = 12;
    
    auto train4 = std::make_shared<TrainSchedule>("REG10236");  // Altro regionale
    train4->add_stop(ScheduleStop("GENOVA", make_time(13, 30), make_time(13, 30), true));
    train4->get_stop(0).platform = 7;
    train4->add_stop(ScheduleStop("LA_SPEZIA", make_time(14, 30), make_time(14, 30), true));
    train4->get_stop(1).platform = 1;
    
    std::cout << "4 treni con conflitti multipli:\n\n";
    print_schedule(train1);
    std::cout << "\n";
    print_schedule(train2);
    std::cout << "\n";
    print_schedule(train3);
    std::cout << "\n";
    print_schedule(train4);
    std::cout << "\n";
    
    // Rileva tutti i conflitti
    ConflictDetector detector(network);
    std::vector<std::shared_ptr<TrainSchedule>> schedules = {train1, train2, train3, train4};
    auto conflicts = detector.detect_all(schedules);
    
    std::cout << "Conflitti rilevati: " << conflicts.size() << "\n\n";
    for (const auto& conflict : conflicts) {
        print_conflict(conflict);
    }
    
    if (!conflicts.empty()) {
        // Risolvi con RailwayAI
        RailwayAIConfig ai_config;
        ai_config.delay_weight = 1.0;
        ai_config.passenger_impact_weight = 1.5;  // IC hanno priorità
        ai_config.allow_platform_reassignment = true;
        ai_config.allow_single_track_meets = true;
        
        RailwayAIResolver resolver(network, ai_config);
        auto result = resolver.resolve_conflicts(schedules, conflicts);
        
        print_resolution(result);
        
        std::cout << "DOPO la risoluzione:\n";
        for (const auto& sched : schedules) {
            print_schedule(sched);
            std::cout << "\n";
        }
        
        // Mostra statistiche
        auto stats = resolver.get_statistics();
        std::cout << "\n📊 STATISTICHE RailwayAI:\n";
        for (const auto& [key, value] : stats) {
            std::cout << "  " << key << ": " << value << "\n";
        }
    }
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   FDC Scheduler - RailwayAI Conflict Resolution Demo            ║\n";
    std::cout << "║   Integrazione Risoluzione Conflitti su Binari e Stazioni       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    try {
        // Esegui gli scenari dimostrativi
        demo_double_track_conflict();
        demo_single_track_conflict();
        demo_station_conflict();
        demo_complex_scenario();
        
        print_section_header("DEMO COMPLETATA");
        std::cout << "Tutti gli scenari sono stati eseguiti con successo!\n\n";
        std::cout << "Funzionalità dimostrate:\n";
        std::cout << "  ✓ Rilevamento conflitti su binario doppio\n";
        std::cout << "  ✓ Rilevamento e risoluzione collisioni frontali su binario singolo\n";
        std::cout << "  ✓ Gestione conflitti di piattaforma in stazione\n";
        std::cout << "  ✓ Ottimizzazione basata su priorità treni\n";
        std::cout << "  ✓ Distribuzione intelligente dei ritardi\n";
        std::cout << "  ✓ Riassegnazione automatica piattaforme\n";
        std::cout << "  ✓ Identificazione punti di incrocio su binario singolo\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Errore: " << e.what() << "\n";
        return 1;
    }
}
