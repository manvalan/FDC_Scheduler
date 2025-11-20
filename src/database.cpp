/**
 * @file database.cpp
 * @brief SQLite database implementation
 */

#include <fdc_scheduler/database.hpp>
#include <fdc_scheduler/railway_network.hpp>
#include <fdc_scheduler/train.hpp>
#include <fdc_scheduler/schedule.hpp>
#include <sqlite3.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>

namespace fdc_scheduler {

// =============================================================================
// SQL Schema Definitions
// =============================================================================

static const char* SCHEMA_NETWORKS = R"(
CREATE TABLE IF NOT EXISTS networks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    node_count INTEGER NOT NULL,
    edge_count INTEGER NOT NULL,
    created_at TEXT NOT NULL,
    metadata TEXT
);

CREATE INDEX IF NOT EXISTS idx_networks_name ON networks(name);
CREATE INDEX IF NOT EXISTS idx_networks_created_at ON networks(created_at);
)";

static const char* SCHEMA_NODES = R"(
CREATE TABLE IF NOT EXISTS nodes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    network_id INTEGER NOT NULL,
    node_id TEXT NOT NULL,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    platforms INTEGER DEFAULT 0,
    latitude REAL,
    longitude REAL,
    metadata TEXT,
    FOREIGN KEY (network_id) REFERENCES networks(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_nodes_network ON nodes(network_id);
CREATE INDEX IF NOT EXISTS idx_nodes_node_id ON nodes(node_id);
)";

static const char* SCHEMA_EDGES = R"(
CREATE TABLE IF NOT EXISTS edges (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    network_id INTEGER NOT NULL,
    from_node TEXT NOT NULL,
    to_node TEXT NOT NULL,
    distance REAL NOT NULL,
    track_type TEXT NOT NULL,
    max_speed REAL NOT NULL,
    capacity INTEGER DEFAULT 1,
    metadata TEXT,
    FOREIGN KEY (network_id) REFERENCES networks(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_edges_network ON edges(network_id);
CREATE INDEX IF NOT EXISTS idx_edges_from ON edges(from_node);
CREATE INDEX IF NOT EXISTS idx_edges_to ON edges(to_node);
)";

static const char* SCHEMA_SCHEDULES = R"(
CREATE TABLE IF NOT EXISTS schedules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    network_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    train_count INTEGER NOT NULL,
    created_at TEXT NOT NULL,
    metadata TEXT,
    FOREIGN KEY (network_id) REFERENCES networks(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_schedules_network ON schedules(network_id);
CREATE INDEX IF NOT EXISTS idx_schedules_created_at ON schedules(created_at);
)";

static const char* SCHEMA_TRAINS = R"(
CREATE TABLE IF NOT EXISTS trains (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    schedule_id INTEGER NOT NULL,
    train_id TEXT NOT NULL,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    max_speed REAL NOT NULL,
    priority INTEGER DEFAULT 0,
    metadata TEXT,
    FOREIGN KEY (schedule_id) REFERENCES schedules(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_trains_schedule ON trains(schedule_id);
CREATE INDEX IF NOT EXISTS idx_trains_train_id ON trains(train_id);
)";

static const char* SCHEMA_STOPS = R"(
CREATE TABLE IF NOT EXISTS stops (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    train_id INTEGER NOT NULL,
    stop_sequence INTEGER NOT NULL,
    node_id TEXT NOT NULL,
    arrival_time TEXT NOT NULL,
    departure_time TEXT NOT NULL,
    platform INTEGER,
    dwell_time INTEGER,
    FOREIGN KEY (train_id) REFERENCES trains(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_stops_train ON stops(train_id);
CREATE INDEX IF NOT EXISTS idx_stops_sequence ON stops(stop_sequence);
)";

static const char* SCHEMA_CONFLICTS = R"(
CREATE TABLE IF NOT EXISTS conflicts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    schedule_id INTEGER NOT NULL,
    type TEXT NOT NULL,
    train1_id TEXT NOT NULL,
    train2_id TEXT NOT NULL,
    location TEXT NOT NULL,
    detected_at TEXT NOT NULL,
    resolved BOOLEAN DEFAULT 0,
    details TEXT,
    FOREIGN KEY (schedule_id) REFERENCES schedules(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_conflicts_schedule ON conflicts(schedule_id);
CREATE INDEX IF NOT EXISTS idx_conflicts_resolved ON conflicts(resolved);
CREATE INDEX IF NOT EXISTS idx_conflicts_detected_at ON conflicts(detected_at);
)";

static const char* SCHEMA_RESOLUTIONS = R"(
CREATE TABLE IF NOT EXISTS resolutions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    conflict_id INTEGER NOT NULL,
    strategy TEXT NOT NULL,
    delay_seconds INTEGER NOT NULL,
    quality_score REAL NOT NULL,
    resolved_at TEXT NOT NULL,
    details TEXT,
    FOREIGN KEY (conflict_id) REFERENCES conflicts(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_resolutions_conflict ON resolutions(conflict_id);
CREATE INDEX IF NOT EXISTS idx_resolutions_resolved_at ON resolutions(resolved_at);
)";

static const char* SCHEMA_METRICS = R"(
CREATE TABLE IF NOT EXISTS metrics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    metric_name TEXT NOT NULL,
    value REAL NOT NULL,
    unit TEXT,
    recorded_at TEXT NOT NULL,
    tags TEXT
);

CREATE INDEX IF NOT EXISTS idx_metrics_name ON metrics(metric_name);
CREATE INDEX IF NOT EXISTS idx_metrics_recorded_at ON metrics(recorded_at);
)";

// =============================================================================
// Database Implementation
// =============================================================================

Database::Database(const DatabaseConfig& config)
    : config_(config), db_(nullptr) {
}

Database::~Database() {
    close();
}

Database::Database(Database&& other) noexcept
    : config_(std::move(other.config_))
    , db_(other.db_)
    , last_error_(std::move(other.last_error_)) {
    other.db_ = nullptr;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        close();
        config_ = std::move(other.config_);
        db_ = other.db_;
        last_error_ = std::move(other.last_error_);
        other.db_ = nullptr;
    }
    return *this;
}

bool Database::open() {
    if (is_open()) {
        return true;
    }
    
    int flags = SQLITE_OPEN_READWRITE;
    if (config_.create_if_missing) {
        flags |= SQLITE_OPEN_CREATE;
    }
    if (config_.read_only) {
        flags = SQLITE_OPEN_READONLY;
    }
    
    int rc = sqlite3_open_v2(config_.path.c_str(), &db_, flags, nullptr);
    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    
    // Set busy timeout
    sqlite3_busy_timeout(db_, config_.timeout_ms);
    
    // Enable foreign keys
    if (config_.foreign_keys) {
        execute("PRAGMA foreign_keys = ON");
    }
    
    // Enable WAL mode for better concurrency
    if (config_.wal_mode && !config_.read_only) {
        execute("PRAGMA journal_mode = WAL");
    }
    
    return true;
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::is_open() const {
    return db_ != nullptr;
}

bool Database::initialize_schema() {
    if (!is_open()) {
        last_error_ = "Database not open";
        return false;
    }
    
    // Create all tables
    const char* schemas[] = {
        SCHEMA_NETWORKS,
        SCHEMA_NODES,
        SCHEMA_EDGES,
        SCHEMA_SCHEDULES,
        SCHEMA_TRAINS,
        SCHEMA_STOPS,
        SCHEMA_CONFLICTS,
        SCHEMA_RESOLUTIONS,
        SCHEMA_METRICS
    };
    
    Transaction tx(*this);
    
    for (const char* schema : schemas) {
        if (!execute(schema)) {
            return false;
        }
    }
    
    tx.commit();
    return true;
}

std::string Database::get_last_error() const {
    return last_error_;
}

// =============================================================================
// Transaction Management
// =============================================================================

bool Database::begin_transaction() {
    return execute("BEGIN TRANSACTION");
}

bool Database::commit() {
    return execute("COMMIT");
}

bool Database::rollback() {
    return execute("ROLLBACK");
}

Database::Transaction::Transaction(Database& db)
    : db_(db), committed_(false), rolled_back_(false) {
    db_.begin_transaction();
}

Database::Transaction::~Transaction() {
    if (!committed_ && !rolled_back_) {
        db_.rollback();
    }
}

void Database::Transaction::commit() {
    if (!committed_ && !rolled_back_) {
        db_.commit();
        committed_ = true;
    }
}

void Database::Transaction::rollback() {
    if (!committed_ && !rolled_back_) {
        db_.rollback();
        rolled_back_ = true;
    }
}

// =============================================================================
// Utility Methods
// =============================================================================

bool Database::execute(const std::string& sql) {
    if (!is_open()) {
        last_error_ = "Database not open";
        return false;
    }
    
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    
    if (rc != SQLITE_OK) {
        last_error_ = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

int64_t Database::get_last_insert_id() {
    return sqlite3_last_insert_rowid(db_);
}

std::string Database::to_iso8601(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
    #ifdef _WIN32
    gmtime_s(&tm, &time);
    #else
    gmtime_r(&time, &tm);
    #endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::chrono::system_clock::time_point Database::from_iso8601(const std::string& str) {
    std::tm tm = {};
    std::istringstream ss(str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    auto time = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(time);
}

int64_t Database::get_database_size() const {
    if (config_.path == ":memory:") {
        return 0;
    }
    
    struct stat st;
    if (stat(config_.path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return -1;
}

bool Database::vacuum() {
    return execute("VACUUM");
}

std::map<std::string, int64_t> Database::get_statistics() {
    std::map<std::string, int64_t> stats;
    
    // Helper lambda to get count
    auto get_count = [this](const std::string& table) -> int64_t {
        sqlite3_stmt* stmt;
        std::string sql = "SELECT COUNT(*) FROM " + table;
        
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return -1;
        }
        
        int64_t count = -1;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int64(stmt, 0);
        }
        
        sqlite3_finalize(stmt);
        return count;
    };
    
    stats["networks"] = get_count("networks");
    stats["schedules"] = get_count("schedules");
    stats["trains"] = get_count("trains");
    stats["conflicts"] = get_count("conflicts");
    stats["resolutions"] = get_count("resolutions");
    stats["metrics"] = get_count("metrics");
    stats["database_size_bytes"] = get_database_size();
    
    return stats;
}

// =============================================================================
// Performance Metrics Implementation
// =============================================================================

int64_t Database::record_metric(const std::string& name,
                                 double value,
                                 const std::string& unit,
                                 const std::string& tags) {
    if (!is_open()) {
        last_error_ = "Database not open";
        return -1;
    }
    
    std::string sql = "INSERT INTO metrics (metric_name, value, unit, recorded_at, tags) VALUES (?, ?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return -1;
    }
    
    auto now = std::chrono::system_clock::now();
    std::string timestamp = to_iso8601(now);
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, value);
    sqlite3_bind_text(stmt, 3, unit.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, tags.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return -1;
    }
    
    return get_last_insert_id();
}

std::vector<PerformanceMetric> Database::get_metrics(const std::string& name, int limit) {
    std::vector<PerformanceMetric> metrics;
    
    if (!is_open()) {
        return metrics;
    }
    
    std::string sql = "SELECT id, metric_name, value, unit, recorded_at, tags "
                     "FROM metrics WHERE metric_name = ? "
                     "ORDER BY recorded_at DESC LIMIT ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return metrics;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, limit);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PerformanceMetric m;
        m.id = sqlite3_column_int64(stmt, 0);
        m.metric_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        m.value = sqlite3_column_double(stmt, 2);
        m.unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        
        const char* timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        m.recorded_at = from_iso8601(timestamp);
        
        m.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        
        metrics.push_back(m);
    }
    
    sqlite3_finalize(stmt);
    return metrics;
}

std::vector<PerformanceMetric> Database::get_metrics_in_range(
    const std::string& name,
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) {
    
    std::vector<PerformanceMetric> metrics;
    
    if (!is_open()) {
        return metrics;
    }
    
    std::string sql = "SELECT id, metric_name, value, unit, recorded_at, tags "
                     "FROM metrics WHERE metric_name = ? "
                     "AND recorded_at >= ? AND recorded_at <= ? "
                     "ORDER BY recorded_at DESC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return metrics;
    }
    
    std::string start_str = to_iso8601(start);
    std::string end_str = to_iso8601(end);
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, start_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, end_str.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PerformanceMetric m;
        m.id = sqlite3_column_int64(stmt, 0);
        m.metric_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        m.value = sqlite3_column_double(stmt, 2);
        m.unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        
        const char* timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        m.recorded_at = from_iso8601(timestamp);
        
        m.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        
        metrics.push_back(m);
    }
    
    sqlite3_finalize(stmt);
    return metrics;
}

// =============================================================================
// Stub implementations for network/schedule/conflict persistence
// These will be implemented when integrating with actual data structures
// =============================================================================

int64_t Database::save_network(const RailwayNetwork& network, 
                               const std::string& name,
                               const std::string& description) {
    // Stub implementation
    last_error_ = "save_network: Not yet implemented";
    return -1;
}

std::optional<RailwayNetwork> Database::load_network(int64_t snapshot_id) {
    // Stub implementation
    return std::nullopt;
}

std::vector<NetworkSnapshot> Database::list_networks() {
    // Stub implementation
    return {};
}

bool Database::delete_network(int64_t snapshot_id) {
    // Stub implementation
    return false;
}

int64_t Database::save_schedules(int64_t network_id,
                                 const std::vector<TrainSchedule>& schedules,
                                 const std::string& name) {
    // Stub implementation
    return -1;
}

std::optional<std::vector<TrainSchedule>> Database::load_schedules(int64_t schedule_id) {
    // Stub implementation
    return std::nullopt;
}

std::vector<ScheduleSnapshot> Database::list_schedules() {
    // Stub implementation
    return {};
}

bool Database::delete_schedule(int64_t schedule_id) {
    // Stub implementation
    return false;
}

int64_t Database::save_conflict(int64_t schedule_id, const Conflict& conflict) {
    // Stub implementation
    return -1;
}

std::vector<int64_t> Database::save_conflicts(int64_t schedule_id, 
                                              const std::vector<Conflict>& conflicts) {
    // Stub implementation
    return {};
}

std::vector<ConflictRecord> Database::load_conflicts(int64_t schedule_id) {
    // Stub implementation
    return {};
}

bool Database::mark_conflict_resolved(int64_t conflict_id) {
    // Stub implementation
    return false;
}

int64_t Database::save_resolution(int64_t conflict_id, const ResolutionResult& resolution) {
    // Stub implementation
    return -1;
}

std::vector<ResolutionRecord> Database::load_resolutions(int64_t conflict_id) {
    // Stub implementation
    return {};
}

bool Database::export_to_sql(const std::string& output_path) {
    // Stub implementation
    return false;
}

} // namespace fdc_scheduler
