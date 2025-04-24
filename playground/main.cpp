#include "rakro/server/server.hpp"
#include <print>

int main() {

    rakro::RakServer server{"19132"};
    server.update_server_id(
        "MCPE;Dedicated Server;782;1.21.71;0;10;13253860892328930865;Bedrock "
        "level;Survival;1;19132;19133;"
    );

    server.start();

    while (true) {}
}