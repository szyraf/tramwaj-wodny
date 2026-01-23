#include "config.h"
#include "common.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/resource.h>
#include <cerrno>

bool load_config(const char* filename, Config& cfg) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config file: " << filename << std::endl;
        return false;
    }
    
    cfg = {0};
    std::string line;
    
    while (std::getline(file, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos)
            line = line.substr(0, comment_pos);
        
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!key.empty() && (key.front() == ' ' || key.front() == '\t')) key.erase(0, 1);
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(0, 1);
        
        if (key.empty() || value.empty()) continue;
        
        int val;
        try {
            val = std::stoi(value);
        } catch (...) {
            std::cerr << "Error: Invalid value for " << key << ": " << value << std::endl;
            return false;
        }
        
        if (key == "N") cfg.N = val;
        else if (key == "M") cfg.M = val;
        else if (key == "K") cfg.K = val;
        else if (key == "T1") cfg.T1 = val;
        else if (key == "T2") cfg.T2 = val;
        else if (key == "R") cfg.R = val;
        else if (key == "QUEUE_TO_BRIDGE_TIME") cfg.queue_to_bridge_time = val;
        else if (key == "BRIDGE_TO_SHIP_TIME") cfg.bridge_to_ship_time = val;
        else if (key == "SHIP_TO_BRIDGE_TIME") cfg.ship_to_bridge_time = val;
        else if (key == "BRIDGE_TO_EXIT_TIME") cfg.bridge_to_exit_time = val;
        else if (key == "TYNIEC_PEOPLE") cfg.tyniec_people = val;
        else if (key == "TYNIEC_BIKES") cfg.tyniec_bikes = val;
        else if (key == "WAWEL_PEOPLE") cfg.wawel_people = val;
        else if (key == "WAWEL_BIKES") cfg.wawel_bikes = val;
    }
    
    return true;
}

bool validate_config(const Config& cfg) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NPROC, &rl) == -1) {
        perror("getrlimit");
        return false;
    }
    
    int total_passengers = cfg.tyniec_people + cfg.tyniec_bikes + cfg.wawel_people + cfg.wawel_bikes;
    int total_processes = total_passengers + 3;
    
    if (total_passengers > MAX_PASSENGERS) {
        std::cerr << "Error: Total passengers (" << total_passengers << ") cannot exceed " << MAX_PASSENGERS << std::endl;
        return false;
    }
    
    if (total_processes > (int)rl.rlim_cur / 2) {
        std::cerr << "Error: Too many passengers. Max allowed: " << (rl.rlim_cur / 2 - 3) << std::endl;
        return false;
    }
    
    if (cfg.N <= 0) { std::cerr << "Error: N must be positive" << std::endl; return false; }
    if (cfg.N > MAX_PASSENGERS) { std::cerr << "Error: N cannot exceed " << MAX_PASSENGERS << std::endl; return false; }
    if (cfg.M < 0) { std::cerr << "Error: M must be non-negative" << std::endl; return false; }
    if (cfg.M >= cfg.N) { std::cerr << "Error: M must be less than N" << std::endl; return false; }
    if (cfg.K <= 0) { std::cerr << "Error: K must be positive" << std::endl; return false; }
    if (cfg.K > MAX_BRIDGE) { std::cerr << "Error: K cannot exceed " << MAX_BRIDGE << std::endl; return false; }
    if (cfg.K >= cfg.N) { std::cerr << "Error: K must be less than N" << std::endl; return false; }
    if (cfg.R <= 0) { std::cerr << "Error: R must be positive" << std::endl; return false; }
    if (cfg.T1 < 0) { std::cerr << "Error: T1 must be non-negative" << std::endl; return false; }
    if (cfg.T2 < 0) { std::cerr << "Error: T2 must be non-negative" << std::endl; return false; }
    if (cfg.queue_to_bridge_time < 0) { std::cerr << "Error: QUEUE_TO_BRIDGE_TIME must be non-negative" << std::endl; return false; }
    if (cfg.bridge_to_ship_time < 0) { std::cerr << "Error: BRIDGE_TO_SHIP_TIME must be non-negative" << std::endl; return false; }
    if (cfg.ship_to_bridge_time < 0) { std::cerr << "Error: SHIP_TO_BRIDGE_TIME must be non-negative" << std::endl; return false; }
    if (cfg.bridge_to_exit_time < 0) { std::cerr << "Error: BRIDGE_TO_EXIT_TIME must be non-negative" << std::endl; return false; }
    if (cfg.tyniec_people < 0 || cfg.tyniec_bikes < 0) { std::cerr << "Error: Tyniec counts must be non-negative" << std::endl; return false; }
    if (cfg.wawel_people < 0 || cfg.wawel_bikes < 0) { std::cerr << "Error: Wawel counts must be non-negative" << std::endl; return false; }
    if (cfg.tyniec_bikes > cfg.tyniec_people + cfg.tyniec_bikes) { std::cerr << "Error: Invalid Tyniec bike count" << std::endl; return false; }
    
    return true;
}

void print_config(const Config& cfg) {
    std::cout << "\n=== Configuration ===" << std::endl;
    std::cout << "Ship capacity (people): N=" << cfg.N << std::endl;
    std::cout << "Ship capacity (bikes):  M=" << cfg.M << std::endl;
    std::cout << "Bridge capacity:        K=" << cfg.K << std::endl;
    std::cout << "Loading time:           T1=" << cfg.T1 << " ms" << std::endl;
    std::cout << "Travel time:            T2=" << cfg.T2 << " ms" << std::endl;
    std::cout << "Max trips:              R=" << cfg.R << std::endl;
    std::cout << "Transition times (ms):  queue->bridge=" << cfg.queue_to_bridge_time 
              << ", bridge->ship=" << cfg.bridge_to_ship_time
              << ", ship->bridge=" << cfg.ship_to_bridge_time
              << ", bridge->exit=" << cfg.bridge_to_exit_time << std::endl;
    std::cout << "Tyniec: " << cfg.tyniec_people << " people, " << cfg.tyniec_bikes << " with bikes" << std::endl;
    std::cout << "Wawel:  " << cfg.wawel_people << " people, " << cfg.wawel_bikes << " with bikes" << std::endl;
    std::cout << "=====================\n" << std::endl;
}
