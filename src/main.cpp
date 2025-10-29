#include "App.h"
#include "Args.h"
#include <iostream>

int main(int argc, char** argv) {
    Config cfg;
    std::string err;
    if (!parseArgs(argc, argv, cfg, err)) {
        if (!err.empty()) std::cerr << "[Argumentos] " << err << "\n";
        return err.empty() ? 0 : 1;
    }

    App app;
    if (!app.init(cfg)) return 1;
    app.run();
    return 0;
}
