#pragma once

#include <filesystem>

class OverwriteManager
{
public:
    OverwriteManager(std::filesystem::path directory);

    /**
     * Returns case-correct path based on case-sensitivity. If the path does not
     * exist it will also create the directory containing the requested file/
     * directory.
     */
    std::filesystem::path get_directory(std::filesystem::path path);

    void set_case_insensitive(bool value);

private:
    bool compare(std::string s1, std::string s2);

    std::filesystem::path get_directory_iterator(
        std::filesystem::path::iterator begin,
        std::filesystem::path::iterator end,
        std::filesystem::path current_dir);

    std::filesystem::path m_path;
    bool m_case_insensitive;
};