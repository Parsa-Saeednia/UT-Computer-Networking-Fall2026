#include "protocol.hpp"
#include "../common/utils.hpp"

using namespace std;

namespace ChatProtocol {

static CommandType resolve_type(const string& word) {
    if (word == "NICK") return CommandType::NICK;
    if (word == "JOIN") return CommandType::JOIN;
    if (word == "PART") return CommandType::PART;
    if (word == "MSG") return CommandType::MSG;
    if (word == "PRIVMSG") return CommandType::PRIVMSG;
    if (word == "LIST") return CommandType::LIST;
    if (word == "WHO") return CommandType::WHO;
    if (word == "PING") return CommandType::PING;
    if (word == "QUIT") return CommandType::QUIT;
    return CommandType::UNKNOWN;
}

Command parse_command(const string& line) {
    Command cmd;
    if (line.empty()) return cmd;

    auto [first_word, rest] = Utils::split_first(line, ' ');
    cmd.raw_cmd = first_word;
    cmd.type = resolve_type(first_word);

    if (cmd.type == CommandType::MSG || cmd.type == CommandType::PRIVMSG) {
        auto [target, text] = Utils::split_first(rest, ' ');
        cmd.arg1 = target;
        cmd.arg2 = text;
    } else {
        cmd.arg1 = rest;
    }

    return cmd;
}

string make_error_response(ErrorCode code, const string& token) {
    return "ERR " + to_string(static_cast<int>(code)) + " " + token + "\n";
}

} // namespace ChatProtocol
