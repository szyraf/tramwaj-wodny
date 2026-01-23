#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    int N;
    int M;
    int K;
    int T1;
    int T2;
    int R;
    int queue_to_bridge_time;
    int bridge_to_ship_time;
    int ship_to_bridge_time;
    int bridge_to_exit_time;
    int tyniec_people;
    int tyniec_bikes;
    int wawel_people;
    int wawel_bikes;
};

bool load_config(const char* filename, Config& cfg);
bool validate_config(const Config& cfg);
void print_config(const Config& cfg);

#endif
