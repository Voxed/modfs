#include "overwritemanager.hh"
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

/* Common helper functions */

static std::string lowercase(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

/* Member definitions */

OverwriteManager::OverwriteManager(fs::path path)
{
    this->m_path = path;
}

void OverwriteManager::set_case_insensitive(bool value)
{
    this->m_case_insensitive = value;
}

fs::path OverwriteManager::get_directory(fs::path path)
{
    return get_directory_iterator(path.begin(), path.end(),
                                  this->m_path);
}

bool OverwriteManager::compare(std::string s1, std::string s2)
{
    if (this->m_case_insensitive)
        return lowercase(s1) == lowercase(s2);
    else
        return s1 == s2;
}

fs::path OverwriteManager::get_directory_iterator(
    fs::path::iterator path_begin,
    fs::path::iterator path_end,
    fs::path current_dir)
{
    if (path_begin == path_end)
    {
        std::filesystem::create_directories(current_dir);
        return current_dir;
    }
    fs::path element = *path_begin;
    if (std::filesystem::exists(current_dir))
        for (const fs::directory_entry &entry :
             fs::directory_iterator(current_dir))
        {
            fs::path entry_name = entry.path().filename();
            if (this->compare(element, entry_name))
            {
                return this->get_directory_iterator(
                    std::next(path_begin), path_end, current_dir / entry_name);
            }
        }
    return this->get_directory_iterator(
        std::next(path_begin), path_end, current_dir / element);
    ;
}