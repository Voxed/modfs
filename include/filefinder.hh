#pragma once

#include <filesystem>
#include <vector>
#include <optional>

class FileFinder
{
public:
    void add_source(std::filesystem::path source);
    void set_case_insensitive(bool value);
    std::vector<std::filesystem::path> search_full(std::filesystem::path path);
    std::optional<std::filesystem::path> search(std::filesystem::path path);

private:
    bool compare(std::string s1, std::string s2);
    std::vector<std::filesystem::path>
    search_iterator(std::filesystem::path::iterator path_begin,
                    std::filesystem::path::iterator path_end,
                    std::filesystem::path current_dir,
                    bool single);
    bool m_case_insensitive;
    std::vector<std::filesystem::path> m_sources;
};