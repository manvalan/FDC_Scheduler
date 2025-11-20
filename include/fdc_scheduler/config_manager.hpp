#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <stdexcept>
#include <variant>

namespace fdc_scheduler {

/**
 * @brief Configuration value types
 */
using ConfigValue = std::variant<
    std::string,
    int,
    double,
    bool,
    std::vector<std::string>
>;

/**
 * @brief Configuration section (nested map)
 */
class ConfigSection {
public:
    ConfigSection() = default;
    explicit ConfigSection(const std::string& name) : name_(name) {}
    
    // Getters with type conversion
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    int get_int(const std::string& key, int default_value = 0) const;
    double get_double(const std::string& key, double default_value = 0.0) const;
    bool get_bool(const std::string& key, bool default_value = false) const;
    std::vector<std::string> get_string_list(const std::string& key) const;
    
    // Setters
    void set(const std::string& key, const ConfigValue& value);
    void set_string(const std::string& key, const std::string& value);
    void set_int(const std::string& key, int value);
    void set_double(const std::string& key, double value);
    void set_bool(const std::string& key, bool value);
    void set_string_list(const std::string& key, const std::vector<std::string>& value);
    
    // Check existence
    bool has(const std::string& key) const;
    
    // Subsections
    ConfigSection& add_section(const std::string& name);
    const ConfigSection& get_section(const std::string& name) const;
    ConfigSection& get_section(const std::string& name);
    bool has_section(const std::string& name) const;
    
    // Iteration
    const std::map<std::string, ConfigValue>& get_values() const { return values_; }
    const std::map<std::string, ConfigSection>& get_sections() const { return sections_; }
    
    const std::string& name() const { return name_; }
    
private:
    std::string name_;
    std::map<std::string, ConfigValue> values_;
    std::map<std::string, ConfigSection> sections_;
};

/**
 * @brief Configuration manager for FDC_Scheduler
 * 
 * Supports multiple configuration sources:
 * - YAML files
 * - TOML files  
 * - JSON files (via nlohmann/json)
 * - Environment variables
 * - Command-line arguments
 * 
 * Features:
 * - Hierarchical configuration structure
 * - Type-safe value access
 * - Default values
 * - Configuration validation
 * - Hot reload support
 * - Environment variable override
 */
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;
    
    // Load configuration from file
    bool load_from_file(const std::string& filename);
    bool load_from_yaml(const std::string& filename);
    bool load_from_toml(const std::string& filename);
    bool load_from_json(const std::string& filename);
    
    // Save configuration to file
    bool save_to_file(const std::string& filename) const;
    bool save_to_yaml(const std::string& filename) const;
    bool save_to_toml(const std::string& filename) const;
    bool save_to_json(const std::string& filename) const;
    
    // Load from environment variables
    void load_from_env(const std::string& prefix = "FDC_");
    
    // Root section access
    const ConfigSection& root() const { return root_; }
    ConfigSection& root() { return root_; }
    
    // Direct access helpers
    std::string get_string(const std::string& path, const std::string& default_value = "") const;
    int get_int(const std::string& path, int default_value = 0) const;
    double get_double(const std::string& path, double default_value = 0.0) const;
    bool get_bool(const std::string& path, bool default_value = false) const;
    
    void set_string(const std::string& path, const std::string& value);
    void set_int(const std::string& path, int value);
    void set_double(const std::string& path, double value);
    void set_bool(const std::string& path, bool value);
    
    // Validation
    bool validate() const;
    std::vector<std::string> get_validation_errors() const;
    
    // Hot reload
    void enable_hot_reload(bool enable = true);
    bool check_and_reload();
    
    // Merge configurations
    void merge(const ConfigManager& other);
    
    // Clear all configuration
    void clear();
    
    // Error handling
    const std::string& get_last_error() const { return last_error_; }
    
private:
    ConfigSection root_;
    std::string last_loaded_file_;
    time_t last_loaded_time_;
    bool hot_reload_enabled_;
    mutable std::string last_error_;
    
    // Path parsing (dot notation: "server.host" -> ["server", "host"])
    std::vector<std::string> parse_path(const std::string& path) const;
    
    // Get nested section from path
    const ConfigSection* get_section_by_path(const std::string& path) const;
    ConfigSection* get_section_by_path(const std::string& path);
};

/**
 * @brief Default configuration generator
 */
class DefaultConfig {
public:
    // Generate default configuration for various components
    static ConfigSection generate_server_config();
    static ConfigSection generate_network_config();
    static ConfigSection generate_scheduler_config();
    static ConfigSection generate_optimizer_config();
    static ConfigSection generate_logging_config();
    
    // Complete default configuration
    static ConfigManager generate_default();
};

/**
 * @brief Configuration builder for fluent API
 */
class ConfigBuilder {
public:
    ConfigBuilder() = default;
    
    ConfigBuilder& server(const std::string& host, int port);
    ConfigBuilder& authentication(bool enabled, const std::string& jwt_secret = "");
    ConfigBuilder& rate_limit(int per_minute, int per_hour);
    ConfigBuilder& database(const std::string& type, const std::string& path);
    ConfigBuilder& logging(const std::string& level, const std::string& file = "");
    ConfigBuilder& optimizer(const std::string& mode, double energy_weight = 0.5);
    
    ConfigManager build() const;
    
private:
    ConfigManager config_;
};

} // namespace fdc_scheduler
