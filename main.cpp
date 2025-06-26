#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>

using json = nlohmann::json;

std::unordered_map<int64_t, std::string> userGroups;
std::unordered_map<int64_t, bool> waitingForGroup;
std::vector<std::string> availableGroups;

void loadUserGroups() {
    std::ifstream in("../data/users.json");
    if (in) {
        json j; in >> j;
        for (auto& el : j.items())
            userGroups[std::stoll(el.key())] = el.value();
    }
}

void saveUserGroups() {
    json j;
    for (auto& [id, group] : userGroups)
        j[std::to_string(id)] = group;
    std::ofstream("../data/users.json") << j.dump(4);
}

void loadAvailableGroups() {
    std::ifstream in("../data/groups.json");
    availableGroups.clear();
    if (in) {
        json j; in >> j;
        if (j.is_array())
            for (auto& group : j)
                availableGroups.push_back(group.get<std::string>());
    }
}

bool isGroupAvailable(const std::string& group) {
    return std::find(availableGroups.begin(), availableGroups.end(), group) != availableGroups.end();
}

std::string getScheduleForGroup(const std::string& group) {
    std::ifstream in("../data/schedule.json");
    if (!in.is_open()) return "Ошибка: файл расписания не найден.";
    json schedule; in >> schedule;
    if (!schedule.contains(group)) return "Расписание для группы не найдено.";
    std::string res = "Расписание для группы " + group + ":\n";
    for (auto& [day, val] : schedule[group].items())
        res += day + ": " + val.get<std::string>() + "\n";
    return res;
}

int main() {
    loadUserGroups();
    loadAvailableGroups();

    TgBot::Bot bot("7324604886:AAEYZcPhKpnVzHburBsPicgd_Fjsz-MRedI");

    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr msg) {
        if (msg->text.empty()) return;
        int64_t chatId = msg->chat->id;
        std::string text = msg->text;

        if (waitingForGroup.count(chatId)) {
            if (isGroupAvailable(text)) {
                userGroups[chatId] = text;
                saveUserGroups();
                bot.getApi().sendMessage(chatId, "Вы успешно зарегистрированы в группе: " + text);
                waitingForGroup.erase(chatId);
            } else {
                bot.getApi().sendMessage(chatId, "Такой группы нет. Введите корректное название.");
            }
            return;
        }

        if (text == "/start") {
            if (userGroups.count(chatId))
                bot.getApi().sendMessage(chatId, "Вы уже зарегистрированы в группе: " + userGroups[chatId]);
            else {
                bot.getApi().sendMessage(chatId, "Введите название вашей группы:");
                waitingForGroup[chatId] = true;
            }
        } else if (text == "/group") {
            bot.getApi().sendMessage(chatId, "Введите новую группу:");
            waitingForGroup[chatId] = true;
        } else if (text == "/schedule") {
            if (!userGroups.count(chatId)) {
                bot.getApi().sendMessage(chatId, "Вы не зарегистрированы. Введите /start.");
                return;
            }
            bot.getApi().sendMessage(chatId, getScheduleForGroup(userGroups[chatId]));
        } else if (text == "/groups") {
            if (availableGroups.empty())
                bot.getApi().sendMessage(chatId, "Список групп пока недоступен.");
            else {
                std::string list = "Доступные группы:\n";
                for (auto& g : availableGroups) list += g + "\n";
                bot.getApi().sendMessage(chatId, list);
            }
        } else {
            bot.getApi().sendMessage(chatId, "Неизвестная команда. Используйте /schedule, /group или /groups.");
        }
    });

    try {
        TgBot::TgLongPoll longPoll(bot);
        while (true) longPoll.start();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}
