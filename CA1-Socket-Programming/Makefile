CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2 -pthread

BUILD_DIR = build
COMMON_SRC = src/common/utils.cpp
COMMON_OBJ = $(BUILD_DIR)/common/utils.o

TARGETS = ping-server ping-client chat-server chat-client ftp-server ftp-client

all: $(TARGETS)

pinger: ping-server ping-client
chat: chat-server chat-client
ftp: ftp-server ftp-client

$(BUILD_DIR)/common/%.o: src/common/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/pinger/%.o: src/pinger/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/chat/%.o: src/chat/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ftp/%.o: src/ftp/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

ping-server: $(BUILD_DIR)/pinger/server_main.o $(BUILD_DIR)/pinger/ping_server.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

ping-client: $(BUILD_DIR)/pinger/client_main.o $(BUILD_DIR)/pinger/ping_client.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

chat-server: $(BUILD_DIR)/chat/server_main.o $(BUILD_DIR)/chat/chat_server.o $(BUILD_DIR)/chat/room_manager.o $(BUILD_DIR)/chat/protocol.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

chat-client: $(BUILD_DIR)/chat/client_main.o $(BUILD_DIR)/chat/chat_client.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

ftp-server: $(BUILD_DIR)/ftp/server_main.o $(BUILD_DIR)/ftp/ftp_server.o $(BUILD_DIR)/ftp/ftp_session.o $(BUILD_DIR)/ftp/mode_b.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

ftp-client: $(BUILD_DIR)/ftp/client_main.o $(BUILD_DIR)/ftp/ftp_client.o $(BUILD_DIR)/ftp/mode_b.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR) $(TARGETS) *.o *.tmp test_ftp_root client_downloads tmp

.PHONY: all pinger chat ftp clean
