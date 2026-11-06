#include "ftp_server.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    try {
        Utils::CommandLineParser parser(argc, argv);
        if (!parser.has("port") || !parser.has("root")) {
            cerr << "Usage: " << argv[0] << " --port P --root DIR\n";
            return 1;
        }

        int port = parser.get_int("port", Config::Ftp::DEFAULT_PORT);
        string root = parser.get_string("root", "./ftp_root");

        FtpServer server(port, root);
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
