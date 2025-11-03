#pragma once
#include <string>

struct Config {
    int   width  = 1280;
    int   height = 720;
    int   n      = 400;
    float radius = 120.f;
    float speed  = 1.f;
    bool  parallel = false;
    unsigned int seed = 0;
    int   threads = 0;
    bool  bench   = false;
    bool  novsync = false;
};

bool parseArgs(int argc, char** argv, Config& out, std::string& error);
void printHelp();
