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

//данные из файла загружает в память на старте
void loadUserGroups() {
    std::ifstream in("../data/users.json");
    if (in) {
        json j; in >> j;
        for (auto& el : j.items())
            userGroups[std::stoll(el.key())] = el.value();
    }
}
//изменение данных из памяти сохраняет в файл
void saveUserGroups() {
    json j;
    for (auto& [id, group] : userGroups)
        j[std::to_string(id)] = group;
    std::ofstream("../data/users.json") << j.dump(4); //отступ 4 пробела
}

//актуальный список групп из файла загружает в память
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

//проверка вводимой пользователем группы среди элементов вектора - существует/нет
bool isGroupAvailable(const std::string& group) {
    return std::find(availableGroups.begin(), availableGroups.end(), group) != availableGroups.end();
}

//открытие файла с расписанием + форматирование данных json 
std::string getScheduleForGroup(const std::string& group) {
    std::ifstream in("../data/schedule.json");
    if (!in.is_open()) return "Ошибка: файл расписания не найден.";
    
    nlohmann::ordered_json schedule;
    in >> schedule;
    
    if (!schedule.contains(group)) return "Расписание для группы не найдено.";
    
    std::string res = "Расписание для группы " + group + "\n\n";
    
    for (auto& [day, val] : schedule[group].items()) {
        res += day + ":\n";
        
        std::string subjects = val.get<std::string>();
        size_t start = 0;
        size_t end = subjects.find(',');
        
        while (end != std::string::npos) {
            std::string subject = subjects.substr(start, end - start);
            
            subject.erase(0, subject.find_first_not_of(' '));
            subject.erase(subject.find_last_not_of(' ') + 1);
            
            if (!subject.empty()) {
                res += "  • " + subject + "\n";
            }
            
            start = end + 1;
            end = subjects.find(',', start);
        }
        
        std::string lastSubject = subjects.substr(start);
        lastSubject.erase(0, lastSubject.find_first_not_of(' '));
        lastSubject.erase(lastSubject.find_last_not_of(' ') + 1);
        
        if (!lastSubject.empty()) {
            res += "  • " + lastSubject + "\n";
        }
        
        res += "\n";
    }
    return res;
}

//токен
std::ifstream file("token.txt");
std::string token;

std::string loadToken() {
    std::ifstream file("../token.txt");
    std::string token;

    if (!file || !std::getline(file, token) || token.empty()) {
        std::cerr << "Ошибка: не удалось загрузить токен из файла или файл пустой\n";
        return "";
    }
    return token;
}

//логика общения и обработка сообщений
int main() {
    loadUserGroups();
    loadAvailableGroups();
 
    std::string token = loadToken();
    if (token.empty()) {
        return 1;
    }
    TgBot::Bot bot(token);

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
            if (userGroups.count(chatId)) {
                bot.getApi().sendMessage(chatId, "Вы уже зарегистрированы в группе: " + userGroups[chatId] + 
                                          "\nИспользуйте команды:\n/schedule - Показать расписание\n"
                                         "/groups - Список групп\n"
                                          "/changegroup - Сменить группу");
            } else {
                bot.getApi().sendMessage(chatId, "Введите название вашей группы:");
                waitingForGroup[chatId] = true;
            }
        } else if (text == "/changegroup" || text == "/group") {
            bot.getApi().sendMessage(chatId, "Введите новую группу:");
            waitingForGroup[chatId] = true;
        } else if (text == "/schedule") {
            if (!userGroups.count(chatId)) {
                bot.getApi().sendMessage(chatId, "Вы не зарегистрированы. Введите /start.");
                return;
            }
            bot.getApi().sendMessage(chatId, getScheduleForGroup(userGroups[chatId]));
        } else if (text == "/groups") {
            if (availableGroups.empty()) {
                bot.getApi().sendMessage(chatId, "Список групп пока недоступен.");
            } else {
                std::string list = "Доступные группы:\n";
                for (auto& g : availableGroups) list += "• " + g + "\n";
                bot.getApi().sendMessage(chatId, list);
            }
        } else {
            bot.getApi().sendMessage(chatId, "Неизвестная команда. Используйте:\n"
                                     "/start - Начать работу\n"
                                     "/schedule - Показать расписание\n"
                                     "/groups - Список групп\n"
                                    "/changegroup - Сменить группу");
        }
    });
//цикл работы бота
    try {
        TgBot::TgLongPoll longPoll(bot);
        while (true) longPoll.start();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}