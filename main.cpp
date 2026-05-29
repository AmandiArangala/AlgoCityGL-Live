#include "Application.h"
#include <iostream>

int main() {
    try {
        Application app(1280, 720, "AlgoCityGL Live");
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}