/**
 * FileManager.cpp — FileManager 的实现
 */

#include "model/FileManager.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

FileManager::FileManager(const std::string& dataRoot)
    : _dataRoot(dataRoot)
{
    // 确保数据根目录存在
    if (!_dataRoot.empty()) {
        std::filesystem::create_directories(_dataRoot);
    }
}

std::string FileManager::readAll(const std::string& relativePath) const
{
    std::filesystem::path fullPath = std::filesystem::path(_dataRoot) / relativePath;

    std::ifstream file(fullPath, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "[FileManager] 警告：无法打开文件 " << fullPath << " 进行读取" << std::endl;
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    return content;
}

bool FileManager::writeAll(const std::string& relativePath,
                           const std::string& content) const
{
    std::filesystem::path fullPath = std::filesystem::path(_dataRoot) / relativePath;

    // 确保父目录存在
    auto parent = fullPath.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream file(fullPath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[FileManager] 错误：无法打开文件 " << fullPath << " 进行写入" << std::endl;
        return false;
    }

    file << content;
    file.close();
    return true;
}

bool FileManager::appendLine(const std::string& relativePath,
                             const std::string& line) const
{
    std::filesystem::path fullPath = std::filesystem::path(_dataRoot) / relativePath;

    auto parent = fullPath.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream file(fullPath, std::ios::out | std::ios::app);
    if (!file.is_open()) {
        std::cerr << "[FileManager] 错误：无法打开文件 " << fullPath << " 进行追加" << std::endl;
        return false;
    }

    file << line << '\n';
    file.close();
    return true;
}

bool FileManager::exists(const std::string& relativePath) const
{
    std::filesystem::path fullPath = std::filesystem::path(_dataRoot) / relativePath;
    return std::filesystem::exists(fullPath);
}

bool FileManager::createDir(const std::string& relativePath) const
{
    std::filesystem::path fullPath = std::filesystem::path(_dataRoot) / relativePath;
    std::error_code ec;
    std::filesystem::create_directories(fullPath, ec);
    return !ec;
}

const std::string& FileManager::dataRoot() const
{
    return _dataRoot;
}
