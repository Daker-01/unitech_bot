cmake_minimum_required(VERSION 3.16)
project(telegram_schedule_bot)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system)
find_package(TgBot REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

add_executable(telegram_schedule_bot
    main.cpp
    utils/schedule_updater.cpp   # Указываем путь к новому файлу
)

target_include_directories(telegram_schedule_bot PRIVATE
    utils                      # чтобы видеть schedule_updater.h
)

target_link_libraries(telegram_schedule_bot
    TgBot::TgBot
    Boost::system
    nlohmann_json::nlohmann_json
    CURL::libcurl
)
find_package(CURL REQUIRED)

target_link_libraries(telegram_schedule_bot
    CURL::libcurl
)
