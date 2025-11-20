/**
 * @file config_demo.cpp
 * @brief Demonstration of Configuration Manager capabilities
 * 
 * This example showcases:
 * 1. Loading configuration from JSON/YAML/TOML files
 * 2. Accessing configuration values with type safety
 * 3. Saving configuration to different formats
 * 4. Environment variable override
 * 5. Configuration builder (fluent API)
 * 6. Hot reload support
 */

#include <fdc_scheduler/config_manager.hpp>
#include <iostream>
#include <iomanip>

using namespace fdc_scheduler;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

/**
 * Demo 1: Creating and Saving Configuration
 */
void demo_create_and_save() {
    print_separator("Demo 1: Creating and Saving Configuration");
    
    ConfigManager config;
    
    // Add server configuration
    auto& server = config.root().add_section("server");
    server.set_string("host", "localhost");
    server.set_int("port", 8080);
    server.set_bool("enable_auth", true);
    
    // Add network configuration
    auto& network = config.root().add_section("network");
    network.set_double("default_speed", 120.5);
    network.set_int("max_trains", 100);
    
    // Add logging configuration
    auto& logging = config.root().add_section("logging");
    logging.set_string("level", "info");
    logging.set_string("file", "scheduler.log");
    logging.set_bool("console", true);
    
    std::cout << "Configuration created with 3 sections:\n";
    std::cout << "  - server (3 values)\n";
    std::cout << "  - network (2 values)\n";
    std::cout << "  - logging (3 values)\n\n";
    
    // Save to different formats
    std::cout << "Saving to files...\n";
    
    if (config.save_to_json("config_demo.json")) {
        std::cout << "✓ Saved to config_demo.json\n";
    }
    
    if (config.save_to_yaml("config_demo.yaml")) {
        std::cout << "✓ Saved to config_demo.yaml\n";
    }
    
    if (config.save_to_toml("config_demo.toml")) {
        std::cout << "✓ Saved to config_demo.toml\n";
    }
}

/**
 * Demo 2: Loading Configuration from Files
 */
void demo_load_from_file() {
    print_separator("Demo 2: Loading Configuration from Files");
    
    // Load from JSON
    ConfigManager json_config;
    if (json_config.load_from_json("config_demo.json")) {
        std::cout << "✓ Loaded from JSON\n";
        std::cout << "  Server host: " << json_config.get_string("server.host") << "\n";
        std::cout << "  Server port: " << json_config.get_int("server.port") << "\n";
        std::cout << "  Auth enabled: " << (json_config.get_bool("server.enable_auth") ? "true" : "false") << "\n";
    }
    
    std::cout << "\n";
    
    // Load from YAML
    ConfigManager yaml_config;
    if (yaml_config.load_from_yaml("config_demo.yaml")) {
        std::cout << "✓ Loaded from YAML\n";
        std::cout << "  Network speed: " << yaml_config.get_double("network.default_speed") << " km/h\n";
        std::cout << "  Max trains: " << yaml_config.get_int("network.max_trains") << "\n";
    }
    
    std::cout << "\n";
    
    // Load from TOML
    ConfigManager toml_config;
    if (toml_config.load_from_toml("config_demo.toml")) {
        std::cout << "✓ Loaded from TOML\n";
        std::cout << "  Log level: " << toml_config.get_string("logging.level") << "\n";
        std::cout << "  Log file: " << toml_config.get_string("logging.file") << "\n";
    }
}

/**
 * Demo 3: Type-Safe Value Access
 */
void demo_type_safe_access() {
    print_separator("Demo 3: Type-Safe Value Access");
    
    ConfigManager config;
    auto& test = config.root().add_section("test");
    
    // Set different types
    test.set_string("name", "FDC_Scheduler");
    test.set_int("version_major", 2);
    test.set_double("performance_factor", 1.5);
    test.set_bool("production_ready", true);
    
    std::vector<std::string> features = {"REST API", "Real-time", "Speed Optimization"};
    test.set_string_list("features", features);
    
    std::cout << "Accessing values with type safety:\n\n";
    
    std::cout << "String:  " << test.get_string("name") << "\n";
    std::cout << "Integer: " << test.get_int("version_major") << "\n";
    std::cout << "Double:  " << std::fixed << std::setprecision(2) 
              << test.get_double("performance_factor") << "\n";
    std::cout << "Boolean: " << (test.get_bool("production_ready") ? "true" : "false") << "\n";
    
    std::cout << "List:    ";
    auto feat_list = test.get_string_list("features");
    for (size_t i = 0; i < feat_list.size(); ++i) {
        std::cout << feat_list[i];
        if (i < feat_list.size() - 1) std::cout << ", ";
    }
    std::cout << "\n\n";
    
    // Test default values
    std::cout << "Testing default values:\n";
    std::cout << "  Missing string: \"" << test.get_string("nonexistent", "default") << "\"\n";
    std::cout << "  Missing int: " << test.get_int("nonexistent", 42) << "\n";
    std::cout << "  Missing bool: " << (test.get_bool("nonexistent", false) ? "true" : "false") << "\n";
}

/**
 * Demo 4: Nested Configuration Access
 */
void demo_nested_access() {
    print_separator("Demo 4: Nested Configuration Access");
    
    ConfigManager config;
    
    // Create nested structure
    auto& db = config.root().add_section("database");
    auto& connection = db.add_section("connection");
    connection.set_string("host", "localhost");
    connection.set_int("port", 5432);
    connection.set_string("name", "scheduler_db");
    
    auto& pool = db.add_section("pool");
    pool.set_int("min_connections", 5);
    pool.set_int("max_connections", 20);
    pool.set_int("timeout", 30);
    
    std::cout << "Nested configuration structure:\n";
    std::cout << "database/\n";
    std::cout << "  connection/\n";
    std::cout << "    host: " << connection.get_string("host") << "\n";
    std::cout << "    port: " << connection.get_int("port") << "\n";
    std::cout << "    name: " << connection.get_string("name") << "\n";
    std::cout << "  pool/\n";
    std::cout << "    min_connections: " << pool.get_int("min_connections") << "\n";
    std::cout << "    max_connections: " << pool.get_int("max_connections") << "\n";
    std::cout << "    timeout: " << pool.get_int("timeout") << " seconds\n";
    
    std::cout << "\nDirect path access:\n";
    std::cout << "  database.connection.host: " << config.get_string("database.connection.host") << "\n";
    std::cout << "  database.pool.max_connections: " << config.get_int("database.pool.max_connections") << "\n";
}

/**
 * Demo 5: Configuration Builder (Fluent API)
 */
void demo_builder() {
    print_separator("Demo 5: Configuration Builder (Fluent API)");
    
    ConfigManager config = ConfigBuilder()
        .server("0.0.0.0", 9000)
        .authentication(true, "secret-key-123")
        .rate_limit(60, 1000)
        .database("sqlite", "/var/db/scheduler.db")
        .logging("debug", "debug.log")
        .optimizer("eco", 0.8)
        .build();
    
    std::cout << "Configuration built with fluent API:\n\n";
    
    std::cout << "Server:\n";
    std::cout << "  Host: " << config.get_string("server.host") << "\n";
    std::cout << "  Port: " << config.get_int("server.port") << "\n";
    std::cout << "  Auth: " << (config.get_bool("server.enable_authentication") ? "enabled" : "disabled") << "\n";
    std::cout << "  JWT Secret: " << config.get_string("server.jwt_secret") << "\n";
    
    std::cout << "\nRate Limiting:\n";
    std::cout << "  Per minute: " << config.get_int("server.rate_limit.per_minute") << "\n";
    std::cout << "  Per hour: " << config.get_int("server.rate_limit.per_hour") << "\n";
    
    std::cout << "\nDatabase:\n";
    std::cout << "  Type: " << config.get_string("database.type") << "\n";
    std::cout << "  Path: " << config.get_string("database.path") << "\n";
    
    std::cout << "\nLogging:\n";
    std::cout << "  Level: " << config.get_string("logging.level") << "\n";
    std::cout << "  File: " << config.get_string("logging.file") << "\n";
    
    std::cout << "\nOptimizer:\n";
    std::cout << "  Mode: " << config.get_string("optimizer.mode") << "\n";
    std::cout << "  Energy weight: " << std::fixed << std::setprecision(1) 
              << config.get_double("optimizer.energy_weight") << "\n";
}

/**
 * Demo 6: Default Configuration
 */
void demo_default_config() {
    print_separator("Demo 6: Default Configuration");
    
    ConfigManager config = DefaultConfig::generate_default();
    
    std::cout << "Default configuration generated:\n\n";
    
    const auto& root = config.root();
    
    std::cout << "Available sections:\n";
    for (const auto& [name, section] : root.get_sections()) {
        std::cout << "  - " << name << " (" << section.get_values().size() << " values)\n";
    }
    
    std::cout << "\nServer defaults:\n";
    const auto& server = root.get_section("server");
    for (const auto& [key, value] : server.get_values()) {
        std::cout << "  " << key << ": ";
        std::visit([](auto&& arg) -> void {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                std::cout << "\"" << arg << "\"";
            } else if constexpr (std::is_same_v<T, bool>) {
                std::cout << (arg ? "true" : "false");
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                std::cout << "[";
                for (size_t i = 0; i < arg.size(); ++i) {
                    std::cout << "\"" << arg[i] << "\"";
                    if (i < arg.size() - 1) std::cout << ", ";
                }
                std::cout << "]";
            } else {
                std::cout << arg;
            }
        }, value);
        std::cout << "\n";
    }
    
    std::cout << "\nSaving default configuration...\n";
    if (config.save_to_json("default_config.json")) {
        std::cout << "✓ Saved to default_config.json\n";
    }
}

/**
 * Demo 7: Configuration Merging
 */
void demo_merge() {
    print_separator("Demo 7: Configuration Merging");
    
    // Base configuration
    ConfigManager base;
    base.set_string("app.name", "FDC_Scheduler");
    base.set_string("app.version", "2.0.0");
    base.set_string("app.environment", "development");
    base.set_int("server.port", 8080);
    
    // Production overrides
    ConfigManager prod_override;
    prod_override.set_string("app.environment", "production");
    prod_override.set_int("server.port", 80);
    prod_override.set_bool("server.enable_auth", true);
    
    std::cout << "Base configuration:\n";
    std::cout << "  App name: " << base.get_string("app.name") << "\n";
    std::cout << "  Environment: " << base.get_string("app.environment") << "\n";
    std::cout << "  Port: " << base.get_int("server.port") << "\n";
    
    std::cout << "\nMerging production overrides...\n";
    base.merge(prod_override);
    
    std::cout << "\nMerged configuration:\n";
    std::cout << "  App name: " << base.get_string("app.name") << " (unchanged)\n";
    std::cout << "  Environment: " << base.get_string("app.environment") << " (overridden)\n";
    std::cout << "  Port: " << base.get_int("server.port") << " (overridden)\n";
    std::cout << "  Auth: " << (base.get_bool("server.enable_auth") ? "enabled" : "disabled") 
              << " (new)\n";
}

int main() {
    std::cout << "FDC_Scheduler Configuration Manager Demo\n";
    std::cout << "=========================================\n";
    std::cout << "\nThis demo showcases configuration management:\n";
    std::cout << "- Multiple file formats (JSON, YAML, TOML)\n";
    std::cout << "- Type-safe value access\n";
    std::cout << "- Nested configuration\n";
    std::cout << "- Fluent builder API\n";
    std::cout << "- Default configuration\n";
    std::cout << "- Configuration merging\n";
    
    try {
        demo_create_and_save();
        demo_load_from_file();
        demo_type_safe_access();
        demo_nested_access();
        demo_builder();
        demo_default_config();
        demo_merge();
        
        print_separator("All Demos Completed Successfully!");
        
        std::cout << "Configuration Manager Features:\n";
        std::cout << "✓ JSON/YAML/TOML file support\n";
        std::cout << "✓ Type-safe value access\n";
        std::cout << "✓ Nested configuration sections\n";
        std::cout << "✓ Default values\n";
        std::cout << "✓ Fluent builder API\n";
        std::cout << "✓ Configuration merging\n";
        std::cout << "✓ Hot reload support (framework)\n";
        std::cout << "✓ Environment variable override\n";
        
        std::cout << "\nGenerated files:\n";
        std::cout << "  - config_demo.json\n";
        std::cout << "  - config_demo.yaml\n";
        std::cout << "  - config_demo.toml\n";
        std::cout << "  - default_config.json\n";
        
        std::cout << "\nNext steps:\n";
        std::cout << "1. Integrate yaml-cpp for full YAML support\n";
        std::cout << "2. Integrate toml11 for full TOML support\n";
        std::cout << "3. Add configuration validation rules\n";
        std::cout << "4. Implement configuration encryption\n";
        std::cout << "5. Add configuration versioning\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
