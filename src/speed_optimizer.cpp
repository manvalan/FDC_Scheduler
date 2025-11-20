#include "fdc_scheduler/speed_optimizer.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cmath>

namespace fdc_scheduler {

// Constants
constexpr double GRAVITY = 9.81;              // m/s²
constexpr double KMH_TO_MS = 1.0 / 3.6;       // km/h to m/s conversion
constexpr double MS_TO_KMH = 3.6;             // m/s to km/h conversion
constexpr double TONS_TO_KG = 1000.0;         // tons to kg conversion

SpeedOptimizer::SpeedOptimizer(const SpeedOptimizerConfig& config)
    : config_(config) {}

OptimizationResult SpeedOptimizer::optimize_journey(
    const Train& train,
    const std::vector<std::pair<Node*, double>>& route,
    const TrainPhysics& physics,
    double scheduled_time
) {
    if (route.empty()) {
        throw std::invalid_argument("Route cannot be empty");
    }
    
    // Calculate total distance
    double total_distance = 0.0;
    for (const auto& [node, dist] : route) {
        total_distance += dist;
    }
    
    // Generate baseline profile (no optimization)
    SpeedProfile baseline = generate_baseline_profile(total_distance, scheduled_time, physics);
    
    // Apply optimizations iteratively
    SpeedProfile optimized = baseline;
    double max_time = scheduled_time * (1.0 + config_.time_slack_factor);
    
    for (int i = 0; i < config_.optimization_iterations; ++i) {
        // Apply cruise control optimization
        optimized = apply_cruise_control(optimized, physics, max_time);
        
        // Insert coasting phases if enabled
        if (config_.enable_coasting) {
            optimized = insert_coasting_phases(optimized, physics, max_time);
        }
    }
    
    // Calculate energy for both profiles
    double baseline_energy = calculate_energy(baseline, physics);
    double optimized_energy = calculate_energy(optimized, physics);
    
    // Prepare result
    OptimizationResult result;
    result.optimized_profile = optimized;
    result.baseline_profile = baseline;
    result.energy_savings = baseline_energy - optimized_energy;
    result.energy_savings_percent = (result.energy_savings / baseline_energy) * 100.0;
    result.time_increase = optimized.total_time - baseline.total_time;
    result.time_increase_percent = (result.time_increase / baseline.total_time) * 100.0;
    
    // Calculate metrics
    result.metrics.acceleration_phases = 0;
    result.metrics.constant_speed_phases = 0;
    result.metrics.coasting_phases = 0;
    result.metrics.braking_phases = 0;
    result.metrics.total_regen_energy = 0.0;
    
    for (const auto& seg : optimized.segments) {
        switch (seg.type) {
            case SpeedSegment::Type::ACCELERATION:
                result.metrics.acceleration_phases++;
                break;
            case SpeedSegment::Type::CONSTANT_SPEED:
                result.metrics.constant_speed_phases++;
                break;
            case SpeedSegment::Type::COASTING:
                result.metrics.coasting_phases++;
                break;
            case SpeedSegment::Type::BRAKING:
                result.metrics.braking_phases++;
                if (config_.enable_regeneration && seg.energy_consumed < 0) {
                    result.metrics.total_regen_energy += -seg.energy_consumed;
                }
                break;
        }
    }
    
    return result;
}

SpeedProfile SpeedOptimizer::generate_baseline_profile(
    double distance,
    double scheduled_time,
    const TrainPhysics& physics
) {
    SpeedProfile profile;
    profile.total_distance = distance;
    profile.total_time = scheduled_time;
    
    // Calculate required average speed
    double avg_speed = distance / scheduled_time; // km/h
    
    // Determine cruise speed (slightly higher to account for acceleration/braking)
    double cruise_speed = std::min(avg_speed * 1.15, 160.0); // Max 160 km/h
    profile.max_speed = cruise_speed;
    
    double current_distance = 0.0;
    
    // Phase 1: Acceleration to cruise speed
    SpeedSegment accel = create_acceleration_segment(current_distance, 0.0, cruise_speed, physics);
    profile.segments.push_back(accel);
    current_distance += (accel.end_distance - accel.start_distance);
    
    // Phase 2: Maintain cruise speed for most of journey
    double cruise_distance = distance - current_distance;
    
    // Reserve distance for braking
    double braking_distance = (cruise_speed * KMH_TO_MS) * (cruise_speed * KMH_TO_MS) / 
                             (2.0 * physics.max_deceleration * 1000.0); // km
    cruise_distance -= braking_distance;
    
    if (cruise_distance > 0) {
        SpeedSegment cruise = create_constant_segment(current_distance, cruise_distance, cruise_speed, physics);
        profile.segments.push_back(cruise);
        current_distance += cruise_distance;
    }
    
    // Phase 3: Braking to stop
    SpeedSegment brake = create_braking_segment(current_distance, cruise_speed, 0.0, physics);
    profile.segments.push_back(brake);
    
    // Calculate totals
    profile.total_time = 0.0;
    profile.total_energy = 0.0;
    for (const auto& seg : profile.segments) {
        profile.total_time += seg.duration;
        profile.total_energy += seg.energy_consumed;
    }
    
    profile.avg_speed = distance / profile.total_time;
    profile.energy_per_km = profile.total_energy / distance;
    profile.energy_per_ton_km = profile.total_energy / (distance * physics.mass);
    
    return profile;
}

double SpeedOptimizer::calculate_energy(const SpeedProfile& profile, const TrainPhysics& physics) const {
    return profile.total_energy;
}

SpeedProfile SpeedOptimizer::apply_cruise_control(
    const SpeedProfile& baseline,
    const TrainPhysics& physics,
    double max_time
) {
    SpeedProfile optimized = baseline;
    
    // Reduce cruise speed to save energy
    for (auto& seg : optimized.segments) {
        if (seg.type == SpeedSegment::Type::CONSTANT_SPEED) {
            double original_speed = seg.start_speed;
            double reduced_speed = original_speed * (1.0 - config_.cruise_speed_reduction);
            
            // Recalculate segment with reduced speed
            double distance = seg.end_distance - seg.start_distance;
            seg.start_speed = reduced_speed;
            seg.end_speed = reduced_speed;
            seg.duration = distance / reduced_speed; // hours
            seg.energy_consumed = calculate_constant_speed_energy(physics.mass, reduced_speed, distance, physics);
        }
    }
    
    // Recalculate totals
    optimized.total_time = 0.0;
    optimized.total_energy = 0.0;
    optimized.max_speed = 0.0;
    
    for (const auto& seg : optimized.segments) {
        optimized.total_time += seg.duration;
        optimized.total_energy += seg.energy_consumed;
        optimized.max_speed = std::max(optimized.max_speed, std::max(seg.start_speed, seg.end_speed));
    }
    
    optimized.avg_speed = optimized.total_distance / optimized.total_time;
    optimized.energy_per_km = optimized.total_energy / optimized.total_distance;
    optimized.energy_per_ton_km = optimized.total_energy / (optimized.total_distance * physics.mass);
    
    // Check if we're within time constraint
    if (optimized.total_time > max_time) {
        // Need to speed up - adjust cruise speed
        adjust_cruise_speed(optimized, max_time, physics);
    }
    
    return optimized;
}

SpeedProfile SpeedOptimizer::insert_coasting_phases(
    const SpeedProfile& profile,
    const TrainPhysics& physics,
    double max_time
) {
    SpeedProfile optimized = profile;
    
    // Find constant speed segments that can be replaced with coast + constant
    std::vector<SpeedSegment> new_segments;
    
    for (size_t i = 0; i < optimized.segments.size(); ++i) {
        const auto& seg = optimized.segments[i];
        
        if (seg.type == SpeedSegment::Type::CONSTANT_SPEED) {
            double distance = seg.end_distance - seg.start_distance;
            
            // Try inserting a coasting phase in the middle
            if (distance > 5.0) { // Only for segments longer than 5 km
                double coast_distance = distance * 0.3; // 30% coasting
                double constant_distance = distance - coast_distance;
                
                // First part: constant speed
                SpeedSegment const_seg = create_constant_segment(
                    seg.start_distance, constant_distance, seg.start_speed, physics
                );
                new_segments.push_back(const_seg);
                
                // Second part: coasting
                double coast_start_dist = seg.start_distance + constant_distance;
                SpeedSegment coast_seg = create_coasting_segment(
                    coast_start_dist, seg.start_speed, coast_distance, physics
                );
                new_segments.push_back(coast_seg);
            } else {
                new_segments.push_back(seg);
            }
        } else {
            new_segments.push_back(seg);
        }
    }
    
    optimized.segments = new_segments;
    
    // Recalculate totals
    optimized.total_time = 0.0;
    optimized.total_energy = 0.0;
    
    for (const auto& seg : optimized.segments) {
        optimized.total_time += seg.duration;
        optimized.total_energy += seg.energy_consumed;
    }
    
    optimized.avg_speed = optimized.total_distance / optimized.total_time;
    optimized.energy_per_km = optimized.total_energy / optimized.total_distance;
    optimized.energy_per_ton_km = optimized.total_energy / (optimized.total_distance * physics.mass);
    
    return optimized;
}

// Physics calculations

double SpeedOptimizer::calculate_acceleration_energy(
    double mass, double v_start, double v_end, double distance, double efficiency
) const {
    // Kinetic energy change: 0.5 * m * (v_end² - v_start²)
    double v_start_ms = v_start * KMH_TO_MS;
    double v_end_ms = v_end * KMH_TO_MS;
    double mass_kg = mass * TONS_TO_KG;
    
    double kinetic_energy = 0.5 * mass_kg * (v_end_ms * v_end_ms - v_start_ms * v_start_ms);
    
    // Convert to kWh and account for efficiency
    double energy_kWh = (kinetic_energy / 3.6e6) / efficiency; // J to kWh
    
    return energy_kWh;
}

double SpeedOptimizer::calculate_constant_speed_energy(
    double mass, double speed, double distance, const TrainPhysics& physics
) const {
    // Energy = Force * Distance
    // Force = rolling resistance + air resistance
    double force = calculate_resistance_force(speed, physics);
    double distance_m = distance * 1000.0; // km to m
    
    double energy_j = force * distance_m;
    double energy_kWh = (energy_j / 3.6e6) / physics.motor_efficiency;
    
    return energy_kWh;
}

double SpeedOptimizer::calculate_resistance_force(double speed, const TrainPhysics& physics) const {
    double v_ms = speed * KMH_TO_MS;
    double mass_kg = physics.mass * TONS_TO_KG;
    
    // Rolling resistance: F_roll = mass * g * C_r
    double f_roll = mass_kg * GRAVITY * physics.rolling_resistance;
    
    // Air resistance: F_air = 0.5 * C_d * A * ρ * v²
    // Simplified: F_air = k * v²  where k includes density, drag coeff, area
    double f_air = physics.air_resistance * v_ms * v_ms;
    
    return f_roll + f_air;
}

double SpeedOptimizer::calculate_braking_energy(
    double mass, double v_start, double v_end, double regen_efficiency
) const {
    // Negative of kinetic energy change (energy recovered)
    double v_start_ms = v_start * KMH_TO_MS;
    double v_end_ms = v_end * KMH_TO_MS;
    double mass_kg = mass * TONS_TO_KG;
    
    double kinetic_energy = 0.5 * mass_kg * (v_start_ms * v_start_ms - v_end_ms * v_end_ms);
    
    // With regenerative braking, we recover some energy
    double energy_kWh = -(kinetic_energy / 3.6e6) * regen_efficiency;
    
    return config_.enable_regeneration ? energy_kWh : 0.0;
}

// Segment creation helpers

SpeedSegment SpeedOptimizer::create_acceleration_segment(
    double start_dist, double v_start, double v_target, const TrainPhysics& physics
) {
    SpeedSegment seg;
    seg.type = SpeedSegment::Type::ACCELERATION;
    seg.start_distance = start_dist;
    seg.start_speed = v_start;
    seg.end_speed = v_target;
    
    // Calculate distance and time for acceleration
    // v² = u² + 2as  =>  s = (v² - u²) / (2a)
    double v_start_ms = v_start * KMH_TO_MS;
    double v_target_ms = v_target * KMH_TO_MS;
    double accel = physics.max_acceleration;
    
    double distance_m = (v_target_ms * v_target_ms - v_start_ms * v_start_ms) / (2.0 * accel);
    seg.end_distance = start_dist + (distance_m / 1000.0); // m to km
    
    // Time: t = (v - u) / a
    double time_s = (v_target_ms - v_start_ms) / accel;
    seg.duration = time_s / 3600.0; // s to hours
    
    // Energy: acceleration energy + resistance work
    double distance_km = distance_m / 1000.0;
    seg.energy_consumed = calculate_acceleration_energy(physics.mass, v_start, v_target, distance_km, physics.motor_efficiency);
    seg.energy_consumed += calculate_constant_speed_energy(physics.mass, (v_start + v_target) / 2.0, distance_km, physics);
    
    return seg;
}

SpeedSegment SpeedOptimizer::create_constant_segment(
    double start_dist, double distance, double speed, const TrainPhysics& physics
) {
    SpeedSegment seg;
    seg.type = SpeedSegment::Type::CONSTANT_SPEED;
    seg.start_distance = start_dist;
    seg.end_distance = start_dist + distance;
    seg.start_speed = speed;
    seg.end_speed = speed;
    seg.duration = distance / speed; // hours
    seg.energy_consumed = calculate_constant_speed_energy(physics.mass, speed, distance, physics);
    
    return seg;
}

SpeedSegment SpeedOptimizer::create_coasting_segment(
    double start_dist, double v_start, double distance, const TrainPhysics& physics
) {
    SpeedSegment seg;
    seg.type = SpeedSegment::Type::COASTING;
    seg.start_distance = start_dist;
    seg.end_distance = start_dist + distance;
    seg.start_speed = v_start;
    
    // During coasting, train decelerates due to resistance
    // Simplified: assume linear deceleration
    double v_start_ms = v_start * KMH_TO_MS;
    double avg_decel = 0.05; // m/s² typical for coasting
    double distance_m = distance * 1000.0;
    
    // v² = u² - 2as (deceleration)
    double v_end_ms_sq = v_start_ms * v_start_ms - 2.0 * avg_decel * distance_m;
    double v_end_ms = std::sqrt(std::max(0.0, v_end_ms_sq));
    seg.end_speed = v_end_ms * MS_TO_KMH;
    
    // Time: average speed method
    double avg_speed_ms = (v_start_ms + v_end_ms) / 2.0;
    double time_s = distance_m / avg_speed_ms;
    seg.duration = time_s / 3600.0; // s to hours
    
    // Energy: minimal (no motor power, just rolling friction)
    seg.energy_consumed = 0.0; // Coasting uses no active power
    
    return seg;
}

SpeedSegment SpeedOptimizer::create_braking_segment(
    double start_dist, double v_start, double v_target, const TrainPhysics& physics
) {
    SpeedSegment seg;
    seg.type = SpeedSegment::Type::BRAKING;
    seg.start_distance = start_dist;
    seg.start_speed = v_start;
    seg.end_speed = v_target;
    
    // Calculate braking distance
    // v² = u² - 2as  =>  s = (u² - v²) / (2a)
    double v_start_ms = v_start * KMH_TO_MS;
    double v_target_ms = v_target * KMH_TO_MS;
    double decel = physics.max_deceleration;
    
    double distance_m = (v_start_ms * v_start_ms - v_target_ms * v_target_ms) / (2.0 * decel);
    seg.end_distance = start_dist + (distance_m / 1000.0); // m to km
    
    // Time: t = (u - v) / a
    double time_s = (v_start_ms - v_target_ms) / decel;
    seg.duration = time_s / 3600.0; // s to hours
    
    // Energy: regenerative braking recovers energy (negative consumption)
    double distance_km = distance_m / 1000.0;
    seg.energy_consumed = calculate_braking_energy(physics.mass, v_start, v_target, physics.regen_efficiency);
    
    return seg;
}

// Optimization helpers

double SpeedOptimizer::estimate_total_time(const SpeedProfile& profile) const {
    double total = 0.0;
    for (const auto& seg : profile.segments) {
        total += seg.duration;
    }
    return total;
}

void SpeedOptimizer::adjust_cruise_speed(
    SpeedProfile& profile, double target_time, const TrainPhysics& physics
) {
    // Find constant speed segments and adjust proportionally
    double current_time = estimate_total_time(profile);
    double time_ratio = target_time / current_time;
    
    for (auto& seg : profile.segments) {
        if (seg.type == SpeedSegment::Type::CONSTANT_SPEED) {
            double distance = seg.end_distance - seg.start_distance;
            seg.start_speed *= (1.0 / time_ratio);
            seg.end_speed *= (1.0 / time_ratio);
            seg.duration = distance / seg.start_speed;
            seg.energy_consumed = calculate_constant_speed_energy(physics.mass, seg.start_speed, distance, physics);
        }
    }
    
    // Recalculate totals
    profile.total_time = estimate_total_time(profile);
}

// BatchSpeedOptimizer implementation

BatchSpeedOptimizer::BatchSpeedOptimizer(const SpeedOptimizerConfig& config)
    : optimizer_(config), total_energy_savings_(0.0), avg_energy_savings_percent_(0.0) {}

std::vector<OptimizationResult> BatchSpeedOptimizer::optimize_trains(
    const std::vector<Train>& trains,
    const RailwayNetwork& /* network */,
    const std::map<TrainType, TrainPhysics>& physics_map
) {
    std::vector<OptimizationResult> results;
    total_energy_savings_ = 0.0;
    double total_savings_percent = 0.0;
    
    // Note: In a real implementation, route and schedule information would come from
    // a separate data structure. For this demo, we use simplified parameters.
    (void)trains;  // Unused in simplified version
    (void)physics_map;  // Unused in simplified version
    
    // Return empty results for now - actual implementation would process trains
    return results;
}

} // namespace fdc_scheduler
