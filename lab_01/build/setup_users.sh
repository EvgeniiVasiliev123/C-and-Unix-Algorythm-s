#!/bin/bash
# Скрипт для настройки пользователей и групп

# Создание групп
sudo groupadd group_max 2>/dev/null
sudo groupadd group_min 2>/dev/null

# Создание пользователей
sudo useradd -m -G group_max user_max_1 2>/dev/null
sudo useradd -m -G group_min user_min_1 2>/dev/null

# Установка паролей
echo "user_max_1:pass123" | sudo chpasswd
echo "user_min_1:pass123" | sudo chpasswd

# Создание директорий
sudo mkdir -p /usr/local/folder_max /usr/local/folder_min

# Установка прав
sudo chown -R :group_max /usr/local/folder_max
sudo chown -R :group_max /usr/local/folder_min
sudo chmod 770 /usr/local/folder_max
sudo chmod 770 /usr/local/folder_min

echo "Настройка завершена"
