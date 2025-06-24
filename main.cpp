#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>
#include <iostream> 
#include <fstream>
#include <unordered_map>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>
#include "utils/schedule_updater.h"  // новый модуль обновления расписания и групп

using json = nlohmann::json;

std::unordered_map<int64_t, std::string> userGroups;
std::unordered_map<int64_t, bool> waitingForGroup;
std::vector<std::string> availableGroups;

void loadUserGroups() {
    std::ifstream in("../data/groups.json");
    if (in) {
        json j;
        in >> j;
        for (auto& el : j.items()) {
            userGroups[std::stoll(el.key())] = el.value();
        }
    }
}

void saveUserGroups() {
    json j;
    for (const auto& [id, group] : userGroups) {
        j[std::to_string(id)] = group;
    }
    std::ofstream out("../data/users.json");
    out << j.dump(4);
}

void loadAvailableGroups() {
    std::ifstream in("../data/groups.json");
    availableGroups.clear();
    if (in) {
        json j;
        in >> j;
        if (j.is_array()) {
            for (const auto& group : j) {
                availableGroups.push_back(group.get<std::string>());
            }
        }
    }
}

bool isGroupAvailable(const std::string& group) {
    return std::find(availableGroups.begin(), availableGroups.end(), group) != availableGroups.end();
}

std::string getScheduleForGroup(const std::string& group) {
    std::ifstream in("../data/schedule.json");
    if (!in.is_open()) {
        return "Ошибка: файл расписания не найден.";
    }

    json schedule;
    in >> schedule;
    if (!schedule.contains(group)) {
        return "Расписание для группы не найдено.";
    }

    std::string result = "Расписание для группы " + group + ":\n";
    for (const auto& day : schedule[group].items()) {
        result += day.key() + ": " + day.value().get<std::string>() + "\n";
    }
    return result;
}

int main() {
    std::cout << "Запуск бота()" << std::endl;

    // Загрузка пользователей и групп
    loadUserGroups();
    loadAvailableGroups();

    // Запускаем периодическое обновление расписания и групп
    startPeriodicUpdate();

    TgBot::Bot bot("7324604886:AAEYZcPhKpnVzHburBsPicgd_Fjsz-MRedI");

    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr message) {
        if (message->text.empty()) return;

        int64_t chatId = message->chat->id;
        std::string text = message->text;

        if (waitingForGroup.count(chatId)) {
            std::string groupName = text;
            if (isGroupAvailable(groupName)) {
                userGroups[chatId] = groupName;
                saveUserGroups();
                bot.getApi().sendMessage(chatId, "Вы успешно зарегистрированы в группе: " + groupName);
                waitingForGroup.erase(chatId);
            } else {
                bot.getApi().sendMessage(chatId, "Такой группы нет в списке доступных. Введите корректное название группы или используйте команду /groups для списка.");
            }
            return;
        }

        if (text == "/start") {
            if (userGroups.count(chatId)) {
                bot.getApi().sendMessage(chatId, "Вы уже зарегистрированы в группе: " + userGroups[chatId]);
            } else {
                bot.getApi().sendMessage(chatId, "Здравствуйте! Введите название вашей группы:");
                waitingForGroup[chatId] = true;
            }
        } else if (text == "/group") {
            bot.getApi().sendMessage(chatId, "Введите новую группу:");
            waitingForGroup[chatId] = true;
        } else if (text == "/schedule") {
            if (!userGroups.count(chatId)) {
                bot.getApi().sendMessage(chatId, "Вы не зарегистрированы. Введите /start для начала.");
                return;
            }
            std::string schedule = getScheduleForGroup(userGroups[chatId]);
            bot.getApi().sendMessage(chatId, schedule);
        } else if (text == "/groups") {
            if (availableGroups.empty()) {
                bot.getApi().sendMessage(chatId, "Список групп пока недоступен.");
            } else {
                std::string groupsList = "Доступные группы:\n";
                for (const auto& group : availableGroups) {
                    groupsList += group + "\n";
                }
                bot.getApi().sendMessage(chatId, groupsList);
            }
        } else {
            bot.getApi().sendMessage(chatId, "Неизвестная команда. Используйте /schedule, /group или /groups.");
        }
    });

    try {
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }

    return 0;
}
