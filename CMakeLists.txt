cmake_minimum_required(VERSION 3.20)
project(telegram_schedule_bot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(tgbot REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(telegram_schedule_bot main.cpp)

target_link_libraries(telegram_schedule_bot
    PRIVATE
        TgBot
        nlohmann_json::nlohmann_json
        OpenSSL::SSL
        OpenSSL::Crypto
)
