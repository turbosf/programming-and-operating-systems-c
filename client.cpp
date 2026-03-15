#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

//XOR Encryption/Decryption
string xor_crypt(string data) {
    for (char &c : data) c ^= 'X';
    return data;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    //Handshake Send PSK
    string psk = "faridkey";
    send(sock, psk.c_str(), psk.length(), 0);
    
    char buffer[1024] = {0};
    read(sock, buffer, 1024);

    if (string(buffer) != "OK") {
        cout << "Authentication failed.\n";
        return 1;
    }
    cout << "Connected securely.\n";

    //Secure Communication Loop
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