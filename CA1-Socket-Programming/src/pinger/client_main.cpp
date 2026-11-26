#include "ping_client.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>

using namespace std;

static void run_client(const Utils::CommandLineParser& parser) {
    string host = parser.get_string("host", "127.0.0.1");
    int port = parser.get_int("port", Config::Pinger::DEFAULT_PORT);
    int count = parser.get_int("count", Config::Pinger::DEFAULT_COUNT);
    int timeout_ms = parser.get_int("timeout", Config::Pinger::DEFAULT_TIMEOUT_MS);
    int interval_ms = parser.get_int("interval", Config::Pinger::DEFAULT_INTERVAL_MS);

    PingClient client(host, port, count, timeout_ms, interval_ms);
    client.run();
}

int main(int argc, char* argv[]) {
    try {
        Utils::CommandLineParser parser(argc, argv);
        if (!parser.has("port")) {
            cerr << "Usage: " << argv[0] << " [--host H] --port P [--count N] [--timeout MS] [--interval MS]\n";
            return 1;
        }
        run_client(parser);
    } catch (const NetworkException& e) {
        cerr << "Network Error: " << e.what() << "\n";
        return 1;
    } catch (const exception& e) {
        cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
