#ifndef SRC_CHAT_ROOM_MANAGER_HPP
#define SRC_CHAT_ROOM_MANAGER_HPP

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <cstdint>

class RoomManager {
  public:
    RoomManager() = default;

    bool room_exists(const std::string& room_name) const;
    void ensure_room(const std::string& room_name);
    bool is_member(int fd, const std::string& room_name) const;

    void add_member(int fd, const std::string& room_name);
    bool remove_member(int fd, const std::string& room_name);
    std::vector<std::string> remove_client_from_all(int fd);

    uint64_t next_message_id(const std::string& room_name);
    std::vector<int> get_members_except(const std::string& room_name, int except_fd) const;
    std::vector<int> get_members(const std::string& room_name) const;

    std::vector<std::pair<std::string, size_t>> get_all_rooms_sorted() const;

  private:
    struct Room {
        std::string name;
        uint64_t next_msg_id = 1;
        std::set<int> member_fds;
    };

    std::unordered_map<std::string, Room> rooms_;
};

#endif // SRC_CHAT_ROOM_MANAGER_HPP
