#include "filefinder.hh"

#include <filesystem>
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

void FileFinder::add_source(fs::path source)
{
    this->m_sources.push_back(source);
}

void FileFinder::set_case_insensitive(bool value)
{
    this->m_case_insensitive = value;
}

bool FileFinder::compare(std::string s1, std::string s2)
{
    if (this->m_case_insensitive)
        return lowercase(s1) == lowercase(s2);
    else
        return s1 == s2;
}

std::vector<fs::path> FileFinder::search_iterator(fs::path::iterator path_begin,
                                                  fs::path::iterator path_end,
                                                  fs::path current_dir,
                                                  bool single)
{
    if (path_begin == path_end)
        return {current_dir};
    fs::path element = *path_begin;
    std::vector<fs::path> matches;
    for (const fs::directory_entry &entry : fs::directory_iterator(current_dir))
    {
        fs::path entry_name = entry.path().filename();
        if (this->compare(element, entry_name))
        {
            std::vector<fs::path> new_matches = this->search_iterator(
                std::next(path_begin), path_end, current_dir / entry_name,
                single);
            if(single && !new_matches.empty())
                return {new_matches[0]};
            matches.insert(
                matches.end(), new_matches.begin(), new_matches.end());
        }
    }
    return matches;
}

std::vector<fs::path> FileFinder::search_full(fs::path path)
{
    std::vector<fs::path> matches;
    for (const fs::path &source : this->m_sources)
    {
        std::vector<fs::path> new_matches =
            search_iterator(path.begin(), path.end(), source, false);
        matches.insert(
            matches.end(), new_matches.begin(), new_matches.end());
    }
    return matches;
}

std::optional<fs::path> FileFinder::search(
    fs::path path)
{
    for (const fs::path &source : this->m_sources)
    {
        std::vector<fs::path> matches =
            search_iterator(path.begin(), path.end(), source, true);
        if (!matches.empty())
            return matches[0];
    }
    return {};
}
