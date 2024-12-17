#ifndef LIBRARY_H
#define LIBRARY_H

#include <winsock2.h>
#include <string>
#include <map>
#include <mutex>
#include <iostream>

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

DLL_EXPORT void broadcast_message(const std::string& message, SOCKET sender, std::map<SOCKET, std::string>& clients, std::mutex& clients_mutex);
DLL_EXPORT void send_to_client(const std::string& message, const std::string& target_name, std::map<SOCKET, std::string>& clients, std::mutex& clients_mutex);

#endif
