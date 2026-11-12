#include "common.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <sstream>

static std::string g_role = "APP";
static std::string g_id = "-";
static std::mutex g_log_mutex;
static LARGE_INTEGER g_qpc_freq = {0};
static std::string g_log_file;

void log_init(const std::string& role, const std::string& id) {
    g_role = role;
    g_id = id;
    SetConsoleOutputCP(CP_UTF8);
}

void log_print(const std::string& event) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[512];
    snprintf(buf, sizeof(buf), "[%02d:%02d:%02d.%03d] %s id=%s %s",
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
             g_role.c_str(), g_id.c_str(), event.c_str());
    std::lock_guard<std::mutex> lk(g_log_mutex);
    printf("%s\n", buf);
    fflush(stdout);
    if (!g_log_file.empty()) {
        FILE* f = fopen(g_log_file.c_str(), "a");
        if (f) { fprintf(f, "%s\n", buf); fclose(f); }
    }
}

std::string wsa_error_text(int code) {
    switch (code) {
    case WSAEINTR: return "WSAEINTR: interrupted function call";
    case WSAEACCES: return "WSAEACCES: permission denied";
    case WSAEINVAL: return "WSAEINVAL: invalid argument";
    case WSAEMSGSIZE: return "WSAEMSGSIZE: message too long";
    case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK: operation would block";
    case WSAENOTSOCK: return "WSAENOTSOCK: not a socket";
    case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ: destination address required";
    case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT: protocol not supported";
    case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT: socket type not supported";
    case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP: operation not supported";
    case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT: address family not supported";
    case WSAEADDRINUSE: return "WSAEADDRINUSE: address already in use";
    case WSAENETDOWN: return "WSAENETDOWN: network is down";
    case WSAENETUNREACH: return "WSAENETUNREACH: network unreachable";
    case WSAECONNRESET: return "WSAECONNRESET: connection reset by peer";
    case WSAENOBUFS: return "WSAENOBUFS: no buffer space available";
    case WSAETIMEDOUT: return "WSAETIMEDOUT: connection timed out";
    case WSAECONNREFUSED: return "WSAECONNREFUSED: connection refused";
    case WSANOTINITIALISED: return "WSANOTINITIALISED: WSAStartup not called";
    default: {
        std::ostringstream os;
        os << "WSA error " << code;
        return os.str();
    }
    }
}

void log_wsa_error(const std::string& func, const std::string& ctx) {
    int code = WSAGetLastError();
    log_print("ERROR func=" + func + " ctx=" + ctx + " code=" + std::to_string(code) +
              " desc=\"" + wsa_error_text(code) + "\"");
}

sockaddr_in make_addr(const std::string& host, int port) {
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons((u_short)port);
    inet_pton(AF_INET, host.c_str(), &a.sin_addr);
    return a;
}

std::string peer_text(const sockaddr_in& a) {
    char ip[64];
    inet_ntop(AF_INET, &a.sin_addr, ip, sizeof(ip));
    return std::string(ip) + ":" + std::to_string(ntohs(a.sin_port));
}

bool send_dgram(SOCKET s, const sockaddr_in& to, const std::string& payload) {
    if (payload.size() > MAX_DGRAM_SIZE) return false;
    int n = sendto(s, payload.data(), (int)payload.size(), 0, (const sockaddr*)&to, sizeof(to));
    return n == (int)payload.size();
}

bool recv_dgram(SOCKET s, sockaddr_in& from, std::string& payload, bool* timed_out) {
    if (timed_out) *timed_out = false;
    char buf[65536];
    int len = sizeof(from);
    int n = recvfrom(s, buf, sizeof(buf), 0, (sockaddr*)&from, &len);
    if (n == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAETIMEDOUT) { if (timed_out) *timed_out = true; return false; }
        return false;
    }
    payload.assign(buf, (size_t)n);
    return true;
}

std::map<std::string, std::string> parse_fields(const std::string& line, std::string* cmd) {
    std::map<std::string, std::string> m;
    std::istringstream is(line);
    std::string tok;
    if (!(is >> tok)) { if (cmd) cmd->clear(); return m; }
    if (cmd) *cmd = tok;
    while (is >> tok) {
        size_t eq = tok.find('=');
        if (eq == std::string::npos) continue;
        m[tok.substr(0, eq)] = tok.substr(eq + 1);
    }
    return m;
}

std::string field(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def) {
    auto it = m.find(key);
    return it == m.end() ? def : it->second;
}

std::string build_msg(const std::string& cmd, const std::vector<std::pair<std::string, std::string>>& fields) {
    std::string s = cmd;
    for (auto& kv : fields) { s += " "; s += kv.first; s += "="; s += kv.second; }
    return s;
}

bool parse_double(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = strtod(s.c_str(), &end);
    return end && *end == '\0';
}

Options parse_args(int argc, char** argv) {
    Options o;
    for (int i = 1; i + 1 <= argc - 1; i += 2) {
        std::string k = argv[i];
        std::string v = argv[i + 1];
        if (k == "--host") o.host = v;
        else if (k == "--port") o.port = atoi(v.c_str());
        else if (k == "--id") o.id = v;
        else if (k == "--timeout") o.timeout_ms = atoi(v.c_str());
        else if (k == "--requests") o.requests = atoi(v.c_str());
        else if (k == "--seed") o.seed = atoi(v.c_str());
        else if (k == "--mode") o.mode = v;
        else if (k == "--log") g_log_file = v;
    }
    return o;
}

void qpc_init() {
    QueryPerformanceFrequency(&g_qpc_freq);
}

double qpc_now_ms() {
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)g_qpc_freq.QuadPart;
}
