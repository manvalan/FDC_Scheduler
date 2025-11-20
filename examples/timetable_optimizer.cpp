/**
 * @file timetable_optimizer.cpp
 * @brief Analisi rete ferroviaria e ottimizzazione orari cadenzati
 * 
 * Questo esempio dimostra:
 * 1. Analisi del traffico esistente su una rete
 * 2. Identificazione pattern di congestione
 * 3. Generazione orari cadenzati ottimizzati
 * 4. Calcolo capacità massima delle tratte
 * 5. Suggerimenti per miglioramento servizio
 */

#include "fdc_scheduler/railway_network.hpp"
#include "fdc_scheduler/schedule.hpp"
#include "fdc_scheduler/conflict_detector.hpp"
#include "fdc_scheduler/railway_ai_resolver.hpp"
#include "fdc_scheduler/node.hpp"
#include "fdc_scheduler/edge.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <cmath>

using namespace fdc_scheduler;

// Helper per creare time_point
std::chrono::system_clock::time_point make_time(int hour, int minute) {
    std::tm tm = {};
    tm.tm_year = 2025 - 1900;
    tm.tm_mon = 10;
    tm.tm_mday = 20;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = 0;
    std::time_t tt = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(tt);
}

std::string format_time(const std::chrono::system_clock::time_point& tp) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
       << std::setfill('0') << std::setw(2) << tm.tm_min;
    return ss.str();
}

void print_header(const std::string& title) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ " << std::left << std::setw(66) << title << " ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";
}

void print_section(const std::string& title) {
    std::cout << "\n> " << title << "\n";
    std::cout << std::string(70, '-') << "\n";
}

/**
 * Struttura per analisi della capacità di una tratta
 */
struct SectionCapacity {
    std::string section_id;
    std::string from_station;
    std::string to_station;
    TrackType track_type;
    int theoretical_capacity;    // Treni per ora teorici
    int current_usage;           // Treni per ora attuali
    double utilization_percent;  // Percentuale utilizzo
    int suggested_capacity;      // Capacità suggerita
    std::vector<int> trains_per_hour; // Distribuzione oraria
};

/**
 * Struttura per pattern di servizio cadenzato
 */
struct CadencedPattern {
    std::string service_name;
    std::string route;
    int frequency_minutes;       // Cadenza in minuti (es: 30, 60)
    std::vector<std::chrono::system_clock::time_point> departure_times;
    double regularity_score;     // Score di regolarità (0-1)
    std::string recommendation;
};

/**
 * Classe per analisi e ottimizzazione orari
 */
class TimetableOptimizer {
private:
    const RailwayNetwork& network_;
    std::vector<std::shared_ptr<TrainSchedule>> schedules_;
    std::map<std::string, SectionCapacity> section_analysis_;
    std::vector<CadencedPattern> patterns_;
    
public:
    TimetableOptimizer(const RailwayNetwork& network) 
        : network_(network) {}
    
    void add_schedule(std::shared_ptr<TrainSchedule> schedule) {
        schedules_.push_back(schedule);
    }
    
    /**
     * Analizza capacità delle tratte
     */
    void analyze_section_capacity() {
        print_section("ANALISI CAPACITÀ TRATTE");
        
        // Per ogni arco della rete
        auto edges = network_.get_all_edges();
        
        for (const auto& edge_ptr : edges) {
            if (!edge_ptr) continue;
            const auto& edge = *edge_ptr;
            
            SectionCapacity cap;
            cap.section_id = edge.get_from_node() + "-" + edge.get_to_node();
            cap.from_station = edge.get_from_node();
            cap.to_station = edge.get_to_node();
            cap.track_type = edge.get_track_type();
            
            // Calcola capacità teorica basata su tipo binario
            switch (edge.get_track_type()) {
                case TrackType::SINGLE:
                    cap.theoretical_capacity = 4;  // 4 treni/ora binario singolo
                    break;
                case TrackType::DOUBLE:
                    cap.theoretical_capacity = 12; // 12 treni/ora binario doppio
                    break;
                case TrackType::HIGH_SPEED:
                    cap.theoretical_capacity = 20; // 20 treni/ora alta velocità
                    break;
                default:
                    cap.theoretical_capacity = 6;
            }
            
            // Conta treni che usano questa tratta
            cap.trains_per_hour.resize(24, 0);
            cap.current_usage = 0;
            
            for (const auto& schedule : schedules_) {
                const auto& stops = schedule->get_stops();
                for (size_t i = 0; i < stops.size() - 1; i++) {
                    if ((stops[i].node_id == cap.from_station && 
                         stops[i+1].node_id == cap.to_station) ||
                        (stops[i].node_id == cap.to_station && 
                         stops[i+1].node_id == cap.from_station)) {
                        
                        cap.current_usage++;
                        
                        // Registra ora di utilizzo
                        auto tt = std::chrono::system_clock::to_time_t(stops[i].departure);
                        std::tm tm = *std::localtime(&tt);
                        if (tm.tm_hour >= 0 && tm.tm_hour < 24) {
                            cap.trains_per_hour[tm.tm_hour]++;
                        }
                    }
                }
            }
            
            // Calcola utilizzo medio per ora
            int max_hourly = *std::max_element(cap.trains_per_hour.begin(), 
                                               cap.trains_per_hour.end());
            cap.utilization_percent = (max_hourly * 100.0) / cap.theoretical_capacity;
            cap.suggested_capacity = std::min(cap.theoretical_capacity, 
                                             max_hourly + 2); // Buffer di 2 treni
            
            section_analysis_[cap.section_id] = cap;
        }
        
        // Stampa analisi
        std::cout << std::left << std::setw(25) << "TRATTA"
                  << std::setw(12) << "TIPO"
                  << std::setw(10) << "CAP.TEOR"
                  << std::setw(10) << "USO MAX"
                  << std::setw(12) << "UTILIZZO"
                  << "ORA PUNTA\n";
        std::cout << std::string(70, '-') << "\n";
        
        for (const auto& [id, cap] : section_analysis_) {
            auto max_it = std::max_element(cap.trains_per_hour.begin(), 
                                          cap.trains_per_hour.end());
            int peak_hour = std::distance(cap.trains_per_hour.begin(), max_it);
            
            std::string track_type_str;
            switch (cap.track_type) {
                case TrackType::SINGLE: track_type_str = "Singolo"; break;
                case TrackType::DOUBLE: track_type_str = "Doppio"; break;
                case TrackType::HIGH_SPEED: track_type_str = "AV"; break;
                default: track_type_str = "Standard";
            }
            
            std::cout << std::left << std::setw(25) << id
                      << std::setw(12) << track_type_str
                      << std::setw(10) << cap.theoretical_capacity
                      << std::setw(10) << *max_it
                      << std::setw(11) << (std::to_string(static_cast<int>(cap.utilization_percent)) + "%")
                      << peak_hour << ":00-" << (peak_hour+1) << ":00\n";
        }
    }
    
    /**
     * Identifica pattern di servizio cadenzato esistenti
     */
    void identify_cadenced_patterns() {
        print_section("PATTERN SERVIZI CADENZATI ESISTENTI");
        
        // Raggruppa treni per route
        std::map<std::string, std::vector<std::shared_ptr<TrainSchedule>>> routes;
        
        for (const auto& schedule : schedules_) {
            const auto& stops = schedule->get_stops();
            if (stops.size() >= 2) {
                std::string route = stops.front().node_id + " → " + stops.back().node_id;
                routes[route].push_back(schedule);
            }
        }
        
        // Analizza ogni route
        for (const auto& [route, trains] : routes) {
            if (trains.size() < 2) continue;
            
            CadencedPattern pattern;
            pattern.route = route;
            pattern.service_name = "Servizio " + route;
            
            // Raccogli orari di partenza
            std::vector<int> departure_minutes;
            for (const auto& train : trains) {
                auto dep_time = train->get_stops().front().departure;
                pattern.departure_times.push_back(dep_time);
                
                auto tt = std::chrono::system_clock::to_time_t(dep_time);
                std::tm tm = *std::localtime(&tt);
                departure_minutes.push_back(tm.tm_hour * 60 + tm.tm_min);
            }
            
            std::sort(departure_minutes.begin(), departure_minutes.end());
            
            // Calcola intervalli tra partenze
            std::vector<int> intervals;
            for (size_t i = 1; i < departure_minutes.size(); i++) {
                intervals.push_back(departure_minutes[i] - departure_minutes[i-1]);
            }
            
            if (!intervals.empty()) {
                // Calcola cadenza media
                double avg_interval = std::accumulate(intervals.begin(), intervals.end(), 0.0) 
                                    / intervals.size();
                pattern.frequency_minutes = static_cast<int>(std::round(avg_interval));
                
                // Calcola regolarità (deviazione standard normalizzata)
                double variance = 0;
                for (int interval : intervals) {
                    variance += std::pow(interval - avg_interval, 2);
                }
                variance /= intervals.size();
                double std_dev = std::sqrt(variance);
                pattern.regularity_score = 1.0 - std::min(1.0, std_dev / avg_interval);
                
                // Genera raccomandazione
                if (pattern.regularity_score > 0.9) {
                    pattern.recommendation = "✅ Cadenza ottima";
                } else if (pattern.regularity_score > 0.7) {
                    pattern.recommendation = "⚠️  Cadenza buona, piccoli aggiustamenti";
                } else {
                    int suggested_cadence = (pattern.frequency_minutes < 30) ? 15 :
                                          (pattern.frequency_minutes < 45) ? 30 : 60;
                    pattern.recommendation = "❌ Cadenza irregolare, suggerito: " + 
                                           std::to_string(suggested_cadence) + " minuti";
                }
                
                patterns_.push_back(pattern);
            }
        }
        
        // Stampa patterns
        std::cout << std::left << std::setw(30) << "ROUTE"
                  << std::setw(12) << "CADENZA"
                  << std::setw(12) << "TRENI/H"
                  << std::setw(15) << "REGOLARITA"
                  << "VALUTAZIONE\n";
        std::cout << std::string(90, '-') << "\n";
        
        for (const auto& pattern : patterns_) {
            int trains_per_hour = pattern.frequency_minutes > 0 ? 
                                 60 / pattern.frequency_minutes : 0;
            
            std::cout << std::left << std::setw(30) << pattern.route
                      << std::setw(12) << (std::to_string(pattern.frequency_minutes) + " min")
                      << std::setw(12) << trains_per_hour
                      << std::setw(15) << (std::to_string(static_cast<int>(pattern.regularity_score * 100)) + "%")
                      << pattern.recommendation << "\n";
        }
    }
    
    /**
     * Genera proposta di orario cadenzato ottimizzato
     */
    void generate_optimized_timetable() {
        print_section("PROPOSTA ORARIO CADENZATO OTTIMIZZATO");
        
        std::cout << "\n📋 LINEE GUIDA GENERALI:\n\n";
        
        // Analizza colli di bottiglia
        std::vector<std::pair<std::string, double>> bottlenecks;
        for (const auto& [id, cap] : section_analysis_) {
            if (cap.utilization_percent > 80) {
                bottlenecks.push_back({id, cap.utilization_percent});
            }
        }
        
        std::sort(bottlenecks.begin(), bottlenecks.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        if (!bottlenecks.empty()) {
            std::cout << "⚠️  COLLI DI BOTTIGLIA IDENTIFICATI:\n";
            for (size_t i = 0; i < std::min(size_t(5), bottlenecks.size()); i++) {
                std::cout << "   " << (i+1) << ". " << bottlenecks[i].first 
                         << " (utilizzo: " << static_cast<int>(bottlenecks[i].second) << "%)\n";
            }
            std::cout << "\n   → Suggerimento: Ridurre frequenza o aggiungere tracce di sorpasso\n\n";
        }
        
        // Proponi pattern cadenzati per ogni route
        std::cout << "🎯 PATTERN CADENZATI SUGGERITI:\n\n";
        
        for (const auto& pattern : patterns_) {
            std::cout << "Route: " << pattern.route << "\n";
            
            if (pattern.regularity_score < 0.8) {
                // Proponi nuovo schema
                int optimal_cadence;
                int current_trains = pattern.departure_times.size();
                
                // Determina cadenza ottimale basata su volume traffico
                if (current_trains >= 12) {
                    optimal_cadence = 30;  // 2 treni/ora
                } else if (current_trains >= 6) {
                    optimal_cadence = 60;  // 1 treno/ora
                } else {
                    optimal_cadence = 120; // 1 treno/2 ore
                }
                
                std::cout << "  Cadenza attuale: " << pattern.frequency_minutes << " min (irregolare)\n";
                std::cout << "  Cadenza proposta: " << optimal_cadence << " min\n";
                std::cout << "  Orari suggeriti: ";
                
                // Genera orari cadenzati dalle 6:00 alle 22:00
                int start_minute = 0; // Partenza al minuto :00
                for (int hour = 6; hour < 22; hour++) {
                    for (int min = start_minute; min < 60; min += optimal_cadence) {
                        std::cout << std::setfill('0') << std::setw(2) << hour << ":"
                                 << std::setfill('0') << std::setw(2) << min << " ";
                        if (min + optimal_cadence >= 60) break;
                    }
                }
                std::cout << "\n\n";
            } else {
                std::cout << "  ✅ Pattern attuale già ottimale (cadenza: " 
                         << pattern.frequency_minutes << " min)\n\n";
            }
        }
    }
    
    /**
     * Suggerimenti di miglioramento globale
     */
    void generate_recommendations() {
        print_section("RACCOMANDAZIONI DI MIGLIORAMENTO");
        
        std::cout << "\n1️⃣  OTTIMIZZAZIONE CAPACITÀ:\n";
        
        // Identifica tratte sovraccariche
        int overloaded = 0;
        for (const auto& [id, cap] : section_analysis_) {
            if (cap.utilization_percent > 90) {
                overloaded++;
                std::cout << "   ⚠️  " << id << " - Sovraccarico critico (" 
                         << static_cast<int>(cap.utilization_percent) << "%)\n";
                std::cout << "      → Suggerimento: ";
                if (cap.track_type == TrackType::SINGLE) {
                    std::cout << "Considerare raddoppio binario\n";
                } else {
                    std::cout << "Implementare sistema di gestione traffico avanzato\n";
                }
            }
        }
        
        if (overloaded == 0) {
            std::cout << "   ✅ Nessuna tratta in sovraccarico critico\n";
        }
        
        std::cout << "\n2️⃣  CADENZAMENTO SERVIZI:\n";
        
        int irregular = 0;
        for (const auto& pattern : patterns_) {
            if (pattern.regularity_score < 0.7) {
                irregular++;
            }
        }
        
        if (irregular > 0) {
            std::cout << "   ⚠️  " << irregular << " servizi con cadenza irregolare\n";
            std::cout << "      → Suggerimento: Adottare schema a memoria (es: :00, :30)\n";
        } else {
            std::cout << "   ✅ Tutti i servizi hanno buon cadenzamento\n";
        }
        
        std::cout << "\n3️⃣  EFFICIENZA OPERATIVA:\n";
        
        // Calcola numero totale conflitti
        ConflictDetector detector(network_);
        auto conflicts = detector.detect_all(schedules_);
        
        std::cout << "   Conflitti rilevati: " << conflicts.size() << "\n";
        
        if (conflicts.size() > 0) {
            std::cout << "      → Suggerimento: Applicare RailwayAI per risoluzione automatica\n";
            std::cout << "      → Conflitti rilevati che richiedono attenzione manuale\n";
        } else {
            std::cout << "   ✅ Nessun conflitto rilevato\n";
        }
        
        std::cout << "\n4️⃣  BILANCIAMENTO CARICO:\n";
        
        // Analizza distribuzione oraria
        std::vector<int> hourly_distribution(24, 0);
        for (const auto& schedule : schedules_) {
            auto dep_time = schedule->get_stops().front().departure;
            auto tt = std::chrono::system_clock::to_time_t(dep_time);
            std::tm tm = *std::localtime(&tt);
            hourly_distribution[tm.tm_hour]++;
        }
        
        int peak_hour = std::distance(hourly_distribution.begin(),
                                     std::max_element(hourly_distribution.begin(),
                                                     hourly_distribution.end()));
        int off_peak = std::distance(hourly_distribution.begin(),
                                    std::min_element(hourly_distribution.begin() + 6,
                                                    hourly_distribution.begin() + 22));
        
        std::cout << "   Ora di punta: " << peak_hour << ":00 (" 
                 << hourly_distribution[peak_hour] << " treni)\n";
        std::cout << "   Ora di morbida: " << off_peak << ":00 (" 
                 << hourly_distribution[off_peak] << " treni)\n";
        
        if (hourly_distribution[peak_hour] > 2 * hourly_distribution[off_peak]) {
            std::cout << "      → Suggerimento: Distribuire meglio il carico nelle ore di morbida\n";
        }
    }
    
    /**
     * Genera report completo
     */
    void generate_full_report() {
        print_header("ANALISI COMPLETA RETE FERROVIARIA E OTTIMIZZAZIONE ORARI");
        
        std::cout << "STATISTICHE GENERALI:\n";
        std::cout << "   Stazioni: " << network_.num_nodes() << "\n";
        std::cout << "   Tratte: " << network_.num_edges() << "\n";
        std::cout << "   Treni schedulati: " << schedules_.size() << "\n";
        
        analyze_section_capacity();
        identify_cadenced_patterns();
        generate_optimized_timetable();
        generate_recommendations();
        
        print_section("FINE ANALISI");
        std::cout << "\n✅ Analisi completata con successo!\n\n";
    }
};

/**
 * Main: Scenario di test con rete realistica
 */
int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                      ║\n";
    std::cout << "║         FDC_Scheduler - Ottimizzatore Orari Cadenzati               ║\n";
    std::cout << "║              Analisi Rete e Suggerimenti Intelligenti               ║\n";
    std::cout << "║                                                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n";
    
    try {
        // Crea rete ferroviaria regionale
        RailwayNetwork network;
        
        // Linea principale A-B-C-D-E (100 km totali)
        network.add_node(Node("A", "Città A", NodeType::STATION, 8));
        network.add_node(Node("B", "Città B", NodeType::STATION, 12));
        network.add_node(Node("C", "Città C", NodeType::STATION, 16));
        network.add_node(Node("D", "Città D", NodeType::STATION, 10));
        network.add_node(Node("E", "Città E", NodeType::STATION, 8));
        
        // Diramazione da C verso F
        network.add_node(Node("F", "Città F", NodeType::STATION, 6));
        
        // Tratte principali
        network.add_edge(Edge("A", "B", 25.0, TrackType::DOUBLE, 160.0));
        network.add_edge(Edge("B", "C", 30.0, TrackType::DOUBLE, 160.0));
        network.add_edge(Edge("C", "D", 20.0, TrackType::SINGLE, 120.0));  // Collo di bottiglia
        network.add_edge(Edge("D", "E", 25.0, TrackType::DOUBLE, 160.0));
        network.add_edge(Edge("C", "F", 35.0, TrackType::SINGLE, 120.0));
        
        TimetableOptimizer optimizer(network);
        
        // Genera traffico realistico ma irregolare
        
        // Servizio A → E (linea principale) - 10 treni al giorno
        std::vector<int> departure_times_AE = {6, 7, 8, 10, 12, 14, 16, 18, 19, 21};
        for (int i = 0; i < departure_times_AE.size(); i++) {
            auto train = std::make_shared<TrainSchedule>("REG_" + std::to_string(i+1));
            
            int hour = departure_times_AE[i];
            int offset = (i % 3) * 5; // Irregolarità: 0, 5, 10 minuti
            
            train->add_stop(ScheduleStop("A", make_time(hour, offset), make_time(hour, offset), true));
            train->get_stop(0).platform = 1;
            
            train->add_stop(ScheduleStop("B", make_time(hour, offset + 10), make_time(hour, offset + 12), true));
            train->get_stop(1).platform = 1;
            
            train->add_stop(ScheduleStop("C", make_time(hour, offset + 24), make_time(hour, offset + 26), true));
            train->get_stop(2).platform = 1;
            
            train->add_stop(ScheduleStop("D", make_time(hour, offset + 36), make_time(hour, offset + 38), true));
            train->get_stop(3).platform = 1;
            
            train->add_stop(ScheduleStop("E", make_time(hour, offset + 48), make_time(hour, offset + 48), true));
            train->get_stop(4).platform = 1;
            
            optimizer.add_schedule(train);
        }
        
        // Servizio E → A (direzione opposta) - 9 treni
        std::vector<int> departure_times_EA = {7, 8, 9, 11, 13, 15, 17, 19, 20};
        for (int i = 0; i < departure_times_EA.size(); i++) {
            auto train = std::make_shared<TrainSchedule>("REG_" + std::to_string(100+i));
            
            int hour = departure_times_EA[i];
            int offset = (i % 2) * 8; // Irregolarità
            
            train->add_stop(ScheduleStop("E", make_time(hour, offset), make_time(hour, offset), true));
            train->get_stop(0).platform = 2;
            
            train->add_stop(ScheduleStop("D", make_time(hour, offset + 12), make_time(hour, offset + 14), true));
            train->get_stop(1).platform = 2;
            
            train->add_stop(ScheduleStop("C", make_time(hour, offset + 24), make_time(hour, offset + 26), true));
            train->get_stop(2).platform = 2;
            
            train->add_stop(ScheduleStop("B", make_time(hour, offset + 38), make_time(hour, offset + 40), true));
            train->get_stop(3).platform = 2;
            
            train->add_stop(ScheduleStop("A", make_time(hour, offset + 50), make_time(hour, offset + 50), true));
            train->get_stop(4).platform = 2;
            
            optimizer.add_schedule(train);
        }
        
        // Servizio C → F (diramazione) - 6 treni
        std::vector<int> departure_times_CF = {8, 10, 12, 14, 16, 18};
        for (int i = 0; i < departure_times_CF.size(); i++) {
            auto train = std::make_shared<TrainSchedule>("REG_F" + std::to_string(i+1));
            
            int hour = departure_times_CF[i];
            
            train->add_stop(ScheduleStop("C", make_time(hour, 15), make_time(hour, 15), true));
            train->get_stop(0).platform = 3;
            
            train->add_stop(ScheduleStop("F", make_time(hour, 35), make_time(hour, 35), true));
            train->get_stop(1).platform = 1;
            
            optimizer.add_schedule(train);
        }
        
        // Servizio F → C (ritorno) - 6 treni
        std::vector<int> departure_times_FC = {9, 11, 13, 15, 17, 19};
        for (int i = 0; i < departure_times_FC.size(); i++) {
            auto train = std::make_shared<TrainSchedule>("REG_F" + std::to_string(50+i));
            
            int hour = departure_times_FC[i];
            
            train->add_stop(ScheduleStop("F", make_time(hour, 0), make_time(hour, 0), true));
            train->get_stop(0).platform = 2;
            
            train->add_stop(ScheduleStop("C", make_time(hour, 20), make_time(hour, 20), true));
            train->get_stop(1).platform = 4;
            
            optimizer.add_schedule(train);
        }
        
        // Esegui analisi completa
        optimizer.generate_full_report();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Errore: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
