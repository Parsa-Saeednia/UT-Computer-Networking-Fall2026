#include "ping_server.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    try {
        Utils::CommandLineParser parser(argc, argv);
        if (!parser.has("port")) {
            cerr << "Usage: " << argv[0] << " --port P [--loss L] [--seed S]\n";
            return 1;
        }

        int port = parser.get_int("port", Config::Pinger::DEFAULT_PORT);
        double loss = parser.get_double("loss", Config::Pinger::DEFAULT_LOSS_RATE);
        bool has_seed = parser.has("seed");
        unsigned int seed = static_cast<unsigned int>(parser.get_int("seed", 0));

        PingServer server(port, loss, seed, has_seed);
        server.start();
    } catch (const NetworkException& e) {
        cerr << "Network Error: " << e.what() << "\n";
        return 1;
    } catch (const exception& e) {
        cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
