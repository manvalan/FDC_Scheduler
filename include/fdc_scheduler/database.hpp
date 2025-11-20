/**
 * @file database.hpp
 * @brief SQLite database integration for FDC_Scheduler
 * 
 * Provides persistence layer for:
 * - Railway networks (nodes, edges, platforms)
 * - Train schedules and timetables
 * - Detected conflicts
 * - Conflict resolutions
 * - Configuration history
 * - Performance metrics
 */

#ifndef FDC_SCHEDULER_DATABASE_HPP
#define FDC_SCHEDULER_DATABASE_HPP

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <map>

// Forward declarations to avoid SQLite header dependency
struct sqlite3;
struct sqlite3_stmt;

namespace fdc_scheduler {

// Forward declarations
class RailwayNetwork;
class Train;
class TrainSchedule;
struct Conflict;
struct ResolutionResult;

/**
 * @brief Database connection configuration
 */
struct DatabaseConfig {
    std::string path;                    ///< Database file path
    bool create_if_missing = true;       ///< Create database if doesn't exist
    bool read_only = false;              ///< Open in read-only mode
    int timeout_ms = 5000;               ///< Busy timeout in milliseconds
    bool foreign_keys = true;            ///< Enable foreign key constraints
    bool wal_mode = true;                ///< Use Write-Ahead Logging
    
    DatabaseConfig() = default;
    explicit DatabaseConfig(const std::string& db_path) : path(db_path) {}
};

/**
 * @brief Represents a saved network snapshot
 */
struct NetworkSnapshot {
    int64_t id;
    std::string name;
    std::string description;
    int node_count;
    int edge_count;
    std::chrono::system_clock::time_point created_at;
    std::string metadata;  // JSON metadata
};

/**
 * @brief Represents a saved schedule
 */
struct ScheduleSnapshot {
    int64_t id;
    int64_t network_id;
    std::string name;
    int train_count;
    std::chrono::system_clock::time_point created_at;
    std::string metadata;
};

/**
 * @brief Represents a saved conflict
 */
struct ConflictRecord {
    int64_t id;
    int64_t schedule_id;
    std::string type;
    std::string train1_id;
    std::string train2_id;
    std::string location;
    std::chrono::system_clock::time_point detected_at;
    bool resolved;
    std::string details;  // JSON details
};

/**
 * @brief Represents a conflict resolution
 */
struct ResolutionRecord {
    int64_t id;
    int64_t conflict_id;
    std::string strategy;
    int delay_seconds;
    double quality_score;
    std::chrono::system_clock::time_point resolved_at;
    std::string details;  // JSON details
};

/**
 * @brief Performance metrics record
 */
struct PerformanceMetric {
    int64_t id;
    std::string metric_name;
    double value;
    std::string unit;
    std::chrono::system_clock::time_point recorded_at;
    std::string tags;  // JSON tags
};

/**
 * @brief SQLite database interface for FDC_Scheduler
 * 
 * Thread-safe database operations for persisting scheduler data.
 * Uses SQLite for lightweight, embedded storage.
 */
class Database {
public:
    /**
     * @brief Construct database connection
     * @param config Database configuration
     */
    explicit Database(const DatabaseConfig& config = DatabaseConfig(":memory:"));
    
    /**
     * @brief Destructor - closes database connection
     */
    ~Database();
    
    // Non-copyable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    // Movable
    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;
    
    // =========================================================================
    // Connection Management
    // =========================================================================
    
    /**
     * @brief Open database connection
     * @return true if successful
     */
    bool open();
    
    /**
     * @brief Close database connection
     */
    void close();
    
    /**
     * @brief Check if database is open
     */
    bool is_open() const;
    
    /**
     * @brief Initialize database schema
     * Creates all tables if they don't exist
     * @return true if successful
     */
    bool initialize_schema();
    
    /**
     * @brief Get last error message
     */
    std::string get_last_error() const;
    
    // =========================================================================
    // Transaction Management
    // =========================================================================
    
    /**
     * @brief Begin transaction
     */
    bool begin_transaction();
    
    /**
     * @brief Commit transaction
     */
    bool commit();
    
    /**
     * @brief Rollback transaction
     */
    bool rollback();
    
    /**
     * @brief RAII transaction guard
     */
    class Transaction {
    public:
        explicit Transaction(Database& db);
        ~Transaction();
        
        void commit();
        void rollback();
        
    private:
        Database& db_;
        bool committed_;
        bool rolled_back_;
    };
    
    // =========================================================================
    // Network Persistence
    // =========================================================================
    
    /**
     * @brief Save railway network to database
     * @param network Network to save
     * @param name Snapshot name
     * @param description Optional description
     * @return Network snapshot ID, or -1 on error
     */
    int64_t save_network(const RailwayNetwork& network, 
                         const std::string& name,
                         const std::string& description = "");
    
    /**
     * @brief Load railway network from database
     * @param snapshot_id Network snapshot ID
     * @return Network if found
     */
    std::optional<RailwayNetwork> load_network(int64_t snapshot_id);
    
    /**
     * @brief List all network snapshots
     */
    std::vector<NetworkSnapshot> list_networks();
    
    /**
     * @brief Delete network snapshot
     * @param snapshot_id Network snapshot ID
     * @return true if successful
     */
    bool delete_network(int64_t snapshot_id);
    
    // =========================================================================
    // Schedule Persistence
    // =========================================================================
    
    /**
     * @brief Save train schedules to database
     * @param network_id Associated network ID
     * @param schedules Vector of train schedules
     * @param name Schedule set name
     * @return Schedule snapshot ID, or -1 on error
     */
    int64_t save_schedules(int64_t network_id,
                           const std::vector<TrainSchedule>& schedules,
                           const std::string& name);
    
    /**
     * @brief Load train schedules from database
     * @param schedule_id Schedule snapshot ID
     * @return Schedules if found
     */
    std::optional<std::vector<TrainSchedule>> load_schedules(int64_t schedule_id);
    
    /**
     * @brief List all schedule snapshots
     */
    std::vector<ScheduleSnapshot> list_schedules();
    
    /**
     * @brief Delete schedule snapshot
     */
    bool delete_schedule(int64_t schedule_id);
    
    // =========================================================================
    // Conflict Persistence
    // =========================================================================
    
    /**
     * @brief Save conflict to database
     * @param schedule_id Associated schedule ID
     * @param conflict Conflict to save
     * @return Conflict ID, or -1 on error
     */
    int64_t save_conflict(int64_t schedule_id, const Conflict& conflict);
    
    /**
     * @brief Save multiple conflicts
     */
    std::vector<int64_t> save_conflicts(int64_t schedule_id, 
                                        const std::vector<Conflict>& conflicts);
    
    /**
     * @brief Load conflicts for a schedule
     */
    std::vector<ConflictRecord> load_conflicts(int64_t schedule_id);
    
    /**
     * @brief Mark conflict as resolved
     */
    bool mark_conflict_resolved(int64_t conflict_id);
    
    // =========================================================================
    // Resolution Persistence
    // =========================================================================
    
    /**
     * @brief Save conflict resolution
     * @param conflict_id Associated conflict ID
     * @param resolution Resolution result
     * @return Resolution ID, or -1 on error
     */
    int64_t save_resolution(int64_t conflict_id, const ResolutionResult& resolution);
    
    /**
     * @brief Load resolutions for a conflict
     */
    std::vector<ResolutionRecord> load_resolutions(int64_t conflict_id);
    
    // =========================================================================
    // Performance Metrics
    // =========================================================================
    
    /**
     * @brief Record performance metric
     * @param name Metric name
     * @param value Metric value
     * @param unit Unit of measurement
     * @param tags Optional tags (JSON)
     * @return Metric ID, or -1 on error
     */
    int64_t record_metric(const std::string& name,
                          double value,
                          const std::string& unit = "",
                          const std::string& tags = "{}");
    
    /**
     * @brief Get metrics by name
     * @param name Metric name
     * @param limit Maximum number of results
     * @return Vector of metrics
     */
    std::vector<PerformanceMetric> get_metrics(const std::string& name, int limit = 100);
    
    /**
     * @brief Get metrics in time range
     */
    std::vector<PerformanceMetric> get_metrics_in_range(
        const std::string& name,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end);
    
    // =========================================================================
    // Utility Methods
    // =========================================================================
    
    /**
     * @brief Execute raw SQL query
     * @param sql SQL statement
     * @return true if successful
     */
    bool execute(const std::string& sql);
    
    /**
     * @brief Get database file size in bytes
     */
    int64_t get_database_size() const;
    
    /**
     * @brief Vacuum database (compact and optimize)
     */
    bool vacuum();
    
    /**
     * @brief Export database to SQL file
     */
    bool export_to_sql(const std::string& output_path);
    
    /**
     * @brief Get database statistics
     */
    std::map<std::string, int64_t> get_statistics();

private:
    DatabaseConfig config_;
    sqlite3* db_;
    std::string last_error_;
    
    // Helper methods
    bool create_tables();
    int64_t get_last_insert_id();
    std::string to_iso8601(const std::chrono::system_clock::time_point& tp);
    std::chrono::system_clock::time_point from_iso8601(const std::string& str);
};

} // namespace fdc_scheduler

#endif // FDC_SCHEDULER_DATABASE_HPP
