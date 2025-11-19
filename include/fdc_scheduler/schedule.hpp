#ifndef FDC_SCHEDULE_HPP
#define FDC_SCHEDULE_HPP

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>
#include "railway_network.hpp"
#include "train.hpp"

namespace fdc_scheduler {

/**
 * @brief Rappresenta una singola fermata in un orario ferroviario
 * 
 * Gestisce:
 * - Orari di arrivo e partenza
 * - Assegnazione binario
 * - Calcolo tempo di sosta
 */
class ScheduleStop {
private:
    std::string node_id_;                           // ID della stazione
    std::chrono::system_clock::time_point arrival_;  // Orario di arrivo
    std::chrono::system_clock::time_point departure_; // Orario di partenza
    std::optional<int> platform_;                    // Binario assegnato (optional)
    bool is_stop_;                                  // true = fermata, false = transito

public:
    // Costruttori
    ScheduleStop();
    ScheduleStop(const std::string& node_id,
                 const std::chrono::system_clock::time_point& arrival,
                 const std::chrono::system_clock::time_point& departure,
                 bool is_stop = true);

    // Getters
    std::string get_node_id() const { return node_id_; }
    std::chrono::system_clock::time_point get_arrival() const { return arrival_; }
    std::chrono::system_clock::time_point get_departure() const { return departure_; }
    std::optional<int> get_platform() const { return platform_; }
    bool is_stop() const { return is_stop_; }

    // Setters
    void set_node_id(const std::string& node_id) { node_id_ = node_id; }
    void set_arrival(const std::chrono::system_clock::time_point& arrival);
    void set_departure(const std::chrono::system_clock::time_point& departure);
    void set_times(const std::chrono::system_clock::time_point& arrival,
                   const std::chrono::system_clock::time_point& departure);  // Imposta entrambi senza validazione intermedia
    void set_platform(int platform);
    void clear_platform();

    // Metodi di calcolo
    std::chrono::seconds get_dwell_time() const;
    bool is_valid() const; // Controlla che arrivo <= partenza

    // Operatori
    bool operator<(const ScheduleStop& other) const;
    bool operator==(const ScheduleStop& other) const;
};

/**
 * @brief Rappresenta l'orario completo di un treno
 * 
 * Gestisce:
 * - Lista ordinata di fermate
 * - Validazione coerenza oraria
 * - Calcolo distanze e tempi totali
 * - Rilevamento conflitti con altri orari
 */
class TrainSchedule {
private:
    std::string train_id_;                          // ID del treno
    std::string schedule_id_;                       // ID univoco dell'orario
    std::vector<ScheduleStop> stops_;               // Lista fermate ordinate
    std::shared_ptr<RailwayNetwork> network_;       // Riferimento alla rete

public:
    // Costruttori
    TrainSchedule();
    TrainSchedule(const std::string& train_id,
                  const std::string& schedule_id,
                  std::shared_ptr<RailwayNetwork> network);

    // Getters
    std::string get_train_id() const { return train_id_; }
    std::string get_schedule_id() const { return schedule_id_; }
    const std::vector<ScheduleStop>& get_stops() const { return stops_; }
    size_t get_stop_count() const { return stops_.size(); }
    std::shared_ptr<RailwayNetwork> get_network() const { return network_; }

    // Setters
    void set_train_id(const std::string& train_id) { train_id_ = train_id; }
    void set_schedule_id(const std::string& schedule_id) { schedule_id_ = schedule_id; }

    // Gestione fermate
    void add_stop(const ScheduleStop& stop);
    void insert_stop(size_t index, const ScheduleStop& stop);
    void remove_stop(size_t index);
    ScheduleStop& get_stop(size_t index);
    const ScheduleStop& get_stop(size_t index) const;
    void clear_stops();

    // Validazione
    bool is_valid() const;
    bool validate_chronological() const;  // Controlla ordine temporale
    bool validate_network() const;        // Controlla esistenza nodi
    bool validate_platforms() const;      // Controlla disponibilità binari

    // Calcoli
    std::chrono::seconds get_total_duration() const;
    double get_total_distance() const;
    double get_average_speed() const; // km/h

    // Conflict detection
    bool has_conflict_with(const TrainSchedule& other) const;
    bool has_platform_conflict_with(const TrainSchedule& other) const;
    bool has_time_overlap_with(const TrainSchedule& other, const std::string& node_id) const;

    // Utility
    std::vector<std::string> get_node_sequence() const;
    bool visits_node(const std::string& node_id) const;
    std::optional<size_t> find_stop_index(const std::string& node_id) const;
};

/**
 * @brief Statistiche di un orario ferroviario
 */
struct ScheduleStats {
    size_t stop_count;                              // Numero di fermate
    std::chrono::seconds total_duration;            // Durata totale
    double total_distance;                          // Distanza totale (km)
    double average_speed;                           // Velocità media (km/h)
    std::chrono::seconds total_dwell_time;          // Tempo totale di sosta
    size_t platforms_used;                          // Numero di binari utilizzati
    std::vector<std::string> nodes_visited;         // Lista nodi visitati
};

/**
 * @brief Builder per la costruzione di orari ferroviari
 * 
 * Pattern Builder con metodo fluent API per:
 * - Costruzione step-by-step di orari
 * - Calcolo automatico tempi di viaggio
 * - Assegnazione automatica binari
 */
class ScheduleBuilder {
private:
    std::shared_ptr<TrainSchedule> schedule_;       // Orario in costruzione
    std::shared_ptr<Train> train_;                  // Treno associato
    std::chrono::system_clock::time_point current_time_; // Tempo corrente di costruzione
    bool auto_assign_platforms_;                    // Flag per assegnazione automatica

public:
    // Costruttore
    ScheduleBuilder(const std::string& train_id,
                   const std::string& schedule_id,
                   std::shared_ptr<RailwayNetwork> network,
                   std::shared_ptr<Train> train = nullptr);

    // Configurazione
    ScheduleBuilder& set_start_time(const std::chrono::system_clock::time_point& start_time);
    ScheduleBuilder& set_train(std::shared_ptr<Train> train);
    ScheduleBuilder& enable_auto_platform_assignment(bool enable = true);

    // Aggiunta fermate
    ScheduleBuilder& add_stop(const std::string& node_id,
                             const std::chrono::system_clock::time_point& arrival,
                             const std::chrono::system_clock::time_point& departure,
                             bool is_stop = true);

    ScheduleBuilder& add_stop_with_dwell(const std::string& node_id,
                                        const std::chrono::seconds& dwell_time = std::chrono::minutes(2),
                                        bool is_stop = true);

    ScheduleBuilder& add_stop_auto(const std::string& node_id,
                                  const std::chrono::seconds& dwell_time = std::chrono::minutes(2));

    // Calcolo automatico tempi
    ScheduleBuilder& calculate_times_from_network();
    ScheduleBuilder& apply_minimum_dwell_times(const std::chrono::seconds& min_dwell = std::chrono::minutes(2));

    // Gestione binari
    ScheduleBuilder& assign_platforms_automatically();
    ScheduleBuilder& assign_platform_to_stop(size_t stop_index, int platform);

    // Validazione
    bool validate() const;

    // Costruzione finale
    std::shared_ptr<TrainSchedule> build();
    void reset();

    // Getters per ispezione
    std::shared_ptr<TrainSchedule> get_schedule() const { return schedule_; }
    std::chrono::system_clock::time_point get_current_time() const { return current_time_; }
};

/**
 * @brief Gestisce collezioni di orari e rilevamento conflitti
 */
class ScheduleManager {
private:
    std::vector<std::shared_ptr<TrainSchedule>> schedules_;
    std::shared_ptr<RailwayNetwork> network_;

public:
    explicit ScheduleManager(std::shared_ptr<RailwayNetwork> network);

    // Gestione orari
    void add_schedule(std::shared_ptr<TrainSchedule> schedule);
    void remove_schedule(const std::string& schedule_id);
    std::shared_ptr<TrainSchedule> get_schedule(const std::string& schedule_id) const;
    const std::vector<std::shared_ptr<TrainSchedule>>& get_all_schedules() const { return schedules_; }

    // Conflict detection
    std::vector<std::pair<std::string, std::string>> find_all_conflicts() const;
    bool has_any_conflicts() const;
    std::vector<std::string> get_conflicting_schedules(const std::string& schedule_id) const;

    // Query
    std::vector<std::shared_ptr<TrainSchedule>> get_schedules_at_node(const std::string& node_id) const;
    std::vector<std::shared_ptr<TrainSchedule>> get_schedules_in_timerange(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) const;

    // Statistiche
    size_t get_schedule_count() const { return schedules_.size(); }
    void clear() { schedules_.clear(); }
};

// Helper function to convert time_point to ISO 8601 string
std::string time_point_to_iso8601(const std::chrono::system_clock::time_point& tp);

// Helper function to convert ISO 8601 string to time_point
std::chrono::system_clock::time_point iso8601_to_time_point(const std::string& iso_str);

/**
 * @brief JSON serialization for ScheduleStop
 */
void to_json(nlohmann::json& j, const ScheduleStop& stop);

/**
 * @brief JSON deserialization for ScheduleStop
 */
void from_json(const nlohmann::json& j, ScheduleStop& stop);

/**
 * @brief JSON serialization for TrainSchedule
 */
void to_json(nlohmann::json& j, const TrainSchedule& schedule);

/**
 * @brief JSON deserialization for TrainSchedule (requires network pointer)
 */
std::shared_ptr<TrainSchedule> train_schedule_from_json(
    const nlohmann::json& j, 
    std::shared_ptr<RailwayNetwork> network);


// Alias for backward compatibility
using TrainStop = ScheduleStop;

} // namespace fdc_scheduler

#endif // FDC_SCHEDULE_HPP


