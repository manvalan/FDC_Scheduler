#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <chrono>
#include <mutex>

namespace fdc_scheduler {

// Forward declarations
class RailwayNetwork;
class ConflictDetector;
class RailwayAIResolver;
class Schedule;

/**
 * @brief HTTP request data
 */
struct HTTPRequest {
    std::string method;        // GET, POST, PUT, DELETE
    std::string path;          // /api/v1/network
    std::string body;          // JSON payload
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::string client_ip;
};

/**
 * @brief HTTP response data
 */
struct HTTPResponse {
    int status_code;           // 200, 404, 500, etc.
    std::string body;          // JSON response
    std::map<std::string, std::string> headers;
    
    HTTPResponse() : status_code(200) {
        headers["Content-Type"] = "application/json";
    }
};

/**
 * @brief Route handler function type
 */
using RouteHandler = std::function<HTTPResponse(const HTTPRequest&)>;

/**
 * @brief Authentication result
 */
struct AuthResult {
    bool authenticated;
    std::string user_id;
    std::string error_message;
    std::vector<std::string> permissions;
};

/**
 * @brief Rate limiting configuration
 */
struct RateLimitConfig {
    int max_requests_per_minute = 60;
    int max_requests_per_hour = 1000;
    bool enabled = true;
};

/**
 * @brief Rate limiter for client requests
 */
class RateLimiter {
public:
    RateLimiter(const RateLimitConfig& config);
    
    bool allow_request(const std::string& client_ip);
    void reset_client(const std::string& client_ip);
    int get_remaining_requests(const std::string& client_ip) const;
    
private:
    struct ClientStats {
        std::chrono::system_clock::time_point last_request;
        int requests_in_minute = 0;
        int requests_in_hour = 0;
        std::chrono::system_clock::time_point minute_start;
        std::chrono::system_clock::time_point hour_start;
    };
    
    RateLimitConfig config_;
    mutable std::mutex mutex_;
    std::map<std::string, ClientStats> client_stats_;
    
    void cleanup_old_entries();
};

/**
 * @brief JWT token manager for authentication
 */
class JWTManager {
public:
    JWTManager(const std::string& secret_key);
    
    std::string generate_token(const std::string& user_id, 
                               const std::vector<std::string>& permissions,
                               int expiry_hours = 24);
    
    AuthResult verify_token(const std::string& token);
    
private:
    std::string secret_key_;
    
    std::string base64_encode(const std::string& input);
    std::string base64_decode(const std::string& input);
    std::string hmac_sha256(const std::string& data);
};

/**
 * @brief REST API Server configuration
 */
struct RESTServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    std::string api_version = "v1";
    bool enable_authentication = false;
    std::string jwt_secret = "change-this-secret";
    RateLimitConfig rate_limit;
    bool enable_cors = true;
    std::vector<std::string> allowed_origins = {"*"};
    int max_body_size_mb = 10;
    int timeout_seconds = 30;
};

/**
 * @brief API usage statistics
 */
struct APIStats {
    size_t total_requests = 0;
    size_t successful_requests = 0;
    size_t failed_requests = 0;
    size_t rate_limited_requests = 0;
    size_t auth_failures = 0;
    std::chrono::system_clock::time_point start_time;
    
    double get_success_rate() const {
        return total_requests > 0 ? 
            (double)successful_requests / total_requests * 100.0 : 0.0;
    }
    
    double get_uptime_hours() const {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::hours>(
            now - start_time);
        return duration.count();
    }
};

/**
 * @brief REST API Server for FDC_Scheduler
 * 
 * Provides HTTP REST API access to railway scheduling services:
 * - Network management (CRUD operations)
 * - Conflict detection and resolution
 * - Schedule optimization
 * - Real-time monitoring
 * 
 * Features:
 * - JWT authentication
 * - Rate limiting
 * - CORS support
 * - API versioning
 * - OpenAPI/Swagger documentation
 * - Health checks and metrics
 */
class RESTServer {
public:
    RESTServer(std::shared_ptr<RailwayNetwork> network,
               const RESTServerConfig& config = RESTServerConfig());
    ~RESTServer();
    
    // Server lifecycle
    void start();
    void stop();
    bool is_running() const;
    
    // Route registration
    void add_route(const std::string& method, 
                   const std::string& path, 
                   RouteHandler handler);
    
    // Authentication
    void set_jwt_manager(std::shared_ptr<JWTManager> jwt_manager);
    std::string generate_api_token(const std::string& user_id,
                                   const std::vector<std::string>& permissions);
    
    // Statistics
    APIStats get_stats() const;
    void reset_stats();
    
    // Configuration
    const RESTServerConfig& get_config() const { return config_; }
    void update_rate_limit(const RateLimitConfig& config);
    
private:
    // Core components
    std::shared_ptr<RailwayNetwork> network_;
    RESTServerConfig config_;
    std::shared_ptr<JWTManager> jwt_manager_;
    std::unique_ptr<RateLimiter> rate_limiter_;
    
    // Server state
    bool running_;
    mutable std::mutex mutex_;
    APIStats stats_;
    
    // Route handling
    std::map<std::string, std::map<std::string, RouteHandler>> routes_;
    
    // Built-in routes
    void register_default_routes();
    
    // Network endpoints
    HTTPResponse handle_get_network(const HTTPRequest& req);
    HTTPResponse handle_get_nodes(const HTTPRequest& req);
    HTTPResponse handle_get_node(const HTTPRequest& req);
    HTTPResponse handle_add_node(const HTTPRequest& req);
    HTTPResponse handle_update_node(const HTTPRequest& req);
    HTTPResponse handle_delete_node(const HTTPRequest& req);
    HTTPResponse handle_get_edges(const HTTPRequest& req);
    HTTPResponse handle_add_edge(const HTTPRequest& req);
    HTTPResponse handle_delete_edge(const HTTPRequest& req);
    
    // Schedule endpoints
    HTTPResponse handle_get_schedules(const HTTPRequest& req);
    HTTPResponse handle_add_schedule(const HTTPRequest& req);
    HTTPResponse handle_get_schedule(const HTTPRequest& req);
    HTTPResponse handle_update_schedule(const HTTPRequest& req);
    HTTPResponse handle_delete_schedule(const HTTPRequest& req);
    
    // Conflict detection endpoints
    HTTPResponse handle_detect_conflicts(const HTTPRequest& req);
    HTTPResponse handle_resolve_conflicts(const HTTPRequest& req);
    
    // Pathfinding endpoints
    HTTPResponse handle_find_path(const HTTPRequest& req);
    HTTPResponse handle_find_k_paths(const HTTPRequest& req);
    
    // Optimization endpoints
    HTTPResponse handle_optimize_speed(const HTTPRequest& req);
    HTTPResponse handle_optimize_route(const HTTPRequest& req);
    HTTPResponse handle_realtime_optimize(const HTTPRequest& req);
    
    // System endpoints
    HTTPResponse handle_health_check(const HTTPRequest& req);
    HTTPResponse handle_stats(const HTTPRequest& req);
    HTTPResponse handle_openapi_spec(const HTTPRequest& req);
    
    // Authentication endpoints
    HTTPResponse handle_login(const HTTPRequest& req);
    HTTPResponse handle_refresh_token(const HTTPRequest& req);
    
    // Middleware
    HTTPResponse apply_middleware(const HTTPRequest& req, 
                                  const RouteHandler& handler);
    bool check_authentication(const HTTPRequest& req, AuthResult& auth);
    bool check_rate_limit(const HTTPRequest& req);
    void add_cors_headers(HTTPResponse& resp);
    
    // Utilities
    HTTPResponse error_response(int status_code, const std::string& message);
    HTTPResponse json_response(const std::string& json, int status_code = 200);
    std::string generate_openapi_spec();
};

/**
 * @brief WebSocket handler for real-time updates (future)
 */
class WebSocketHandler {
public:
    virtual ~WebSocketHandler() = default;
    virtual void on_connect(const std::string& client_id) = 0;
    virtual void on_disconnect(const std::string& client_id) = 0;
    virtual void on_message(const std::string& client_id, 
                           const std::string& message) = 0;
    virtual void broadcast(const std::string& message) = 0;
};

} // namespace fdc_scheduler
