/**
 * FileManager.hpp — 文件管理类 (FileManager)
 *
 * 封装所有底层文件 I/O 操作，提供统一的错误处理接口。
 * 所有路径参数均为相对于 _dataRoot 的相对路径。
 *
 * 依赖：
 *   - C++ 标准库 <fstream>、<string>
 *
 * 所属架构层：Model（数据持久化子层）
 * 所属模块：数据持久化与存储模块
 */

#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <string>

class FileManager {
public:
    /** 构造函数，指定数据根目录（如 "data/"） */
    explicit FileManager(const std::string& dataRoot);

    /**
     * 读取文本文件全部内容并以 std::string 返回。
     * 文件不存在时返回空字符串并打印警告。
     */
    std::string readAll(const std::string& relativePath) const;

    /**
     * 将内容写入文本文件（覆盖模式）。
     * 返回 true 表示写入成功，false 表示失败。
     */
    bool writeAll(const std::string& relativePath, const std::string& content) const;

    /**
     * 向文件末尾追加一行。
     * 若文件不存在则自动创建。
     * 返回 true 表示追加成功。
     */
    bool appendLine(const std::string& relativePath, const std::string& line) const;

    /** 检查文件是否存在 */
    bool exists(const std::string& relativePath) const;

    /** 在数据根目录下创建子目录（递归创建中间目录） */
    bool createDir(const std::string& relativePath) const;

    /** 返回数据根目录路径（只读） */
    const std::string& dataRoot() const;

private:
    std::string _dataRoot;
};

#endif // FILEMANAGER_HPP
