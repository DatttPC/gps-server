#include "header.h"

int startTcpServer() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    // std::vector<uint8_t> receiveBuffer(1024);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345); // Port number
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the address and port
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding socket." << std::endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 1) == -1) {
        std::cerr << "Error listening." << std::endl;
        return 1;
    }

    std::cout << "Server is listening for incoming connections..." << std::endl;

    // Accept a connection
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == -1) {
        std::cerr << "Error accepting connection." << std::endl;
        return 1;
    }

    std::cout << "Connected to a client." << std::endl;

    // Receive and send messages
    while (true) {   
        std::vector<uint8_t> receiveBuffer(1024);
        // memset(receiveBuffer.data(), 0, receiveBuffer.size());
        ssize_t bytesRead = recv(clientSocket, receiveBuffer.data(), receiveBuffer.size(), 0);
        if (bytesRead <= 0) {
            std::cerr << "Error receiving data" << std::endl;
            break;
        }
        std::cout << "Received " << bytesRead << " bytes" << std::endl;
        receiveBuffer.resize(static_cast<size_t>(bytesRead));

        // Process the received packet 
        uint16_t packetId = Packet::GetPacketId(receiveBuffer);
        Packet* receivedPacket = PacketFactory::GetPacket(packetId);
        receivedPacket->Deserialize(receiveBuffer);
        receivedPacket->PrintInformation();

        // Create a response packet
        Packet* responsePacket = PacketFactory::GetPacket(packetId + 1);
        responsePacket->FillResponseInformation(receivedPacket);
        std::vector<uint8_t> serializedResponsePacket = responsePacket->Serialize();

        // Send response packet back to the client
        ssize_t bytesSent = send(clientSocket, serializedResponsePacket.data(), serializedResponsePacket.size(), 0);
            if (bytesSent < 0) {
                perror("Sending failed");
                return 1;
            }
            else {
                std::cout << "Sent response packet successfully" << std::endl << std::endl;
            }

        // Free memory
        delete receivedPacket;
        receivedPacket = nullptr;
        delete responsePacket;
        responsePacket = nullptr;
    }

    // Close sockets
    close(clientSocket);
    close(serverSocket);

    return 0;}
