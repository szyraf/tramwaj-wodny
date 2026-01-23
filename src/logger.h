#ifndef LOGGER_H
#define LOGGER_H

#include "common.h"
#include <string>

void init_logger(SharedState* state);
void log_msg(SharedState* state, const char* source, const char* format, ...);
std::string get_timestamp(SharedState* state);
const char* location_name(Location loc);

#endif
