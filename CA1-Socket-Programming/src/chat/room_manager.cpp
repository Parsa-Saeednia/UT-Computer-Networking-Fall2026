#include "room_manager.hpp"

#include <algorithm>

using namespace std;

bool RoomManager::room_exists(const string& room_name) const {
    return rooms_.find(room_name) != rooms_.end();
}

void RoomManager::ensure_room(const string& room_name) {
    if (rooms_.find(room_name) == rooms_.end()) {
        rooms_[room_name] = Room{room_name, 1, {}};
    }
}

bool RoomManager::is_member(int fd, const string& room_name) const {
    auto it = rooms_.find(room_name);
    if (it == rooms_.end()) return false;
    return it->second.member_fds.find(fd) != it->second.member_fds.end();
}

void RoomManager::add_member(int fd, const string& room_name) {
    ensure_room(room_name);
    rooms_[room_name].member_fds.insert(fd);
}

bool RoomManager::remove_member(int fd, const string& room_name) {
    auto it = rooms_.find(room_name);
    if (it == rooms_.end()) return false;
    return it->second.member_fds.erase(fd) > 0;
}

vector<string> RoomManager::remove_client_from_all(int fd) {
    vector<string> left_rooms;
    for (auto& pair : rooms_) {
        if (pair.second.member_fds.erase(fd) > 0) {
            left_rooms.push_back(pair.first);
        }
    }
    return left_rooms;
}

uint64_t RoomManager::next_message_id(const string& room_name) {
    ensure_room(room_name);
    return rooms_[room_name].next_msg_id++;
}

vector<int> RoomManager::get_members_except(const string& room_name, int except_fd) const {
    vector<int> result;
    auto it = rooms_.find(room_name);
    if (it != rooms_.end()) {
        for (int fd : it->second.member_fds) {
            if (fd != except_fd) result.push_back(fd);
        }
    }
    return result;
}

vector<int> RoomManager::get_members(const string& room_name) const {
    vector<int> result;
    auto it = rooms_.find(room_name);
    if (it != rooms_.end()) {
        for (int fd : it->second.member_fds) result.push_back(fd);
    }
    return result;
}

vector<pair<string, size_t>> RoomManager::get_all_rooms_sorted() const {
    vector<pair<string, size_t>> result;
    for (const auto& pair : rooms_) {
        result.emplace_back(pair.first, pair.second.member_fds.size());
    }
    sort(result.begin(), result.end(),
         [](const auto& a, const auto& b) { return a.first < b.first; });
    return result;
}
