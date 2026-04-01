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

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    char buffer[1024] = {0};
    read(client_socket, buffer, 1024);

    bool auth = false;
    ifstream file("users.txt");
    string line;
    while(getline(file, line)) {
        if(line == string(buffer)) auth = true;
    }

    if (!auth) {
        cout << "Authentication failed.\n";
        close(client_socket);
        return NULL;
    }
    
    send(client_socket, "OK", 2, 0);
    cout << "Client authenticated.\n";

    while (true) {
        memset(buffer, 0, 1024);
        if (read(client_socket, buffer, 1024) <= 0) break;
        
        cout << "Encrypted received: " << buffer << "\n";
        cout << "Decrypted message: " << xor_crypt(buffer) << "\n";
        
        string reply = xor_crypt("Message received");
        send(client_socket, reply.c_str(), reply.length(), 0);
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);
    
    cout << "Listening on port 8080...\n";

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        pthread_t thread_id;
        int* new_sock = new int(client_socket);
        pthread_create(&thread_id, NULL, handle_client, (void*)new_sock);
    }

    close(server_fd);
    return 0;
}