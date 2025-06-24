#include "schedule_updater.h"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <regex>
#include <nlohmann/json.hpp>
#include <regex>

// Буфер для записи ответа от curl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

std::string httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // Можно добавить таймауты, заголовки и т.д.

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << "\n";
            response.clear();
        }
        curl_easy_cleanup(curl);
    }
    return response;
}

// Пример парсинга списка групп из страницы
bool updateGroups() {
    std::cout << "[Обновление] Загружаю список групп...\n";
    std::string url = "https://ies.unitech-mo.ru/schedule_list_groups";
    std::string page = httpGet(url);
    if (page.empty()) {
        std::cerr << "[Ошибка] Не удалось получить список групп.\n";
        return false;
    }

    // Очень упрощённый пример парсинга — ищем группы в <option value="GROUP_NAME"> или аналогично
    std::regex groupRegex("<option[^>]*value=\"([^\"]+)\"");
    std::smatch match;
    std::string::const_iterator searchStart(page.cbegin());

    // JSON структура для групп
    nlohmann::json groupsJson = nlohmann::json::array();

    while (std::regex_search(searchStart, page.cend(), match, groupRegex)) {
        std::string groupName = match[1];
        groupsJson.push_back(groupName);
        searchStart = match.suffix().first;
    }

    // Сохраняем в файл data/groups.json
    std::ofstream out("../data/groups.json");
    if (!out.is_open()) {
        std::cerr << "[Ошибка] Не могу записать groups.json\n";
        return false;
    }
    out << groupsJson.dump(4);
    std::cout << "[Обновление] Список групп обновлен (" << groupsJson.size() << " групп).\n";
    return true;
}

// Загружаем и парсим расписание для всех групп
bool updateSchedule() {
    std::cout << "[Обновление] Обновляю расписание...\n";

    // Сначала загружаем список групп из файла
    std::ifstream in("../data/groups.json");
    if (!in.is_open()) {
        std::cerr << "[Ошибка] Не найден файл groups.json, сначала обновите группы.\n";
        return false;
    }
    nlohmann::json groupsJson;
    in >> groupsJson;
    in.close();

    nlohmann::json scheduleJson;

    // Для каждого group загружаем расписание
    for (auto& group : groupsJson) {
        std::string groupName = group.get<std::string>();
        std::string url = "https://ies.unitech-mo.ru/schedule/" + groupName;  // пример URL, нужно уточнить

        std::string page = httpGet(url);
        if (page.empty()) {
            std::cerr << "[Ошибка] Не удалось загрузить расписание для группы " << groupName << "\n";
            continue;
        }

        // Здесь нужно написать парсер расписания из HTML или JSON, зависит от сайта
        // Для примера — просто сохраним всю страницу как строку
        scheduleJson[groupName] = page;
    }

    // Записываем расписание в data/schedule.json
    std::ofstream out("../data/schedule.json");
    if (!out.is_open()) {
        std::cerr << "[Ошибка] Не могу записать schedule.json\n";
        return false;
    }
    out << scheduleJson.dump(4);

    std::cout << "[Обновление] Расписание обновлено.\n";
    return true;
}

// Запускаем цикл обновления в отдельном потоке
void startPeriodicUpdate() {
    std::thread([]() {
        int scheduleUpdateIntervalMinutes = 30;
        int groupUpdateIntervalMinutes = 60*24*30; // ~месяц

        int counter = 0;
        while (true) {
            if (counter % (groupUpdateIntervalMinutes / scheduleUpdateIntervalMinutes) == 0) {
                updateGroups();
            }
            updateSchedule();

            std::this_thread::sleep_for(std::chrono::minutes(scheduleUpdateIntervalMinutes));
            counter++;
        }
    }).detach();
}
