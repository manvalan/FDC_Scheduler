#include "fdc_scheduler/schedule.hpp"
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace fdc_scheduler {

// ============================================================================
// ScheduleStop Implementation
// ============================================================================

ScheduleStop::ScheduleStop()
    : node_id_(""),
      arrival_(std::chrono::system_clock::now()),
      departure_(std::chrono::system_clock::now()),
      platform_(std::nullopt),
      is_stop_(true) {
}

ScheduleStop::ScheduleStop(const std::string& node_id,
                           const std::chrono::system_clock::time_point& arrival,
                           const std::chrono::system_clock::time_point& departure,
                           bool is_stop)
    : node_id_(node_id),
      arrival_(arrival),
      departure_(departure),
      platform_(std::nullopt),
      is_stop_(is_stop) {
    
    if (!is_valid()) {
        throw std::invalid_argument("ScheduleStop: departure must be >= arrival");
    }
}

void ScheduleStop::set_arrival(const std::chrono::system_clock::time_point& arrival) {
    arrival_ = arrival;
    if (!is_valid()) {
        throw std::invalid_argument("ScheduleStop: arrival cannot be after departure");
    }
}

void ScheduleStop::set_departure(const std::chrono::system_clock::time_point& departure) {
    departure_ = departure;
    if (!is_valid()) {
        throw std::invalid_argument("ScheduleStop: departure must be >= arrival");
    }
}

void ScheduleStop::set_times(const std::chrono::system_clock::time_point& arrival,
                              const std::chrono::system_clock::time_point& departure) {
    // Imposta entrambi i tempi SENZA validazione intermedia
    arrival_ = arrival;
    departure_ = departure;
    // Valida SOLO alla fine
    if (!is_valid()) {
        throw std::invalid_argument("ScheduleStop: departure must be >= arrival");
    }
}

void ScheduleStop::set_platform(int platform) {
    if (platform < 1) {
        throw std::invalid_argument("ScheduleStop: platform number must be >= 1");
    }
    platform_ = platform;
}

void ScheduleStop::clear_platform() {
    platform_ = std::nullopt;
}

std::chrono::seconds ScheduleStop::get_dwell_time() const {
    return std::chrono::duration_cast<std::chrono::seconds>(departure_ - arrival_);
}

bool ScheduleStop::is_valid() const {
    return arrival_ <= departure_;
}

bool ScheduleStop::operator<(const ScheduleStop& other) const {
    return arrival_ < other.arrival_;
}

bool ScheduleStop::operator==(const ScheduleStop& other) const {
    return node_id_ == other.node_id_ &&
           arrival_ == other.arrival_ &&
           departure_ == other.departure_ &&
           platform_ == other.platform_ &&
           is_stop_ == other.is_stop_;
}

// ============================================================================
// TrainSchedule Implementation
// ============================================================================

TrainSchedule::TrainSchedule()
    : train_id_(""),
      schedule_id_(""),
      network_(nullptr) {
}

TrainSchedule::TrainSchedule(const std::string& train_id,
                             const std::string& schedule_id,
                             std::shared_ptr<RailwayNetwork> network)
    : train_id_(train_id),
      schedule_id_(schedule_id),
      network_(network) {
    
    if (!network) {
        throw std::invalid_argument("TrainSchedule: network cannot be null");
    }
}

void TrainSchedule::add_stop(const ScheduleStop& stop) {
    stops_.push_back(stop);
}

void TrainSchedule::insert_stop(size_t index, const ScheduleStop& stop) {
    if (index > stops_.size()) {
        throw std::out_of_range("TrainSchedule: insert index out of range");
    }
    stops_.insert(stops_.begin() + index, stop);
}

void TrainSchedule::remove_stop(size_t index) {
    if (index >= stops_.size()) {
        throw std::out_of_range("TrainSchedule: remove index out of range");
    }
    stops_.erase(stops_.begin() + index);
}

ScheduleStop& TrainSchedule::get_stop(size_t index) {
    if (index >= stops_.size()) {
        throw std::out_of_range("TrainSchedule: stop index out of range");
    }
    return stops_[index];
}

const ScheduleStop& TrainSchedule::get_stop(size_t index) const {
    if (index >= stops_.size()) {
        throw std::out_of_range("TrainSchedule: stop index out of range");
    }
    return stops_[index];
}

void TrainSchedule::clear_stops() {
    stops_.clear();
}

bool TrainSchedule::is_valid() const {
    return validate_chronological() && validate_network() && validate_platforms();
}

bool TrainSchedule::validate_chronological() const {
    if (stops_.size() < 2) {
        return true; // Single stop or empty is valid
    }
    
    for (size_t i = 1; i < stops_.size(); ++i) {
        if (stops_[i].get_arrival() < stops_[i-1].get_departure()) {
            return false; // Arrival at next stop before departure from previous
        }
    }
    
    return true;
}

bool TrainSchedule::validate_network() const {
    if (!network_) {
        return false;
    }
    
    for (const auto& stop : stops_) {
        if (!network_->has_node(stop.get_node_id())) {
            return false; // Node doesn't exist in network
        }
    }
    
    return true;
}

bool TrainSchedule::validate_platforms() const {
    if (!network_) {
        return false;
    }
    
    for (const auto& stop : stops_) {
        if (stop.get_platform().has_value()) {
            auto node = network_->get_node(stop.get_node_id());
            if (!node) {
                return false;
            }
            
            int platform = stop.get_platform().value();
            if (platform < 1 || platform > node->get_platforms()) {
                return false; // Platform number out of range
            }
        }
    }
    
    return true;
}

std::chrono::seconds TrainSchedule::get_total_duration() const {
    if (stops_.empty()) {
        return std::chrono::seconds(0);
    }
    
    if (stops_.size() == 1) {
        return stops_[0].get_dwell_time();
    }
    
    auto start = stops_.front().get_arrival();
    auto end = stops_.back().get_departure();
    
    return std::chrono::duration_cast<std::chrono::seconds>(end - start);
}

double TrainSchedule::get_total_distance() const {
    if (!network_ || stops_.size() < 2) {
        return 0.0;
    }
    
    double total = 0.0;
    
    for (size_t i = 1; i < stops_.size(); ++i) {
        const auto& from_node = stops_[i-1].get_node_id();
        const auto& to_node = stops_[i].get_node_id();
        
        auto edge = network_->get_edge(from_node, to_node);
        if (edge) {
            total += edge->get_distance();
        }
    }
    
    return total;
}

double TrainSchedule::get_average_speed() const {
    auto duration_sec = get_total_duration().count();
    if (duration_sec == 0) {
        return 0.0;
    }
    
    double distance_km = get_total_distance();
    double duration_hours = duration_sec / 3600.0;
    
    return distance_km / duration_hours;
}

bool TrainSchedule::has_conflict_with(const TrainSchedule& other) const {
    return has_platform_conflict_with(other);
}

bool TrainSchedule::has_platform_conflict_with(const TrainSchedule& other) const {
    // Check each stop in this schedule
    for (const auto& stop : stops_) {
        if (!stop.get_platform().has_value()) {
            continue; // Skip stops without platform assignment
        }
        
        // Check if other schedule has overlapping time at same node+platform
        for (const auto& other_stop : other.get_stops()) {
            if (!other_stop.get_platform().has_value()) {
                continue;
            }
            
            // Same node and platform?
            if (stop.get_node_id() == other_stop.get_node_id() &&
                stop.get_platform() == other_stop.get_platform()) {
                
                // Check time overlap: [arrival1, departure1] overlaps [arrival2, departure2]?
                auto arr1 = stop.get_arrival();
                auto dep1 = stop.get_departure();
                auto arr2 = other_stop.get_arrival();
                auto dep2 = other_stop.get_departure();
                
                // Overlap if: arr1 < dep2 AND arr2 < dep1
                if (arr1 < dep2 && arr2 < dep1) {
                    return true; // Conflict found
                }
            }
        }
    }
    
    return false;
}

bool TrainSchedule::has_time_overlap_with(const TrainSchedule& other, const std::string& node_id) const {
    // Find stops at the specified node in both schedules
    auto this_stop_it = std::find_if(stops_.begin(), stops_.end(),
        [&node_id](const ScheduleStop& s) { return s.get_node_id() == node_id; });
    
    if (this_stop_it == stops_.end()) {
        return false; // This schedule doesn't visit the node
    }
    
    auto other_stop_it = std::find_if(other.get_stops().begin(), other.get_stops().end(),
        [&node_id](const ScheduleStop& s) { return s.get_node_id() == node_id; });
    
    if (other_stop_it == other.get_stops().end()) {
        return false; // Other schedule doesn't visit the node
    }
    
    // Check time overlap
    auto arr1 = this_stop_it->get_arrival();
    auto dep1 = this_stop_it->get_departure();
    auto arr2 = other_stop_it->get_arrival();
    auto dep2 = other_stop_it->get_departure();
    
    return (arr1 < dep2 && arr2 < dep1);
}

std::vector<std::string> TrainSchedule::get_node_sequence() const {
    std::vector<std::string> sequence;
    sequence.reserve(stops_.size());
    
    for (const auto& stop : stops_) {
        sequence.push_back(stop.get_node_id());
    }
    
    return sequence;
}

bool TrainSchedule::visits_node(const std::string& node_id) const {
    return std::any_of(stops_.begin(), stops_.end(),
        [&node_id](const ScheduleStop& s) { return s.get_node_id() == node_id; });
}

std::optional<size_t> TrainSchedule::find_stop_index(const std::string& node_id) const {
    auto it = std::find_if(stops_.begin(), stops_.end(),
        [&node_id](const ScheduleStop& s) { return s.get_node_id() == node_id; });
    
    if (it != stops_.end()) {
        return std::distance(stops_.begin(), it);
    }
    
    return std::nullopt;
}

// ============================================================================
// ScheduleBuilder Implementation
// ============================================================================

ScheduleBuilder::ScheduleBuilder(const std::string& train_id,
                                 const std::string& schedule_id,
                                 std::shared_ptr<RailwayNetwork> network,
                                 std::shared_ptr<Train> train)
    : schedule_(std::make_shared<TrainSchedule>(train_id, schedule_id, network)),
      train_(train),
      current_time_(std::chrono::system_clock::now()),
      auto_assign_platforms_(false) {
}

ScheduleBuilder& ScheduleBuilder::set_start_time(const std::chrono::system_clock::time_point& start_time) {
    current_time_ = start_time;
    return *this;
}

ScheduleBuilder& ScheduleBuilder::set_train(std::shared_ptr<Train> train) {
    train_ = train;
    return *this;
}

ScheduleBuilder& ScheduleBuilder::enable_auto_platform_assignment(bool enable) {
    auto_assign_platforms_ = enable;
    return *this;
}

ScheduleBuilder& ScheduleBuilder::add_stop(const std::string& node_id,
                                          const std::chrono::system_clock::time_point& arrival,
                                          const std::chrono::system_clock::time_point& departure,
                                          bool is_stop) {
    ScheduleStop stop(node_id, arrival, departure, is_stop);
    schedule_->add_stop(stop);
    current_time_ = departure;
    return *this;
}

ScheduleBuilder& ScheduleBuilder::add_stop_with_dwell(const std::string& node_id,
                                                     const std::chrono::seconds& dwell_time,
                                                     bool is_stop) {
    auto arrival = current_time_;
    auto departure = arrival + dwell_time;
    
    ScheduleStop stop(node_id, arrival, departure, is_stop);
    schedule_->add_stop(stop);
    current_time_ = departure;
    
    return *this;
}

ScheduleBuilder& ScheduleBuilder::add_stop_auto(const std::string& node_id,
                                               const std::chrono::seconds& dwell_time) {
    // Calculate travel time from previous stop
    if (schedule_->get_stop_count() > 0) {
        const auto& prev_stop = schedule_->get_stop(schedule_->get_stop_count() - 1);
        const auto& prev_node = prev_stop.get_node_id();
        
        // Get edge and calculate travel time
        auto network = schedule_->get_network();
        if (network) {
            auto edge = network->get_edge(prev_node, node_id);
            if (edge && train_) {
                // Calculate travel time based on edge and train speed
                double distance_km = edge->get_distance();
                double speed_kmh = train_->get_max_speed();
                double travel_hours = distance_km / speed_kmh;
                auto travel_seconds = std::chrono::seconds(static_cast<long long>(travel_hours * 3600));
                
                current_time_ += travel_seconds;
            }
        }
    }
    
    return add_stop_with_dwell(node_id, dwell_time, true);
}

ScheduleBuilder& ScheduleBuilder::calculate_times_from_network() {
    if (!train_ || schedule_->get_stop_count() < 2) {
        return *this;
    }
    
    auto network = schedule_->get_network();
    if (!network) {
        return *this;
    }
    
    // Recalculate times for all stops except the first
    auto start_time = schedule_->get_stop(0).get_arrival();
    auto current = start_time;
    
    for (size_t i = 1; i < schedule_->get_stop_count(); ++i) {
        auto& prev_stop = schedule_->get_stop(i-1);
        auto& curr_stop = schedule_->get_stop(i);
        
        // Add previous stop's dwell time
        current = prev_stop.get_departure();
        
        // Calculate travel time
        auto edge = network->get_edge(prev_stop.get_node_id(), curr_stop.get_node_id());
        if (edge) {
            double distance_km = edge->get_distance();
            double speed_kmh = train_->get_max_speed();
            double travel_hours = distance_km / speed_kmh;
            auto travel_seconds = std::chrono::seconds(static_cast<long long>(travel_hours * 3600));
            
            current += travel_seconds;
        }
        
        // Update arrival time
        curr_stop.set_arrival(current);
        
        // Keep original dwell time
        auto dwell = curr_stop.get_dwell_time();
        curr_stop.set_departure(current + dwell);
    }
    
    return *this;
}

ScheduleBuilder& ScheduleBuilder::apply_minimum_dwell_times(const std::chrono::seconds& min_dwell) {
    for (size_t i = 0; i < schedule_->get_stop_count(); ++i) {
        auto& stop = schedule_->get_stop(i);
        
        if (stop.get_dwell_time() < min_dwell) {
            auto arrival = stop.get_arrival();
            stop.set_departure(arrival + min_dwell);
        }
    }
    
    return *this;
}

ScheduleBuilder& ScheduleBuilder::assign_platforms_automatically() {
    auto network = schedule_->get_network();
    if (!network) {
        return *this;
    }
    
    // Simple strategy: assign platform 1 to all stops that have platforms
    for (size_t i = 0; i < schedule_->get_stop_count(); ++i) {
        auto& stop = schedule_->get_stop(i);
        auto node = network->get_node(stop.get_node_id());
        
        if (node && node->get_platforms() > 0) {
            stop.set_platform(1); // Simple assignment
        }
    }
    
    return *this;
}

ScheduleBuilder& ScheduleBuilder::assign_platform_to_stop(size_t stop_index, int platform) {
    if (stop_index >= schedule_->get_stop_count()) {
        throw std::out_of_range("ScheduleBuilder: stop index out of range");
    }
    
    auto& stop = schedule_->get_stop(stop_index);
    stop.set_platform(platform);
    
    return *this;
}

bool ScheduleBuilder::validate() const {
    return schedule_->is_valid();
}

std::shared_ptr<TrainSchedule> ScheduleBuilder::build() {
    if (!validate()) {
        throw std::runtime_error("ScheduleBuilder: cannot build invalid schedule");
    }
    
    if (auto_assign_platforms_) {
        const_cast<ScheduleBuilder*>(this)->assign_platforms_automatically();
    }
    
    return schedule_;
}

void ScheduleBuilder::reset() {
    auto train_id = schedule_->get_train_id();
    auto schedule_id = schedule_->get_schedule_id();
    auto network = schedule_->get_network();
    
    schedule_ = std::make_shared<TrainSchedule>(train_id, schedule_id, network);
    current_time_ = std::chrono::system_clock::now();
}

// ============================================================================
// ScheduleManager Implementation
// ============================================================================

ScheduleManager::ScheduleManager(std::shared_ptr<RailwayNetwork> network)
    : network_(network) {
}

void ScheduleManager::add_schedule(std::shared_ptr<TrainSchedule> schedule) {
    if (!schedule) {
        throw std::invalid_argument("ScheduleManager: schedule cannot be null");
    }
    schedules_.push_back(schedule);
}

void ScheduleManager::remove_schedule(const std::string& schedule_id) {
    schedules_.erase(
        std::remove_if(schedules_.begin(), schedules_.end(),
            [&schedule_id](const auto& s) { return s->get_schedule_id() == schedule_id; }),
        schedules_.end()
    );
}

std::shared_ptr<TrainSchedule> ScheduleManager::get_schedule(const std::string& schedule_id) const {
    auto it = std::find_if(schedules_.begin(), schedules_.end(),
        [&schedule_id](const auto& s) { return s->get_schedule_id() == schedule_id; });
    
    return (it != schedules_.end()) ? *it : nullptr;
}

std::vector<std::pair<std::string, std::string>> ScheduleManager::find_all_conflicts() const {
    std::vector<std::pair<std::string, std::string>> conflicts;
    
    for (size_t i = 0; i < schedules_.size(); ++i) {
        for (size_t j = i + 1; j < schedules_.size(); ++j) {
            if (schedules_[i]->has_conflict_with(*schedules_[j])) {
                conflicts.emplace_back(schedules_[i]->get_schedule_id(),
                                     schedules_[j]->get_schedule_id());
            }
        }
    }
    
    return conflicts;
}

bool ScheduleManager::has_any_conflicts() const {
    return !find_all_conflicts().empty();
}

std::vector<std::string> ScheduleManager::get_conflicting_schedules(const std::string& schedule_id) const {
    std::vector<std::string> conflicts;
    
    auto schedule = get_schedule(schedule_id);
    if (!schedule) {
        return conflicts;
    }
    
    for (const auto& other : schedules_) {
        if (other->get_schedule_id() != schedule_id &&
            schedule->has_conflict_with(*other)) {
            conflicts.push_back(other->get_schedule_id());
        }
    }
    
    return conflicts;
}

std::vector<std::shared_ptr<TrainSchedule>> ScheduleManager::get_schedules_at_node(const std::string& node_id) const {
    std::vector<std::shared_ptr<TrainSchedule>> result;
    
    for (const auto& schedule : schedules_) {
        if (schedule->visits_node(node_id)) {
            result.push_back(schedule);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<TrainSchedule>> ScheduleManager::get_schedules_in_timerange(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) const {
    
    std::vector<std::shared_ptr<TrainSchedule>> result;
    
    for (const auto& schedule : schedules_) {
        if (schedule->get_stop_count() == 0) {
            continue;
        }
        
        auto first_arrival = schedule->get_stop(0).get_arrival();
        auto last_departure = schedule->get_stop(schedule->get_stop_count() - 1).get_departure();
        
        // Check if schedule overlaps with time range
        if (first_arrival < end && last_departure > start) {
            result.push_back(schedule);
        }
    }
    
    return result;
}

// ============================================================================
// JSON Serialization
// ============================================================================

std::string time_point_to_iso8601(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::gmtime(&time_t);
    
    char buffer[25];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);
    return std::string(buffer);
}

std::chrono::system_clock::time_point iso8601_to_time_point(const std::string& iso_str) {
    std::tm tm = {};
    std::istringstream ss(iso_str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    if (ss.fail()) {
        throw std::invalid_argument("Invalid ISO 8601 time format: " + iso_str);
    }
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

void to_json(nlohmann::json& j, const ScheduleStop& stop) {
    j = nlohmann::json{
        {"node_id", stop.get_node_id()},
        {"arrival", time_point_to_iso8601(stop.get_arrival())},
        {"departure", time_point_to_iso8601(stop.get_departure())},
        {"is_stop", stop.is_stop()}
    };
    
    if (stop.get_platform().has_value()) {
        j["platform"] = stop.get_platform().value();
    } else {
        j["platform"] = nullptr;
    }
}

void from_json(const nlohmann::json& j, ScheduleStop& stop) {
    std::string node_id = j.at("node_id").get<std::string>();
    std::string arrival_str = j.at("arrival").get<std::string>();
    std::string departure_str = j.at("departure").get<std::string>();
    bool is_stop = j.at("is_stop").get<bool>();
    
    auto arrival = iso8601_to_time_point(arrival_str);
    auto departure = iso8601_to_time_point(departure_str);
    
    stop = ScheduleStop(node_id, arrival, departure, is_stop);
    
    if (j.contains("platform") && !j["platform"].is_null()) {
        stop.set_platform(j["platform"].get<int>());
    }
}

void to_json(nlohmann::json& j, const TrainSchedule& schedule) {
    j = nlohmann::json{
        {"train_id", schedule.get_train_id()},
        {"schedule_id", schedule.get_schedule_id()},
        {"stops", schedule.get_stops()}
    };
}

std::shared_ptr<TrainSchedule> train_schedule_from_json(
    const nlohmann::json& j, 
    std::shared_ptr<RailwayNetwork> network) {
    
    std::string train_id = j.at("train_id").get<std::string>();
    std::string schedule_id = j.at("schedule_id").get<std::string>();
    
    auto schedule = std::make_shared<TrainSchedule>(train_id, schedule_id, network);
    
    if (j.contains("stops")) {
        for (const auto& stop_json : j["stops"]) {
            ScheduleStop stop;
            from_json(stop_json, stop);
            schedule->add_stop(stop);
        }
    }
    
    return schedule;
}

} // namespace fdc_scheduler
