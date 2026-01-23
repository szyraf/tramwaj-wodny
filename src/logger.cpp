#include "logger.h"
#include <sys/time.h>
#include <sys/file.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>

void init_logger(SharedState* state) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    state->start_time_sec = tv.tv_sec;
    state->start_time_usec = tv.tv_usec;
    
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    snprintf(state->log_file, sizeof(state->log_file), 
             "simulation_%04d%02d%02d_%02d%02d%02d.log",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

std::string get_timestamp(SharedState* state) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    
    long elapsed_usec = (tv.tv_sec - state->start_time_sec) * 1000000L + 
                        (tv.tv_usec - state->start_time_usec);
    
    int hours = (int)(elapsed_usec / 3600000000L);
    int mins = (int)((elapsed_usec % 3600000000L) / 60000000L);
    int secs = (int)((elapsed_usec % 60000000L) / 1000000L);
    int msecs = (int)((elapsed_usec % 1000000L) / 1000L);
    
    char buf[32];
    snprintf(buf, sizeof(buf), "[%02d:%02d:%02d.%03d]", hours, mins, secs, msecs);
    return std::string(buf);
}

void log_msg(SharedState* state, const char* source, const char* format, ...) {
    char msg[512];
    va_list args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);
    
    std::string ts = get_timestamp(state);
    
    FILE* f = fopen(state->log_file, "a");
    if (f) {
        flock(fileno(f), LOCK_EX);
        fprintf(f, "%s [%s] %s\n", ts.c_str(), source, msg);
        fflush(f);
        flock(fileno(f), LOCK_UN);
        fclose(f);
    }
    
    printf("%s [%s] %s\n", ts.c_str(), source, msg);
    fflush(stdout);
}

const char* location_name(Location loc) {
    return loc == TYNIEC ? "TYNIEC" : "WAWEL";
}
