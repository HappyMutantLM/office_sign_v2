#pragma once
#include <Arduino.h>
#include <vector>

struct Message {
  String name;
  String text;
  String timestamp;
};

void storageInit();
void storageSaveMessage(const String& name, const String& text);
std::vector<Message> storageGetMessages();
void storageClearMessages(); // optional maintenance helper
