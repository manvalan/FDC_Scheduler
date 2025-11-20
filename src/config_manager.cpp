#include "fdc_scheduler/config_manager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

using json = nlohmann::json;

namespace fdc_scheduler {

// ============================================================================
// ConfigSection Implementation
// ============================================================================

std::string ConfigSection::get_string(const std::string& key, const std::string& default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    
    if (std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    
    return default_value;
}

int ConfigSection::get_int(const std::string& key, int default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    
    if (std::holds_alternative<int>(it->second)) {
        return std::get<int>(it->second);
    } else if (std::holds_alternative<double>(it->second)) {
        return static_cast<int>(std::get<double>(it->second));
    }
    
    return default_value;
}

double ConfigSection::get_double(const std::string& key, double default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    
    if (std::holds_alternative<double>(it->second)) {
        return std::get<double>(it->second);
    } else if (std::holds_alternative<int>(it->second)) {
        return static_cast<double>(std::get<int>(it->second));
    }
    
    return default_value;
}

bool ConfigSection::get_bool(const std::string& key, bool default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    
    if (std::holds_alternative<bool>(it->second)) {
        return std::get<bool>(it->second);
    }
    
    return default_value;
}

std::vector<std::string> ConfigSection::get_string_list(const std::string& key) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return {};
    }
    
    if (std::holds_alternative<std::vector<std::string>>(it->second)) {
        return std::get<std::vector<std::string>>(it->second);
    }
    
    return {};
}

void ConfigSection::set(const std::string& key, const ConfigValue& value) {
    values_[key] = value;
}

void ConfigSection::set_string(const std::string& key, const std::string& value) {
    values_[key] = value;
}

void ConfigSection::set_int(const std::string& key, int value) {
    values_[key] = value;
}

void ConfigSection::set_double(const std::string& key, double value) {
    values_[key] = value;
}

void ConfigSection::set_bool(const std::string& key, bool value) {
    values_[key] = value;
}

void ConfigSection::set_string_list(const std::string& key, const std::vector<std::string>& value) {
    values_[key] = value;
}

bool ConfigSection::has(const std::string& key) const {
    return values_.find(key) != values_.end();
}

ConfigSection& ConfigSection::add_section(const std::string& name) {
    sections_[name] = ConfigSection(name);
    return sections_[name];
}

const ConfigSection& ConfigSection::get_section(const std::string& name) const {
    auto it = sections_.find(name);
    if (it == sections_.end()) {
        throw std::runtime_error("Section not found: " + name);
    }
    return it->second;
}

ConfigSection& ConfigSection::get_section(const std::string& name) {
    auto it = sections_.find(name);
    if (it == sections_.end()) {
        throw std::runtime_error("Section not found: " + name);
    }
    return it->second;
}

bool ConfigSection::has_section(const std::string& name) const {
    return sections_.find(name) != sections_.end();
}

// ============================================================================
// ConfigManager Implementation
// ============================================================================

ConfigManager::ConfigManager()
    : root_("root")
    , last_loaded_time_(0)
    , hot_reload_enabled_(false) {
}

bool ConfigManager::load_from_file(const std::string& filename) {
    // Detect format from extension
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    
    if (ext == "json") {
        return load_from_json(filename);
    } else if (ext == "yaml" || ext == "yml") {
        return load_from_yaml(filename);
    } else if (ext == "toml") {
        return load_from_toml(filename);
    }
    
    last_error_ = "Unknown file format: " + ext;
    return false;
}

bool ConfigManager::load_from_json(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            last_error_ = "Cannot open file: " + filename;
            return false;
        }
        
        json j;
        file >> j;
        
        // Clear existing configuration
        root_ = ConfigSection("root");
        
        // Parse JSON into configuration
        std::function<void(const json&, ConfigSection&)> parse_json;
        parse_json = [&](const json& node, ConfigSection& section) {
            for (auto& [key, value] : node.items()) {
                if (value.is_object()) {
                    auto& subsection = section.add_section(key);
                    parse_json(value, subsection);
                } else if (value.is_string()) {
                    section.set_string(key, value.get<std::string>());
                } else if (value.is_number_integer()) {
                    section.set_int(key, value.get<int>());
                } else if (value.is_number_float()) {
                    section.set_double(key, value.get<double>());
                } else if (value.is_boolean()) {
                    section.set_bool(key, value.get<bool>());
                } else if (value.is_array() && !value.empty() && value[0].is_string()) {
                    std::vector<std::string> arr;
                    for (const auto& item : value) {
                        arr.push_back(item.get<std::string>());
                    }
                    section.set_string_list(key, arr);
                }
            }
        };
        
        parse_json(j, root_);
        
        last_loaded_file_ = filename;
        struct stat st;
        if (stat(filename.c_str(), &st) == 0) {
            last_loaded_time_ = st.st_mtime;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("JSON parsing error: ") + e.what();
        return false;
    }
}

bool ConfigManager::load_from_yaml(const std::string& filename) {
    // Simplified YAML parser (basic key-value pairs and sections)
    // For production, use yaml-cpp library
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            last_error_ = "Cannot open file: " + filename;
            return false;
        }
        
        root_ = ConfigSection("root");
        ConfigSection* current_section = &root_;
        std::string line;
        int current_indent = 0;
        
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            // Count indentation
            int indent = 0;
            while (indent < line.size() && line[indent] == ' ') {
                indent++;
            }
            
            std::string trimmed = line.substr(indent);
            if (trimmed.empty()) continue;
            
            // Check if it's a section (ends with :)
            if (trimmed.back() == ':') {
                std::string section_name = trimmed.substr(0, trimmed.size() - 1);
                current_section = &root_.add_section(section_name);
                current_indent = indent;
            } else {
                // Parse key-value pair
                size_t colon_pos = trimmed.find(':');
                if (colon_pos != std::string::npos) {
                    std::string key = trimmed.substr(0, colon_pos);
                    std::string value = trimmed.substr(colon_pos + 1);
                    
                    // Trim whitespace
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    
                    // Parse value type
                    if (value == "true") {
                        current_section->set_bool(key, true);
                    } else if (value == "false") {
                        current_section->set_bool(key, false);
                    } else if (!value.empty() && (std::isdigit(value[0]) || value[0] == '-')) {
                        if (value.find('.') != std::string::npos) {
                            current_section->set_double(key, std::stod(value));
                        } else {
                            current_section->set_int(key, std::stoi(value));
                        }
                    } else {
                        // Remove quotes if present
                        if (!value.empty() && (value.front() == '"' || value.front() == '\'')) {
                            value = value.substr(1, value.size() - 2);
                        }
                        current_section->set_string(key, value);
                    }
                }
            }
        }
        
        last_loaded_file_ = filename;
        struct stat st;
        if (stat(filename.c_str(), &st) == 0) {
            last_loaded_time_ = st.st_mtime;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("YAML parsing error: ") + e.what();
        return false;
    }
}

bool ConfigManager::load_from_toml(const std::string& filename) {
    // Simplified TOML parser (basic key-value pairs and sections)
    // For production, use toml11 library
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            last_error_ = "Cannot open file: " + filename;
            return false;
        }
        
        root_ = ConfigSection("root");
        ConfigSection* current_section = &root_;
        std::string line;
        
        while (std::getline(file, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            // Check if it's a section header [section]
            if (line.front() == '[' && line.back() == ']') {
                std::string section_name = line.substr(1, line.size() - 2);
                current_section = &root_.add_section(section_name);
            } else {
                // Parse key-value pair
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = line.substr(0, eq_pos);
                    std::string value = line.substr(eq_pos + 1);
                    
                    // Trim whitespace
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    
                    // Parse value type
                    if (value == "true") {
                        current_section->set_bool(key, true);
                    } else if (value == "false") {
                        current_section->set_bool(key, false);
                    } else if (!value.empty() && value.front() == '"') {
                        // String value
                        value = value.substr(1, value.size() - 2);
                        current_section->set_string(key, value);
                    } else if (!value.empty() && std::isdigit(value[0])) {
                        // Numeric value
                        if (value.find('.') != std::string::npos) {
                            current_section->set_double(key, std::stod(value));
                        } else {
                            current_section->set_int(key, std::stoi(value));
                        }
                    } else {
                        current_section->set_string(key, value);
                    }
                }
            }
        }
        
        last_loaded_file_ = filename;
        struct stat st;
        if (stat(filename.c_str(), &st) == 0) {
            last_loaded_time_ = st.st_mtime;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("TOML parsing error: ") + e.what();
        return false;
    }
}

bool ConfigManager::save_to_file(const std::string& filename) const {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    
    if (ext == "json") {
        return save_to_json(filename);
    } else if (ext == "yaml" || ext == "yml") {
        return save_to_yaml(filename);
    } else if (ext == "toml") {
        return save_to_toml(filename);
    }
    
    last_error_ = "Unknown file format: " + ext;
    return false;
}

bool ConfigManager::save_to_json(const std::string& filename) const {
    try {
        json j;
        
        std::function<void(const ConfigSection&, json&)> build_json;
        build_json = [&](const ConfigSection& section, json& node) {
            // Add values
            for (const auto& [key, value] : section.get_values()) {
                std::visit([&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        node[key] = arg;
                    } else if constexpr (std::is_same_v<T, int>) {
                        node[key] = arg;
                    } else if constexpr (std::is_same_v<T, double>) {
                        node[key] = arg;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        node[key] = arg;
                    } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                        node[key] = arg;
                    }
                }, value);
            }
            
            // Add subsections
            for (const auto& [name, subsection] : section.get_sections()) {
                json subnode = json::object();
                build_json(subsection, subnode);
                node[name] = subnode;
            }
        };
        
        build_json(root_, j);
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            last_error_ = "Cannot write to file: " + filename;
            return false;
        }
        
        file << j.dump(2);
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("JSON writing error: ") + e.what();
        return false;
    }
}

bool ConfigManager::save_to_yaml(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            last_error_ = "Cannot write to file: " + filename;
            return false;
        }
        
        std::function<void(const ConfigSection&, std::ofstream&, int)> write_yaml;
        write_yaml = [&](const ConfigSection& section, std::ofstream& out, int indent) {
            std::string prefix(indent, ' ');
            
            // Write values
            for (const auto& [key, value] : section.get_values()) {
                out << prefix << key << ": ";
                std::visit([&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        out << "\"" << arg << "\"";
                    } else if constexpr (std::is_same_v<T, bool>) {
                        out << (arg ? "true" : "false");
                    } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                        out << "[";
                        for (size_t i = 0; i < arg.size(); ++i) {
                            out << "\"" << arg[i] << "\"";
                            if (i < arg.size() - 1) out << ", ";
                        }
                        out << "]";
                    } else {
                        out << arg;
                    }
                }, value);
                out << "\n";
            }
            
            // Write subsections
            for (const auto& [name, subsection] : section.get_sections()) {
                out << prefix << name << ":\n";
                write_yaml(subsection, out, indent + 2);
            }
        };
        
        write_yaml(root_, file, 0);
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("YAML writing error: ") + e.what();
        return false;
    }
}

bool ConfigManager::save_to_toml(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            last_error_ = "Cannot write to file: " + filename;
            return false;
        }
        
        std::function<void(const ConfigSection&, std::ofstream&, const std::string&)> write_toml;
        write_toml = [&](const ConfigSection& section, std::ofstream& out, const std::string& section_path) {
            // Write values
            for (const auto& [key, value] : section.get_values()) {
                out << key << " = ";
                std::visit([&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        out << "\"" << arg << "\"";
                    } else if constexpr (std::is_same_v<T, bool>) {
                        out << (arg ? "true" : "false");
                    } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                        out << "[";
                        for (size_t i = 0; i < arg.size(); ++i) {
                            out << "\"" << arg[i] << "\"";
                            if (i < arg.size() - 1) out << ", ";
                        }
                        out << "]";
                    } else {
                        out << arg;
                    }
                }, value);
                out << "\n";
            }
            
            // Write subsections
            for (const auto& [name, subsection] : section.get_sections()) {
                out << "\n[" << (section_path.empty() ? name : section_path + "." + name) << "]\n";
                write_toml(subsection, out, section_path.empty() ? name : section_path + "." + name);
            }
        };
        
        write_toml(root_, file, "");
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("TOML writing error: ") + e.what();
        return false;
    }
}

void ConfigManager::load_from_env(const std::string& prefix) {
    // Simplified implementation using getenv
    // In production, iterate through all env vars or use a config list
    
    // Common environment variables to check
    std::vector<std::string> common_vars = {
        "HOST", "PORT", "DATABASE_PATH", "LOG_LEVEL", "LOG_FILE",
        "API_VERSION", "JWT_SECRET", "ENABLE_AUTH", "MAX_CONNECTIONS"
    };
    
    for (const auto& var : common_vars) {
        std::string full_key = prefix + var;
        const char* value = std::getenv(full_key.c_str());
        
        if (value != nullptr) {
            // Convert UPPER_CASE to lower.case
            std::string config_key;
            for (char c : var) {
                if (c == '_') config_key += '.';
                else config_key += std::tolower(c);
            }
            
            set_string(config_key, value);
        }
    }
}

std::vector<std::string> ConfigManager::parse_path(const std::string& path) const {
    std::vector<std::string> parts;
    std::string current;
    
    for (char c : path) {
        if (c == '.') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        parts.push_back(current);
    }
    
    return parts;
}

const ConfigSection* ConfigManager::get_section_by_path(const std::string& path) const {
    auto parts = parse_path(path);
    if (parts.empty()) return &root_;
    
    const ConfigSection* section = &root_;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        if (!section->has_section(parts[i])) {
            return nullptr;
        }
        section = &section->get_section(parts[i]);
    }
    
    return section;
}

ConfigSection* ConfigManager::get_section_by_path(const std::string& path) {
    auto parts = parse_path(path);
    if (parts.empty()) return &root_;
    
    ConfigSection* section = &root_;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        if (!section->has_section(parts[i])) {
            section->add_section(parts[i]);
        }
        section = &section->get_section(parts[i]);
    }
    
    return section;
}

std::string ConfigManager::get_string(const std::string& path, const std::string& default_value) const {
    auto parts = parse_path(path);
    if (parts.empty()) return default_value;
    
    const ConfigSection* section = get_section_by_path(path);
    if (!section) return default_value;
    
    return section->get_string(parts.back(), default_value);
}

int ConfigManager::get_int(const std::string& path, int default_value) const {
    auto parts = parse_path(path);
    if (parts.empty()) return default_value;
    
    const ConfigSection* section = get_section_by_path(path);
    if (!section) return default_value;
    
    return section->get_int(parts.back(), default_value);
}

double ConfigManager::get_double(const std::string& path, double default_value) const {
    auto parts = parse_path(path);
    if (parts.empty()) return default_value;
    
    const ConfigSection* section = get_section_by_path(path);
    if (!section) return default_value;
    
    return section->get_double(parts.back(), default_value);
}

bool ConfigManager::get_bool(const std::string& path, bool default_value) const {
    auto parts = parse_path(path);
    if (parts.empty()) return default_value;
    
    const ConfigSection* section = get_section_by_path(path);
    if (!section) return default_value;
    
    return section->get_bool(parts.back(), default_value);
}

void ConfigManager::set_string(const std::string& path, const std::string& value) {
    auto parts = parse_path(path);
    if (parts.empty()) return;
    
    ConfigSection* section = get_section_by_path(path);
    if (section) {
        section->set_string(parts.back(), value);
    }
}

void ConfigManager::set_int(const std::string& path, int value) {
    auto parts = parse_path(path);
    if (parts.empty()) return;
    
    ConfigSection* section = get_section_by_path(path);
    if (section) {
        section->set_int(parts.back(), value);
    }
}

void ConfigManager::set_double(const std::string& path, double value) {
    auto parts = parse_path(path);
    if (parts.empty()) return;
    
    ConfigSection* section = get_section_by_path(path);
    if (section) {
        section->set_double(parts.back(), value);
    }
}

void ConfigManager::set_bool(const std::string& path, bool value) {
    auto parts = parse_path(path);
    if (parts.empty()) return;
    
    ConfigSection* section = get_section_by_path(path);
    if (section) {
        section->set_bool(parts.back(), value);
    }
}

bool ConfigManager::validate() const {
    // Basic validation - can be extended
    return true;
}

std::vector<std::string> ConfigManager::get_validation_errors() const {
    return {};
}

void ConfigManager::enable_hot_reload(bool enable) {
    hot_reload_enabled_ = enable;
}

bool ConfigManager::check_and_reload() {
    if (!hot_reload_enabled_ || last_loaded_file_.empty()) {
        return false;
    }
    
    struct stat st;
    if (stat(last_loaded_file_.c_str(), &st) != 0) {
        return false;
    }
    
    if (st.st_mtime > last_loaded_time_) {
        return load_from_file(last_loaded_file_);
    }
    
    return false;
}

void ConfigManager::merge(const ConfigManager& other) {
    // Simple merge - other values override current values
    // Can be extended for more sophisticated merging
    std::function<void(ConfigSection&, const ConfigSection&)> merge_sections;
    merge_sections = [&](ConfigSection& target, const ConfigSection& source) {
        for (const auto& [key, value] : source.get_values()) {
            target.set(key, value);
        }
        
        for (const auto& [name, subsection] : source.get_sections()) {
            if (target.has_section(name)) {
                merge_sections(target.get_section(name), subsection);
            } else {
                target.add_section(name);
                merge_sections(target.get_section(name), subsection);
            }
        }
    };
    
    merge_sections(root_, other.root_);
}

void ConfigManager::clear() {
    root_ = ConfigSection("root");
    last_loaded_file_.clear();
    last_loaded_time_ = 0;
}

// ============================================================================
// DefaultConfig Implementation
// ============================================================================

ConfigSection DefaultConfig::generate_server_config() {
    ConfigSection server("server");
    server.set_string("host", "0.0.0.0");
    server.set_int("port", 8080);
    server.set_string("api_version", "v1");
    server.set_bool("enable_cors", true);
    server.set_int("max_body_size_mb", 10);
    server.set_int("timeout_seconds", 30);
    return server;
}

ConfigSection DefaultConfig::generate_network_config() {
    ConfigSection network("network");
    network.set_double("default_speed_limit", 120.0);
    network.set_double("buffer_time_seconds", 60.0);
    network.set_bool("allow_dynamic_routing", true);
    return network;
}

ConfigSection DefaultConfig::generate_scheduler_config() {
    ConfigSection scheduler("scheduler");
    scheduler.set_int("max_conflicts_per_run", 100);
    scheduler.set_double("optimization_timeout_seconds", 5.0);
    scheduler.set_string("resolution_strategy", "priority_based");
    return scheduler;
}

ConfigSection DefaultConfig::generate_optimizer_config() {
    ConfigSection optimizer("optimizer");
    optimizer.set_string("mode", "balanced");
    optimizer.set_double("energy_weight", 0.5);
    optimizer.set_double("time_weight", 0.5);
    optimizer.set_bool("enable_speed_optimization", true);
    return optimizer;
}

ConfigSection DefaultConfig::generate_logging_config() {
    ConfigSection logging("logging");
    logging.set_string("level", "info");
    logging.set_string("file", "fdc_scheduler.log");
    logging.set_bool("console_output", true);
    logging.set_int("max_file_size_mb", 100);
    return logging;
}

ConfigManager DefaultConfig::generate_default() {
    ConfigManager config;
    
    auto& root = config.root();
    root.add_section("server") = generate_server_config();
    root.add_section("network") = generate_network_config();
    root.add_section("scheduler") = generate_scheduler_config();
    root.add_section("optimizer") = generate_optimizer_config();
    root.add_section("logging") = generate_logging_config();
    
    return config;
}

// ============================================================================
// ConfigBuilder Implementation
// ============================================================================

ConfigBuilder& ConfigBuilder::server(const std::string& host, int port) {
    config_.set_string("server.host", host);
    config_.set_int("server.port", port);
    return *this;
}

ConfigBuilder& ConfigBuilder::authentication(bool enabled, const std::string& jwt_secret) {
    config_.set_bool("server.enable_authentication", enabled);
    if (!jwt_secret.empty()) {
        config_.set_string("server.jwt_secret", jwt_secret);
    }
    return *this;
}

ConfigBuilder& ConfigBuilder::rate_limit(int per_minute, int per_hour) {
    config_.set_int("server.rate_limit.per_minute", per_minute);
    config_.set_int("server.rate_limit.per_hour", per_hour);
    return *this;
}

ConfigBuilder& ConfigBuilder::database(const std::string& type, const std::string& path) {
    config_.set_string("database.type", type);
    config_.set_string("database.path", path);
    return *this;
}

ConfigBuilder& ConfigBuilder::logging(const std::string& level, const std::string& file) {
    config_.set_string("logging.level", level);
    if (!file.empty()) {
        config_.set_string("logging.file", file);
    }
    return *this;
}

ConfigBuilder& ConfigBuilder::optimizer(const std::string& mode, double energy_weight) {
    config_.set_string("optimizer.mode", mode);
    config_.set_double("optimizer.energy_weight", energy_weight);
    return *this;
}

ConfigManager ConfigBuilder::build() const {
    return config_;
}

} // namespace fdc_scheduler
