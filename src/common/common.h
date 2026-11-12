#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <map>

#define PROTO_VERSION 1
#define MAX_DGRAM_SIZE 1200
#define DEFAULT_PORT 20071

#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

void log_init(const std::string& role, const std::string& id);
void log_print(const std::string& event);
std::string wsa_error_text(int code);
void log_wsa_error(const std::string& func, const std::string& ctx);

sockaddr_in make_addr(const std::string& host, int port);
std::string peer_text(const sockaddr_in& a);
bool send_dgram(SOCKET s, const sockaddr_in& to, const std::string& payload);
bool recv_dgram(SOCKET s, sockaddr_in& from, std::string& payload, bool* timed_out);

std::map<std::string, std::string> parse_fields(const std::string& line, std::string* cmd);
std::string field(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def = "");
std::string build_msg(const std::string& cmd, const std::vector<std::pair<std::string, std::string>>& fields);
bool parse_double(const std::string& s, double& out);

struct Options {
    std::string host = "127.0.0.1";
    int port = DEFAULT_PORT;
    std::string id = "1";
    int timeout_ms = 2000;
    int requests = 0;
    int seed = 7887;
    std::string mode = "interactive";
    std::string log_file;
};

Options parse_args(int argc, char** argv);
void qpc_init();
double qpc_now_ms();
