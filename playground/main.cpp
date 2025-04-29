#include "rakro/server/debug_instrument.hpp"
#include "rakro/server/server.hpp"
#include <memory>
#include <print>

int main() {

    rakro::RakServer server{"19132", rakro::ServerConfig{}};
    server.update_server_id(
        "MCPE;Dedicated Server;782;1.21.71;0;10;13253860892328930865;Bedrock "
        "level;Survival;1;19132;19133;"
    );

    std::unique_ptr<rakro::IRakServerDebugInstrument> val =
        std::make_unique<rakro::debugger::RakServerLogDebugger>();

    server.update_debugger(std::move(val));

    server.start();

    while (true) {}
}