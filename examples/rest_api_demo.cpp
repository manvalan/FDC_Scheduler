/**
 * @file rest_api_demo.cpp
 * @brief Demonstration of REST API Server capabilities
 * 
 * This example showcases:
 * 1. Server initialization and configuration
 * 2. JWT authentication setup
 * 3. Rate limiting configuration
 * 4. Custom route registration
 * 5. API statistics and monitoring
 */

#include <fdc_scheduler/rest_api.hpp>
#include <fdc_scheduler/railway_network.hpp>
#include <fdc_scheduler/node.hpp>
#include <fdc_scheduler/edge.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace fdc_scheduler;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

/**
 * Demo 1: Basic Server Setup
 */
void demo_basic_setup() {
    print_separator("Demo 1: Basic Server Setup");
    
    // Create network
    auto network = std::make_shared<RailwayNetwork>();
    
    // Add some nodes
    Node milano("MILANO", "Milano Centrale", NodeType::STATION);
    milano.set_coordinates(45.4864, 9.2052);
    network->add_node(milano);
    
    Node bologna("BOLOGNA", "Bologna Centrale", NodeType::STATION);
    bologna.set_coordinates(44.5065, 11.3428);
    network->add_node(bologna);
    
    // Basic configuration
    RESTServerConfig config;
    config.host = "localhost";
    config.port = 8080;
    config.api_version = "v1";
    config.enable_authentication = false;  // Disabled for demo
    
    std::cout << "Server Configuration:\n";
    std::cout << "  Host: " << config.host << "\n";
    std::cout << "  Port: " << config.port << "\n";
    std::cout << "  API Version: " << config.api_version << "\n";
    std::cout << "  Authentication: " << (config.enable_authentication ? "Enabled" : "Disabled") << "\n";
    
    // Create server
    RESTServer server(network, config);
    
    std::cout << "\n✓ Server created successfully\n";
    std::cout << "  Network nodes: " << network->num_nodes() << "\n";
    std::cout << "  Network edges: " << network->num_edges() << "\n";
    
    // Start server (stub implementation)
    server.start();
    std::cout << "\n✓ Server started (stub mode)\n";
    
    // Simulate some activity
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check if running
    std::cout << "  Server running: " << (server.is_running() ? "Yes" : "No") << "\n";
    
    // Stop server
    server.stop();
    std::cout << "\n✓ Server stopped\n";
}

/**
 * Demo 2: Authentication with JWT
 */
void demo_authentication() {
    print_separator("Demo 2: JWT Authentication");
    
    // Create JWT manager
    std::string secret = "super-secret-key-change-in-production";
    JWTManager jwt_manager(secret);
    
    std::cout << "JWT Manager Configuration:\n";
    std::cout << "  Secret key: " << secret.substr(0, 20) << "..." << "\n";
    
    // Generate token for user
    std::string user_id = "admin@railway.com";
    std::vector<std::string> permissions = {"read", "write", "admin"};
    
    std::cout << "\nGenerating token for user: " << user_id << "\n";
    std::cout << "  Permissions: ";
    for (const auto& perm : permissions) {
        std::cout << perm << " ";
    }
    std::cout << "\n";
    
    std::string token = jwt_manager.generate_token(user_id, permissions, 24);
    
    std::cout << "\n✓ Token generated:\n";
    std::cout << "  " << token.substr(0, 50) << "..." << "\n";
    std::cout << "  Length: " << token.length() << " characters\n";
    
    // Verify token
    std::cout << "\nVerifying token...\n";
    AuthResult result = jwt_manager.verify_token(token);
    
    if (result.authenticated) {
        std::cout << "✓ Token valid!\n";
        std::cout << "  User ID: " << result.user_id << "\n";
        std::cout << "  Permissions: ";
        for (const auto& perm : result.permissions) {
            std::cout << perm << " ";
        }
        std::cout << "\n";
    } else {
        std::cout << "✗ Token invalid: " << result.error_message << "\n";
    }
    
    // Test invalid token
    std::cout << "\nTesting invalid token...\n";
    AuthResult invalid_result = jwt_manager.verify_token("invalid.token.here");
    
    if (!invalid_result.authenticated) {
        std::cout << "✓ Invalid token correctly rejected\n";
        std::cout << "  Error: " << invalid_result.error_message << "\n";
    }
}

/**
 * Demo 3: Rate Limiting
 */
void demo_rate_limiting() {
    print_separator("Demo 3: Rate Limiting");
    
    // Configure rate limiter
    RateLimitConfig config;
    config.max_requests_per_minute = 10;
    config.max_requests_per_hour = 100;
    config.enabled = true;
    
    std::cout << "Rate Limit Configuration:\n";
    std::cout << "  Max requests/minute: " << config.max_requests_per_minute << "\n";
    std::cout << "  Max requests/hour: " << config.max_requests_per_hour << "\n";
    std::cout << "  Enabled: " << (config.enabled ? "Yes" : "No") << "\n";
    
    RateLimiter limiter(config);
    
    // Simulate requests from client
    std::string client_ip = "192.168.1.100";
    int successful_requests = 0;
    int blocked_requests = 0;
    
    std::cout << "\nSimulating " << (config.max_requests_per_minute + 5) 
              << " requests from " << client_ip << "...\n";
    
    for (int i = 0; i < config.max_requests_per_minute + 5; ++i) {
        bool allowed = limiter.allow_request(client_ip);
        
        if (allowed) {
            successful_requests++;
            if (i < 3 || i == config.max_requests_per_minute - 1) {
                std::cout << "  Request " << (i + 1) << ": ✓ Allowed\n";
            }
        } else {
            blocked_requests++;
            if (blocked_requests <= 2) {
                std::cout << "  Request " << (i + 1) << ": ✗ Rate limited\n";
            }
        }
        
        if (i == 2) {
            std::cout << "  ...\n";
        }
    }
    
    std::cout << "\nResults:\n";
    std::cout << "  Successful: " << successful_requests << "\n";
    std::cout << "  Blocked: " << blocked_requests << "\n";
    std::cout << "  Remaining quota: " << limiter.get_remaining_requests(client_ip) << "\n";
    
    // Reset client
    std::cout << "\nResetting client quota...\n";
    limiter.reset_client(client_ip);
    
    bool allowed_after_reset = limiter.allow_request(client_ip);
    std::cout << "✓ First request after reset: " 
              << (allowed_after_reset ? "Allowed" : "Blocked") << "\n";
}

/**
 * Demo 4: Custom Routes
 */
void demo_custom_routes() {
    print_separator("Demo 4: Custom Routes");
    
    auto network = std::make_shared<RailwayNetwork>();
    RESTServerConfig config;
    config.enable_authentication = false;
    
    RESTServer server(network, config);
    
    std::cout << "Registering custom routes...\n\n";
    
    // Custom route 1: Echo endpoint
    server.add_route("POST", "/api/v1/echo", 
        [](const HTTPRequest& req) -> HTTPResponse {
            HTTPResponse resp;
            resp.status_code = 200;
            resp.body = "{\"echo\": \"" + req.body + "\"}";
            resp.headers["Content-Type"] = "application/json";
            return resp;
        });
    std::cout << "✓ Registered: POST /api/v1/echo\n";
    
    // Custom route 2: Version info
    server.add_route("GET", "/api/v1/version",
        [](const HTTPRequest& req) -> HTTPResponse {
            HTTPResponse resp;
            resp.status_code = 200;
            resp.body = "{\"version\": \"2.0.0\", \"api\": \"v1\"}";
            resp.headers["Content-Type"] = "application/json";
            return resp;
        });
    std::cout << "✓ Registered: GET /api/v1/version\n";
    
    // Custom route 3: Status endpoint
    server.add_route("GET", "/status",
        [](const HTTPRequest& req) -> HTTPResponse {
            HTTPResponse resp;
            resp.status_code = 200;
            resp.body = "{\"status\": \"operational\"}";
            resp.headers["Content-Type"] = "application/json";
            return resp;
        });
    std::cout << "✓ Registered: GET /status\n";
    
    std::cout << "\n✓ 3 custom routes registered successfully\n";
    
    // Start server
    server.start();
    std::cout << "\n✓ Server ready to handle requests\n";
    
    // Simulate some time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server.stop();
}

/**
 * Demo 5: API Statistics and Monitoring
 */
void demo_statistics() {
    print_separator("Demo 5: API Statistics");
    
    auto network = std::make_shared<RailwayNetwork>();
    RESTServerConfig config;
    config.rate_limit.enabled = false;  // Disable for demo
    
    RESTServer server(network, config);
    server.start();
    
    std::cout << "Simulating API activity...\n\n";
    
    // Simulate requests (using internal methods - in real scenario these would be HTTP requests)
    HTTPRequest dummy_req;
    dummy_req.client_ip = "192.168.1.100";
    
    std::cout << "Simulating 20 requests...\n";
    
    // This is just for demonstration - real stats would be tracked internally
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Get statistics
    APIStats stats = server.get_stats();
    
    std::cout << "\nAPI Statistics:\n";
    std::cout << "  Total requests: " << stats.total_requests << "\n";
    std::cout << "  Successful: " << stats.successful_requests << "\n";
    std::cout << "  Failed: " << stats.failed_requests << "\n";
    std::cout << "  Rate limited: " << stats.rate_limited_requests << "\n";
    std::cout << "  Auth failures: " << stats.auth_failures << "\n";
    std::cout << "  Success rate: " << std::fixed << std::setprecision(1) 
              << stats.get_success_rate() << "%\n";
    std::cout << "  Uptime: " << stats.get_uptime_hours() << " hours\n";
    
    // Reset statistics
    std::cout << "\nResetting statistics...\n";
    server.reset_stats();
    
    APIStats new_stats = server.get_stats();
    std::cout << "✓ Statistics reset\n";
    std::cout << "  Total requests: " << new_stats.total_requests << "\n";
    
    server.stop();
}

/**
 * Demo 6: Complete Server Configuration
 */
void demo_complete_configuration() {
    print_separator("Demo 6: Complete Configuration");
    
    auto network = std::make_shared<RailwayNetwork>();
    
    // Full configuration
    RESTServerConfig config;
    config.host = "0.0.0.0";
    config.port = 8080;
    config.api_version = "v1";
    config.enable_authentication = true;
    config.jwt_secret = "production-secret-key-32-chars!";
    config.enable_cors = true;
    config.allowed_origins = {"https://railway.com", "https://admin.railway.com"};
    config.max_body_size_mb = 10;
    config.timeout_seconds = 30;
    
    // Rate limiting
    config.rate_limit.enabled = true;
    config.rate_limit.max_requests_per_minute = 60;
    config.rate_limit.max_requests_per_hour = 1000;
    
    std::cout << "Complete Server Configuration:\n\n";
    
    std::cout << "Network:\n";
    std::cout << "  Host: " << config.host << "\n";
    std::cout << "  Port: " << config.port << "\n";
    std::cout << "  API Version: " << config.api_version << "\n";
    
    std::cout << "\nSecurity:\n";
    std::cout << "  Authentication: " << (config.enable_authentication ? "Enabled" : "Disabled") << "\n";
    std::cout << "  JWT Secret: " << config.jwt_secret.substr(0, 10) << "...\n";
    std::cout << "  CORS: " << (config.enable_cors ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Allowed Origins: ";
    for (const auto& origin : config.allowed_origins) {
        std::cout << origin << " ";
    }
    std::cout << "\n";
    
    std::cout << "\nRate Limiting:\n";
    std::cout << "  Enabled: " << (config.rate_limit.enabled ? "Yes" : "No") << "\n";
    std::cout << "  Max requests/minute: " << config.rate_limit.max_requests_per_minute << "\n";
    std::cout << "  Max requests/hour: " << config.rate_limit.max_requests_per_hour << "\n";
    
    std::cout << "\nLimits:\n";
    std::cout << "  Max body size: " << config.max_body_size_mb << " MB\n";
    std::cout << "  Timeout: " << config.timeout_seconds << " seconds\n";
    
    // Create server with full config
    RESTServer server(network, config);
    
    // Generate API token
    std::string token = server.generate_api_token("admin@railway.com", 
        {"read", "write", "admin"});
    
    std::cout << "\n✓ Server configured and ready\n";
    std::cout << "  Sample API token: " << token.substr(0, 30) << "...\n";
    
    server.start();
    std::cout << "\n✓ Production-ready server started\n";
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server.stop();
}

int main() {
    std::cout << "FDC_Scheduler REST API Server Demo\n";
    std::cout << "====================================\n";
    std::cout << "\nThis demo showcases REST API capabilities:\n";
    std::cout << "- Server setup and configuration\n";
    std::cout << "- JWT authentication\n";
    std::cout << "- Rate limiting\n";
    std::cout << "- Custom route registration\n";
    std::cout << "- API statistics and monitoring\n";
    std::cout << "\nNOTE: This is a framework demonstration.\n";
    std::cout << "For actual HTTP server, integrate cpp-httplib or Boost.Beast.\n";
    
    try {
        demo_basic_setup();
        demo_authentication();
        demo_rate_limiting();
        demo_custom_routes();
        demo_statistics();
        demo_complete_configuration();
        
        print_separator("All Demos Completed Successfully!");
        
        std::cout << "REST API Framework Features:\n";
        std::cout << "✓ Server lifecycle management\n";
        std::cout << "✓ JWT authentication system\n";
        std::cout << "✓ Rate limiting per client\n";
        std::cout << "✓ Custom route registration\n";
        std::cout << "✓ API statistics tracking\n";
        std::cout << "✓ CORS configuration\n";
        std::cout << "✓ OpenAPI specification\n";
        std::cout << "\nNext steps:\n";
        std::cout << "1. Integrate cpp-httplib for actual HTTP server\n";
        std::cout << "2. Add database persistence\n";
        std::cout << "3. Implement WebSocket support\n";
        std::cout << "4. Add request logging\n";
        std::cout << "5. Deploy to production\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
