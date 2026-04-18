#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fstream>

using namespace std;

string xor_crypt(string data) {
    for (char &c : data) c ^= 'X';
    return data;
}

string run_cmd(string cmd) {
    char buffer[128];
    string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "Error: System execution failed.";
    
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) result += buffer;
    pclose(pipe);
    
    return result.empty() ? "Command executed" : result;
}

void* handle_client(void* arg) {
    int sock = *(int*)arg;
    delete (int*)arg;
    char buffer[2048];

    // 1. Authentication
    memset(buffer, 0, 2048);
    int len = read(sock, buffer, 2048);
    if (len <= 0) { close(sock); return NULL; }
    string key(buffer, len);
    
    string user_role = "";
    ifstream file("users.txt");
    string line;
    while(getline(file, line)) {
        if (line.find(key) == 0) {
            user_role = line.substr(line.find(' ') + 1);
            if (!user_role.empty() && user_role.back() == '\r') user_role.pop_back(); 
            break;
        }
    }

    if (user_role == "") {
        close(sock);
        return NULL;
    }
    
    send(sock, "OK", 2, 0);
    cout << "User: " << key << " Logged in as: " << user_role << endl;

    // 2. Command Logic
    while (true) {
        memset(buffer, 0, 2048); 
        int bytes = read(sock, buffer, 2048);
        if (bytes <= 0) break;

        string msg = xor_crypt(string(buffer, bytes));
        size_t first_space = msg.find(' ');
        string cmd = (first_space == string::npos) ? msg : msg.substr(0, first_space);
        string args = (first_space == string::npos) ? "" : msg.substr(first_space + 1);
        
        string res = "Access Denied.";
        int rank = (user_role == "Top") ? 3 : (user_role == "Medium") ? 2 : 1;

        // Rank 2 access
        if (cmd == "ls") res = run_cmd("ls -F");
        else if (cmd == "cat") res = run_cmd("cat " + args);
        else if (rank >= 2 && cmd == "read") res = run_cmd("cat " + args);
        else if (rank >= 2 && cmd == "copy") res = run_cmd("cp " + args);
        else if (rank >= 2 && cmd == "edit") {
            size_t arg_sep = args.find(' ');
            if (arg_sep != string::npos) {
                res = run_cmd("echo \"" + args.substr(arg_sep + 1) + "\" > " + args.substr(0, arg_sep));
            } else res = "Usage: edit [file] [content]";
        }
        else if (rank == 3 && cmd == "delete") res = run_cmd("rm " + args);
        
        // Rank 3 access 
        else if (rank == 3) {
            cout << "Top Level override: running " << msg << endl;
            res = run_cmd(msg);
        }

        string reply = xor_crypt(res);
        send(sock, reply.c_str(), reply.length(), 0);
    }

    close(sock);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{AF_INET, htons(8080), INADDR_ANY};
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);
    cout << "Listening on port 8080..." << endl;
    while (true) {
        int* client_sock = new int(accept(server_fd, NULL, NULL));
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_sock);
        pthread_detach(tid);
    }
    return 0;
}