#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;

string xor_crypt(string data) {
    for (char &c : data) c ^= 'X';
    return data;
}

void* receive_messages(void* arg) {
    int sock = *(int*)arg;
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, 1024);
        if (read(sock, buffer, 1024) <= 0) break;
        cout << "\nServer reply: " << xor_crypt(buffer) << "\n> ";
        fflush(stdout);
    }
    return NULL;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    string psk;
    cout << "Enter key: ";
    getline(cin, psk);
    send(sock, psk.c_str(), psk.length(), 0);
    
    char buffer[1024] = {0};
    read(sock, buffer, 1024);

    if (string(buffer) != "OK") {
        cout << "Authentication failed.\n";
        return 1;
    }
    cout << "Connected securely.\n";

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, (void*)&sock);

    string input;
    while (true) {
        cout << "> ";
        getline(cin, input);
        if (input == "quit") break;

        string encrypted = xor_crypt(input);
        send(sock, encrypted.c_str(), encrypted.length(), 0);
    }

    close(sock);
    return 0;
}