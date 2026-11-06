#ifndef SRC_CHAT_PROTOCOL_HPP
#define SRC_CHAT_PROTOCOL_HPP

#include <string>

namespace ChatProtocol {

enum class CommandType {
    NICK,
    JOIN,
    PART,
    MSG,
    PRIVMSG,
    LIST,
    WHO,
    PING,
    QUIT,
    UNKNOWN
};

enum class ErrorCode {
    UNKNOWN_COMMAND = 400,
    BAD_SYNTAX = 400,
    NOT_REGISTERED = 401,
    NOT_IN_ROOM = 403,
    NO_SUCH_NICK = 404,
    NO_SUCH_ROOM = 404,
    NICK_IN_USE = 409,
    ALREADY_REGISTERED = 409,
    ALREADY_IN_ROOM = 409,
    LINE_TOO_LONG = 413
};

struct Command {
    CommandType type = CommandType::UNKNOWN;
    std::string raw_cmd;
    std::string arg1;
    std::string arg2;
};

Command parse_command(const std::string& line);

std::string make_error_response(ErrorCode code, const std::string& token);

} // namespace ChatProtocol

#endif // SRC_CHAT_PROTOCOL_HPP
