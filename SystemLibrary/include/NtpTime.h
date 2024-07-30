#pragma once
#ifndef NTPTIME
#define NTPTIME
namespace SystemLibrary {
	namespace NtpTime {
        typedef struct
        {
            uint8_t li_vn_mode;      // 八位的协议版本号、模式和Leap Indicator
            uint8_t stratum;         // 八位的时钟层级
            uint8_t poll;            // 八位的轮询间隔
            uint8_t precision;       // 八位的时钟精度
            uint32_t rootDelay;      // 32位的根延迟
            uint32_t rootDispersion; // 32位的根差值
            uint32_t refId;          // 32位的参考时间源标识符
            uint32_t refTm_s;        // 32位的参考时间戳
            uint32_t refTm_f;        // 32位的参考时间戳小数部分
            uint32_t origTm_s;       // 32位的发出请求的时间戳
            uint32_t origTm_f;       // 32位的发出请求的时间戳小数部分
            uint32_t rxTm_s;         // 32位的NTP服务器接收到请求的时间戳
            uint32_t rxTm_f;         // 32位的NTP服务器接收到请求的时间戳小数部分
            uint32_t txTm_s;         // 32位的NTP服务器发送回应的时间戳
            uint32_t txTm_f;         // 32位的NTP服务器发送回应的时间戳小数部分
        } ntp_packet;
        time_t GetNtpTime(const char* szNtpServerName) {
            constexpr auto NTP_TIMESTAMP_DELTA = 2208988800ull;
            WSADATA wsaData{};
            int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (err != 0) {
                std::string Error = std::string(xor_str("Error: cannot initialize Winsock library. ")) + std::to_string(WSAGetLastError());
                OutputDebugStringA(Error.c_str());
                return -1;
            }
            SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sockfd == INVALID_SOCKET) {
                std::string Error = std::string(xor_str("Error: cannot create socket. ")) + std::to_string(WSAGetLastError());
                OutputDebugStringA(Error.c_str());
                WSACleanup();
                return -1;
            }
            int timeout = 5000;
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
                std::string Error = std::string(xor_str("Error: cannot set socket option.")) + std::to_string(WSAGetLastError());
                OutputDebugStringA(Error.c_str());
                closesocket(sockfd);
                WSACleanup();
                return -1;
            }
            struct addrinfo hints = {};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
            struct addrinfo* result = nullptr;
            err = getaddrinfo(szNtpServerName, "ntp", &hints, &result);
            if (err != 0) {
                std::string Error = std::string(xor_str("Error: cannot resolve server address.")) + std::to_string(WSAGetLastError());
                OutputDebugStringA(Error.c_str());
                closesocket(sockfd);
                WSACleanup();
                return -1;
            }
            struct sockaddr_in servaddr = {};
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(123);
            servaddr.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
            ntp_packet packet = {};
            packet.li_vn_mode = 0x1b;
            packet.txTm_s = htonl((uint32_t)time(NULL) + NTP_TIMESTAMP_DELTA);
            err = sendto(sockfd, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
            if (err == SOCKET_ERROR) {
                std::string Error = std::string(xor_str("Error: cannot send packet.")) + std::to_string(WSAGetLastError());
                OutputDebugStringA(Error.c_str());
                closesocket(sockfd);
                WSACleanup();
                return -1;
            }
            ntp_packet recvpacket = {};
            int addrlen = sizeof(servaddr);
            err = recvfrom(sockfd, (char*)&recvpacket, sizeof(recvpacket), 0, (struct sockaddr*)&servaddr, &addrlen);
            if (err == SOCKET_ERROR) {
                std::string Error=std::string(xor_str("Error: cannot receive packet. ")) +std::to_string(WSAGetLastError());
                OutputDebugStringA(Error.c_str());
                closesocket(sockfd);
                WSACleanup();
                return -1;
            }
            uint32_t timestamp = ntohl(recvpacket.txTm_s) - NTP_TIMESTAMP_DELTA;
            time_t t = (time_t)timestamp;
            closesocket(sockfd);
            WSACleanup();
            return t;
        }

	}
}
#endif