#include "..\common\common.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>

static SOCKET g_sock = INVALID_SOCKET;
static sockaddr_in g_server{};
static int g_next_req = 0;

static bool open_socket(const Options& o) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { log_wsa_error("WSAStartup", "init"); return false; }
    g_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_sock == INVALID_SOCKET) { log_wsa_error("socket", "create"); return false; }
    DWORD tv = (DWORD)o.timeout_ms;
    setsockopt(g_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    DWORD br = 0;
    BOOL b = FALSE;
    WSAIoctl(g_sock, SIO_UDP_CONNRESET, &b, sizeof(b), nullptr, 0, &br, nullptr, nullptr);
    g_server = make_addr(o.host, o.port);
    double t0 = qpc_now_ms();
    sockaddr_in any = make_addr("0.0.0.0", 0);
    if (bind(g_sock, (sockaddr*)&any, sizeof(any)) == SOCKET_ERROR) { log_wsa_error("bind", "ephemeral"); return false; }
    char pb[32];
    snprintf(pb, sizeof(pb), "%.3f", qpc_now_ms() - t0);
    log_print("READY host=" + o.host + " port=" + std::to_string(o.port) + " bindMs=" + pb);
    return true;
}

static bool exchange(const std::string& body, std::string& resp, bool* timed_out, double* rtt) {
    std::string msg = body;
    if (msg.find("requestId=") == std::string::npos)
        msg += " requestId=" + std::to_string(++g_next_req);
    log_print("SEND peer=" + peer_text(g_server) + " size=" + std::to_string(msg.size()) + " msg=" + msg);
    double t0 = qpc_now_ms();
    if (!send_dgram(g_sock, g_server, msg)) { log_wsa_error("sendto", "request"); return false; }
    sockaddr_in from{};
    bool ok = recv_dgram(g_sock, from, resp, timed_out);
    if (rtt) *rtt = qpc_now_ms() - t0;
    if (ok) log_print("RECV peer=" + peer_text(from) + " size=" + std::to_string(resp.size()) + " msg=" + resp);
    return ok;
}

static int run_interactive(const Options& o) {
    std::string line;
    std::cout << "commands: RECTANGLE a=<x> b=<y> | CIRCLE r=<x> | TRIANGLE a=<x> b=<y> c=<z> | quit" << std::endl;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;
        std::string resp;
        bool timed = false;
        double rtt = 0;
        if (!exchange(line, resp, &timed, &rtt)) {
            if (timed) std::cout << "TIMEOUT: no response from server" << std::endl;
            else std::cout << "SEND/RECV error" << std::endl;
            continue;
        }
        char rb[32];
        snprintf(rb, sizeof(rb), "%.3f", rtt);
        std::cout << resp << "  rttMs=" << rb << std::endl;
    }
    return 0;
}

static int run_batch(const Options& o) {
    std::mt19937 rng((unsigned)(o.seed + atoi(o.id.c_str()) * 7919));
    double t_start = qpc_now_ms();
    int ok = 0, timeouts = 0, errors = 0;
    long long bytes_sent = 0, bytes_recv = 0;
    double rtt_sum = 0;

    for (int i = 0; i < o.requests; ++i) {
        int kind = i % 3;
        std::string body;
        if (kind == 0) {
            double a = 1 + (rng() % 10000) / 100.0;
            double b = 1 + (rng() % 10000) / 100.0;
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "RECTANGLE a=%.2f b=%.2f", a, b);
            body = tmp;
        } else if (kind == 1) {
            double r = 1 + (rng() % 5000) / 100.0;
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "CIRCLE r=%.2f", r);
            body = tmp;
        } else {
            double a = 10 + (rng() % 900) / 10.0;
            double b = 10 + (rng() % 900) / 10.0;
            double lo = fabs(a - b), hi = a + b;
            double c = lo + 0.05 * (hi - lo) + (rng() % 1000) / 1000.0 * 0.9 * (hi - lo);
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "TRIANGLE a=%.2f b=%.2f c=%.2f", a, b, c);
            body = tmp;
        }
        body += " requestId=" + std::to_string(++g_next_req);
        log_print("SEND peer=" + peer_text(g_server) + " size=" + std::to_string(body.size()) + " msg=" + body);
        bytes_sent += (long long)body.size();
        double t0 = qpc_now_ms();
        if (!send_dgram(g_sock, g_server, body)) { log_wsa_error("sendto", "request"); ++errors; continue; }
        sockaddr_in from{};
        std::string resp;
        bool timed = false;
        bool got = recv_dgram(g_sock, from, resp, &timed);
        double rtt = qpc_now_ms() - t0;
        if (!got) {
            if (timed) { ++timeouts; log_print("TIMEOUT requestId=" + std::to_string(g_next_req) + " waitMs=" + std::to_string(o.timeout_ms)); }
            else { ++errors; log_wsa_error("recvfrom", "response"); }
            continue;
        }
        bytes_recv += (long long)resp.size();
        rtt_sum += rtt;
        log_print("RECV peer=" + peer_text(from) + " size=" + std::to_string(resp.size()) + " msg=" + resp);
        std::string cmd;
        parse_fields(resp, &cmd);
        if (cmd == "OK") ++ok; else ++errors;
    }

    double total = qpc_now_ms() - t_start;
    char buf[512];
    snprintf(buf, sizeof(buf),
             "STATS id=%s requests=%d ok=%d timeouts=%d errors=%d bytesSent=%lld bytesRecv=%lld "
             "totalMs=%.3f avgRttMs=%.4f throughput=%.3f",
             o.id.c_str(), o.requests, ok, timeouts, errors, bytes_sent, bytes_recv,
             total, (ok + errors) ? rtt_sum / (ok + errors) : 0.0,
             total > 0 ? ok * 1000.0 / total : 0.0);
    log_print(buf);
    printf("%s\n", buf);
    return timeouts > 0 || errors > 0 ? 3 : 0;
}

int main(int argc, char** argv) {
    Options o = parse_args(argc, argv);
    log_init("CLIENT", o.id);
    qpc_init();
    if (!open_socket(o)) {
        if (g_sock != INVALID_SOCKET) closesocket(g_sock);
        WSACleanup();
        log_print("STOP result=socket_failed");
        return 1;
    }
    int rc = (o.mode == "batch" && o.requests > 0) ? run_batch(o) : run_interactive(o);
    closesocket(g_sock);
    WSACleanup();
    log_print("STOP result=" + std::to_string(rc));
    return rc;
}
