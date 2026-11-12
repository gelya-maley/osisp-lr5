#include "..\common\common.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>

static std::atomic<bool> g_running(true);

static BOOL WINAPI ctrl_handler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT || type == CTRL_CLOSE_EVENT) {
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

static std::string fmt(double v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.6f", v);
    return buf;
}

static void reply(SOCKET s, const sockaddr_in& to, const std::string& msg) {
    log_print("SEND peer=" + peer_text(to) + " size=" + std::to_string(msg.size()) + " msg=" + msg);
    if (!send_dgram(s, to, msg)) log_wsa_error("sendto", "reply");
}

static void reply_error(SOCKET s, const sockaddr_in& to, const std::string& req, const std::string& code) {
    reply(s, to, build_msg("ERROR", {{"requestId", req}, {"code", code}}));
}

static void handle_request(SOCKET s, const sockaddr_in& from, const std::string& msg) {
    std::string cmd;
    auto f = parse_fields(msg, &cmd);
    std::string req = field(f, "requestId", "0");

    if (cmd == "RECTANGLE") {
        if (f.find("a") == f.end() || f.find("b") == f.end()) return reply_error(s, from, req, "MISSING_FIELD");
        double a, b;
        if (!parse_double(field(f, "a"), a) || !parse_double(field(f, "b"), b)) return reply_error(s, from, req, "BAD_NUMBER");
        if (a <= 0 || b <= 0) return reply_error(s, from, req, "BAD_DIMENSIONS");
        reply(s, from, build_msg("OK", {{"requestId", req}, {"kind", "RECTANGLE"},
                                        {"area", fmt(a * b)}, {"perimeter", fmt(2 * (a + b))}}));
    } else if (cmd == "CIRCLE") {
        if (f.find("r") == f.end()) return reply_error(s, from, req, "MISSING_FIELD");
        double r;
        if (!parse_double(field(f, "r"), r)) return reply_error(s, from, req, "BAD_NUMBER");
        if (r <= 0) return reply_error(s, from, req, "BAD_DIMENSIONS");
        const double pi = 3.14159265358979323846;
        reply(s, from, build_msg("OK", {{"requestId", req}, {"kind", "CIRCLE"},
                                        {"area", fmt(pi * r * r)}, {"circumference", fmt(2 * pi * r)}}));
    } else if (cmd == "TRIANGLE") {
        if (f.find("a") == f.end() || f.find("b") == f.end() || f.find("c") == f.end()) return reply_error(s, from, req, "MISSING_FIELD");
        double a, b, c;
        if (!parse_double(field(f, "a"), a) || !parse_double(field(f, "b"), b) || !parse_double(field(f, "c"), c))
            return reply_error(s, from, req, "BAD_NUMBER");
        if (a <= 0 || b <= 0 || c <= 0) return reply_error(s, from, req, "BAD_DIMENSIONS");
        if (a + b <= c || a + c <= b || b + c <= a) return reply_error(s, from, req, "IMPOSSIBLE_TRIANGLE");
        double p = (a + b + c) / 2;
        reply(s, from, build_msg("OK", {{"requestId", req}, {"kind", "TRIANGLE"},
                                        {"area", fmt(sqrt(p * (p - a) * (p - b) * (p - c)))}}));
    } else {
        reply_error(s, from, req, "UNKNOWN_COMMAND");
    }
}

int main(int argc, char** argv) {
    Options o = parse_args(argc, argv);
    log_init("SERVER", o.id);
    qpc_init();
    SetConsoleCtrlHandler(ctrl_handler, TRUE);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        log_wsa_error("WSAStartup", "init");
        return 1;
    }
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) { log_wsa_error("socket", "create"); WSACleanup(); return 1; }
    BOOL reuse = TRUE;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
    sockaddr_in addr = make_addr(o.host, o.port);
    if (o.host != "127.0.0.1") log_print("WARNING desc=server bound to non-loopback address");
    if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        log_wsa_error("bind", "port may be busy");
        closesocket(s); WSACleanup(); return 1;
    }
    log_print("START host=" + o.host + " port=" + std::to_string(o.port) +
              " proto=UDP version=" + std::to_string(PROTO_VERSION) + " maxDgram=" + std::to_string(MAX_DGRAM_SIZE));

    while (g_running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(s, &fds);
        timeval tv{0, 300000};
        int r = select(0, &fds, nullptr, nullptr, &tv);
        if (r == SOCKET_ERROR) { log_wsa_error("select", "main loop"); break; }
        if (r == 0) continue;
        sockaddr_in from{};
        std::string msg;
        bool timed = false;
        if (!recv_dgram(s, from, msg, &timed)) { log_wsa_error("recvfrom", "main loop"); continue; }
        log_print("RECV peer=" + peer_text(from) + " size=" + std::to_string(msg.size()) + " msg=" + msg);
        if (msg.size() > MAX_DGRAM_SIZE) {
            std::string cmd;
            auto f = parse_fields(msg, &cmd);
            reply_error(s, from, field(f, "requestId", "0"), "MSG_TOO_LARGE");
            continue;
        }
        handle_request(s, from, msg);
    }

    closesocket(s);
    WSACleanup();
    log_print("STOP");
    return 0;
}
