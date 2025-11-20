#include "fdc_scheduler/rest_api.hpp"
#include "fdc_scheduler/railway_network.hpp"
#include "fdc_scheduler/conflict_detector.hpp"
#include "fdc_scheduler/json_api.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

using json = nlohmann::json;

namespace fdc_scheduler {

// ============================================================================
// RateLimiter Implementation
// ============================================================================

RateLimiter::RateLimiter(const RateLimitConfig& config) 
    : config_(config) {
}

bool RateLimiter::allow_request(const std::string& client_ip) {
    if (!config_.enabled) return true;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto& stats = client_stats_[client_ip];
    
    // Initialize if first request
    if (stats.requests_in_minute == 0) {
        stats.minute_start = now;
        stats.hour_start = now;
    }
    
    // Check minute window
    auto minute_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - stats.minute_start).count();
    if (minute_elapsed >= 60) {
        stats.requests_in_minute = 0;
        stats.minute_start = now;
    }
    
    // Check hour window
    auto hour_elapsed = std::chrono::duration_cast<std::chrono::minutes>(
        now - stats.hour_start).count();
    if (hour_elapsed >= 60) {
        stats.requests_in_hour = 0;
        stats.hour_start = now;
    }
    
    // Check limits
    if (stats.requests_in_minute >= config_.max_requests_per_minute) {
        return false;
    }
    if (stats.requests_in_hour >= config_.max_requests_per_hour) {
        return false;
    }
    
    // Increment counters
    stats.requests_in_minute++;
    stats.requests_in_hour++;
    stats.last_request = now;
    
    // Cleanup old entries periodically
    if (client_stats_.size() > 1000) {
        cleanup_old_entries();
    }
    
    return true;
}

void RateLimiter::reset_client(const std::string& client_ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    client_stats_.erase(client_ip);
}

int RateLimiter::get_remaining_requests(const std::string& client_ip) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = client_stats_.find(client_ip);
    if (it == client_stats_.end()) {
        return config_.max_requests_per_minute;
    }
    
    return std::max(0, config_.max_requests_per_minute - it->second.requests_in_minute);
}

void RateLimiter::cleanup_old_entries() {
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::hours(2);
    
    for (auto it = client_stats_.begin(); it != client_stats_.end(); ) {
        if (it->second.last_request < cutoff) {
            it = client_stats_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// JWTManager Implementation (Simplified - production should use a library)
// ============================================================================

JWTManager::JWTManager(const std::string& secret_key) 
    : secret_key_(secret_key) {
}

std::string JWTManager::generate_token(const std::string& user_id,
                                       const std::vector<std::string>& permissions,
                                       int expiry_hours) {
    // Header
    json header = {
        {"alg", "HS256"},
        {"typ", "JWT"}
    };
    
    // Payload
    auto now = std::chrono::system_clock::now();
    auto expiry = now + std::chrono::hours(expiry_hours);
    auto expiry_time = std::chrono::system_clock::to_time_t(expiry);
    
    json payload = {
        {"user_id", user_id},
        {"permissions", permissions},
        {"exp", expiry_time},
        {"iat", std::chrono::system_clock::to_time_t(now)}
    };
    
    // Encode
    std::string header_encoded = base64_encode(header.dump());
    std::string payload_encoded = base64_encode(payload.dump());
    std::string signature_input = header_encoded + "." + payload_encoded;
    std::string signature = hmac_sha256(signature_input);
    
    return header_encoded + "." + payload_encoded + "." + signature;
}

AuthResult JWTManager::verify_token(const std::string& token) {
    AuthResult result;
    result.authenticated = false;
    
    // Split token
    std::istringstream iss(token);
    std::string header_enc, payload_enc, signature;
    
    if (!std::getline(iss, header_enc, '.') ||
        !std::getline(iss, payload_enc, '.') ||
        !std::getline(iss, signature, '.')) {
        result.error_message = "Invalid token format";
        return result;
    }
    
    // Verify signature
    std::string signature_input = header_enc + "." + payload_enc;
    std::string expected_signature = hmac_sha256(signature_input);
    
    if (signature != expected_signature) {
        result.error_message = "Invalid signature";
        return result;
    }
    
    // Decode payload
    try {
        std::string payload_str = base64_decode(payload_enc);
        json payload = json::parse(payload_str);
        
        // Check expiry
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        
        if (payload.contains("exp") && payload["exp"].get<time_t>() < now_time) {
            result.error_message = "Token expired";
            return result;
        }
        
        result.authenticated = true;
        result.user_id = payload["user_id"].get<std::string>();
        if (payload.contains("permissions")) {
            result.permissions = payload["permissions"].get<std::vector<std::string>>();
        }
        
    } catch (const std::exception& e) {
        result.error_message = std::string("Token parsing error: ") + e.what();
        return result;
    }
    
    return result;
}

std::string JWTManager::base64_encode(const std::string& input) {
    // Simplified base64 encoding (URL-safe)
    static const char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";
    
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    for (char c : input) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];
    }
    
    return ret;
}

std::string JWTManager::base64_decode(const std::string& input) {
    // Simplified base64 decoding (URL-safe)
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";
    
    std::string ret;
    int i = 0;
    unsigned char char_array_4[4], char_array_3[3];
    
    for (char c : input) {
        if (c == '=') break;
        
        size_t pos = base64_chars.find(c);
        if (pos == std::string::npos) continue;
        
        char_array_4[i++] = static_cast<unsigned char>(pos);
        if (i == 4) {
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                ret += char_array_3[i];
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 4; j++)
            char_array_4[j] = 0;
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        
        for (int j = 0; j < i - 1; j++)
            ret += char_array_3[j];
    }
    
    return ret;
}

std::string JWTManager::hmac_sha256(const std::string& data) {
    // Simplified HMAC (production should use OpenSSL or similar)
    std::hash<std::string> hasher;
    size_t hash_value = hasher(secret_key_ + data);
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hash_value;
    return base64_encode(oss.str());
}

// ============================================================================
// RESTServer Implementation
// ============================================================================

RESTServer::RESTServer(std::shared_ptr<RailwayNetwork> network,
                       const RESTServerConfig& config)
    : network_(network)
    , config_(config)
    , running_(false) {
    
    // Initialize components
    if (config_.enable_authentication) {
        jwt_manager_ = std::make_shared<JWTManager>(config_.jwt_secret);
    }
    
    rate_limiter_ = std::make_unique<RateLimiter>(config_.rate_limit);
    
    // Initialize stats
    stats_.start_time = std::chrono::system_clock::now();
    
    // Register default routes
    register_default_routes();
}

RESTServer::~RESTServer() {
    if (running_) {
        stop();
    }
}

void RESTServer::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        throw std::runtime_error("Server already running");
    }
    
    // NOTE: This is a stub implementation
    // A real implementation would use cpp-httplib or Boost.Beast
    // to create an actual HTTP server
    
    running_ = true;
    
    // Log server start
    std::cout << "REST API Server started on " 
              << config_.host << ":" << config_.port << std::endl;
    std::cout << "API Version: " << config_.api_version << std::endl;
    std::cout << "Authentication: " 
              << (config_.enable_authentication ? "Enabled" : "Disabled") << std::endl;
}

void RESTServer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    std::cout << "REST API Server stopped" << std::endl;
}

bool RESTServer::is_running() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

void RESTServer::add_route(const std::string& method,
                           const std::string& path,
                           RouteHandler handler) {
    routes_[method][path] = handler;
}

void RESTServer::set_jwt_manager(std::shared_ptr<JWTManager> jwt_manager) {
    jwt_manager_ = jwt_manager;
}

std::string RESTServer::generate_api_token(const std::string& user_id,
                                           const std::vector<std::string>& permissions) {
    if (!jwt_manager_) {
        throw std::runtime_error("JWT manager not initialized");
    }
    return jwt_manager_->generate_token(user_id, permissions);
}

APIStats RESTServer::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void RESTServer::reset_stats() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_ = APIStats();
    stats_.start_time = std::chrono::system_clock::now();
}

void RESTServer::update_rate_limit(const RateLimitConfig& config) {
    rate_limiter_ = std::make_unique<RateLimiter>(config);
    config_.rate_limit = config;
}

void RESTServer::register_default_routes() {
    // Network endpoints
    add_route("GET", "/api/" + config_.api_version + "/network",
              [this](const HTTPRequest& req) { return handle_get_network(req); });
    add_route("GET", "/api/" + config_.api_version + "/nodes",
              [this](const HTTPRequest& req) { return handle_get_nodes(req); });
    add_route("POST", "/api/" + config_.api_version + "/nodes",
              [this](const HTTPRequest& req) { return handle_add_node(req); });
    
    // Health check
    add_route("GET", "/health",
              [this](const HTTPRequest& req) { return handle_health_check(req); });
    add_route("GET", "/api/" + config_.api_version + "/stats",
              [this](const HTTPRequest& req) { return handle_stats(req); });
    
    // OpenAPI spec
    add_route("GET", "/api/" + config_.api_version + "/openapi",
              [this](const HTTPRequest& req) { return handle_openapi_spec(req); });
}

// Network endpoints
HTTPResponse RESTServer::handle_get_network(const HTTPRequest& req) {
    try {
        json response = {
            {"status", "success"},
            {"data", {
                {"node_count", network_->num_nodes()},
                {"edge_count", network_->num_edges()}
            }}
        };
        return json_response(response.dump());
    } catch (const std::exception& e) {
        return error_response(500, e.what());
    }
}

HTTPResponse RESTServer::handle_get_nodes(const HTTPRequest& req) {
    try {
        json nodes_array = json::array();
        
        // Get all nodes from network
        // This is a placeholder - actual implementation depends on network API
        
        json response = {
            {"status", "success"},
            {"data", {
                {"nodes", nodes_array},
                {"count", nodes_array.size()}
            }}
        };
        return json_response(response.dump());
    } catch (const std::exception& e) {
        return error_response(500, e.what());
    }
}

HTTPResponse RESTServer::handle_get_node(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_add_node(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_update_node(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_delete_node(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_get_edges(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_add_edge(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_delete_edge(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

// Schedule endpoints
HTTPResponse RESTServer::handle_get_schedules(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_add_schedule(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_get_schedule(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_update_schedule(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_delete_schedule(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

// Conflict detection endpoints
HTTPResponse RESTServer::handle_detect_conflicts(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_resolve_conflicts(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

// Pathfinding endpoints
HTTPResponse RESTServer::handle_find_path(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_find_k_paths(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

// Optimization endpoints
HTTPResponse RESTServer::handle_optimize_speed(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_optimize_route(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_realtime_optimize(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

// System endpoints
HTTPResponse RESTServer::handle_health_check(const HTTPRequest& req) {
    json response = {
        {"status", "healthy"},
        {"version", config_.api_version},
        {"uptime_hours", stats_.get_uptime_hours()},
        {"timestamp", std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now())}
    };
    return json_response(response.dump());
}

HTTPResponse RESTServer::handle_stats(const HTTPRequest& req) {
    auto stats = get_stats();
    
    json response = {
        {"status", "success"},
        {"data", {
            {"total_requests", stats.total_requests},
            {"successful_requests", stats.successful_requests},
            {"failed_requests", stats.failed_requests},
            {"rate_limited_requests", stats.rate_limited_requests},
            {"auth_failures", stats.auth_failures},
            {"success_rate", stats.get_success_rate()},
            {"uptime_hours", stats.get_uptime_hours()}
        }}
    };
    return json_response(response.dump());
}

HTTPResponse RESTServer::handle_openapi_spec(const HTTPRequest& req) {
    std::string spec = generate_openapi_spec();
    return json_response(spec);
}

// Authentication endpoints
HTTPResponse RESTServer::handle_login(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

HTTPResponse RESTServer::handle_refresh_token(const HTTPRequest& req) {
    return error_response(501, "Not implemented");
}

// Middleware
HTTPResponse RESTServer::apply_middleware(const HTTPRequest& req,
                                         const RouteHandler& handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.total_requests++;
    
    // Check rate limit
    if (!check_rate_limit(req)) {
        stats_.rate_limited_requests++;
        stats_.failed_requests++;
        return error_response(429, "Too many requests");
    }
    
    // Check authentication
    if (config_.enable_authentication) {
        AuthResult auth;
        if (!check_authentication(req, auth)) {
            stats_.auth_failures++;
            stats_.failed_requests++;
            return error_response(401, auth.error_message);
        }
    }
    
    // Execute handler
    try {
        HTTPResponse resp = handler(req);
        
        // Add CORS headers
        if (config_.enable_cors) {
            add_cors_headers(resp);
        }
        
        if (resp.status_code >= 200 && resp.status_code < 300) {
            stats_.successful_requests++;
        } else {
            stats_.failed_requests++;
        }
        
        return resp;
    } catch (const std::exception& e) {
        stats_.failed_requests++;
        return error_response(500, e.what());
    }
}

bool RESTServer::check_authentication(const HTTPRequest& req, AuthResult& auth) {
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) {
        auth.error_message = "Missing Authorization header";
        return false;
    }
    
    std::string auth_header = it->second;
    if (auth_header.substr(0, 7) != "Bearer ") {
        auth.error_message = "Invalid Authorization header format";
        return false;
    }
    
    std::string token = auth_header.substr(7);
    auth = jwt_manager_->verify_token(token);
    
    return auth.authenticated;
}

bool RESTServer::check_rate_limit(const HTTPRequest& req) {
    return rate_limiter_->allow_request(req.client_ip);
}

void RESTServer::add_cors_headers(HTTPResponse& resp) {
    resp.headers["Access-Control-Allow-Origin"] = 
        config_.allowed_origins.empty() ? "*" : config_.allowed_origins[0];
    resp.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
    resp.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
}

HTTPResponse RESTServer::error_response(int status_code, const std::string& message) {
    HTTPResponse resp;
    resp.status_code = status_code;
    
    json error_json = {
        {"status", "error"},
        {"error", {
            {"code", status_code},
            {"message", message}
        }}
    };
    
    resp.body = error_json.dump();
    return resp;
}

HTTPResponse RESTServer::json_response(const std::string& json_str, int status_code) {
    HTTPResponse resp;
    resp.status_code = status_code;
    resp.body = json_str;
    resp.headers["Content-Type"] = "application/json";
    return resp;
}

std::string RESTServer::generate_openapi_spec() {
    json spec = {
        {"openapi", "3.0.0"},
        {"info", {
            {"title", "FDC_Scheduler REST API"},
            {"version", config_.api_version},
            {"description", "REST API for railway network scheduling and optimization"}
        }},
        {"servers", json::array({
            {{"url", "http://" + config_.host + ":" + std::to_string(config_.port)}}
        })},
        {"paths", {
            {"/health", {
                {"get", {
                    {"summary", "Health check"},
                    {"responses", {
                        {"200", {{"description", "Service is healthy"}}}
                    }}
                }}
            }},
            {"/api/" + config_.api_version + "/network", {
                {"get", {
                    {"summary", "Get network information"},
                    {"responses", {
                        {"200", {{"description", "Network data"}}}
                    }}
                }}
            }}
        }}
    };
    
    return spec.dump(2);
}

} // namespace fdc_scheduler
