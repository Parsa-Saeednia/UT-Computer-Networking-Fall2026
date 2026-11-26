#include "chat_server.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    try {
        Utils::CommandLineParser parser(argc, argv);
        if (!parser.has("port")) {
            cerr << "Usage: " << argv[0] << " --port P\n";
            return 1;
        }

        int port = parser.get_int("port", Config::Chat::DEFAULT_PORT);
        ChatServer server(port);
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
