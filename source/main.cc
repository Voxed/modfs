#define FUSE_USE_VERSION 31
//#include <fuse.h>
#include <fuse3/fuse.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "filefinder.hh"
#include "overwritemanager.hh"
#include <fstream>

static FileFinder file_finder;
static OverwriteManager overwrite_manager("");

/* Helper methods */

static std::filesystem::path extract_last(std::filesystem::path path,
                                          std::filesystem::path &lastcomponent)
{
    std::filesystem::path p;
    if (!(*std::prev(path.end())).string().empty())
    {
        lastcomponent = path.filename();
        p = path.parent_path();
    }
    else
    {
        p = path.parent_path();
        lastcomponent = p.filename();
        p = p.parent_path();
    }
    return p;
}

static std::string search(const char *path)
{
    return file_finder.search(path + 1).value_or("");
}

static std::string search_or_create(const char *path)
{
    std::optional<std::filesystem::path> p = file_finder.search(path + 1);
    if (p)
        return p.value();
    std::filesystem::path last_component;
    std::filesystem::path parent = extract_last(path + 1, last_component);
    parent = overwrite_manager.get_directory(parent);
    return parent / last_component;
}

static std::string search_and_copy(const char *path)
{
    std::optional<std::filesystem::path> p = file_finder.search(path + 1);
    if (p)
    {
        std::filesystem::path std_path = p.value();
        if (!std_path.string().rfind(overwrite_manager.get_directory(""), 0))
        {
            return p.value();
        }
        if (!std::filesystem::is_directory(std_path))
        {

            std::filesystem::path last_component;
            std::filesystem::path parent =
                extract_last(path + 1, last_component);
            parent = overwrite_manager.get_directory(parent);

            std::filesystem::copy(std_path,
                                  parent / last_component);

            return parent / last_component;
        }
    }
    return std::string("");
}

static std::string lowercase(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

static int errno_handler(int ret)
{
    if (ret == -1)
        return -errno;
    return ret;
}

/* Fuse methods */

static int modfs_release(const char *path, struct fuse_file_info *fi)
{
    close(fi->fh);
    return 0;
}

static int modfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags)
{
    // Test the gosh darn waters
    std::filesystem::path test_path = search(path);
    DIR *d = opendir(test_path.c_str());
    if (d == nullptr)
        return -errno;
    closedir(d);

    std::unordered_set<std::string> file_names({});
    for (const std::filesystem::path &path :
         file_finder.search_full(path + 1))
    {
        std::error_code error;
        if (std::filesystem::is_directory(path, error))
            for (const std::filesystem::directory_entry &e :
                 std::filesystem::directory_iterator(path))
            {
                std::string lowercase_name = lowercase(e.path().filename());
                if (file_names.count(lowercase_name) == 0)
                {
                    filler(buf, e.path().filename().c_str(), nullptr, 0,
                           (fuse_fill_dir_flags)0);
                    file_names.insert(lowercase_name);
                }
            }
    }
    return 0;
}

int modfs_open(const char *path_p, fuse_file_info *fi)
{
    std::string path_str;
    if ((fi->flags & O_WRONLY) == O_WRONLY || (fi->flags & O_RDWR) == O_RDWR)
        path_str = search_and_copy(path_p);
    else
    {
        path_str = search(path_p);
    }
    const char *path = path_str.c_str();
    int res = errno_handler(open(path, fi->flags));
    if (res < 0)
        return res;
    fi->fh = res;
    return 0;
}

int modfs_write(const char *path, const char *buf, size_t size, off_t offset,
                fuse_file_info *fi)
{
    int fd;
    if (fi == NULL)
        fd = open(search_and_copy(path).c_str(), O_WRONLY);
    else
        fd = fi->fh;
    if (fd == -1)
        return -errno;
    int res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    if (fi == NULL)
        close(fd);
    return res;
}

int modfs_readlink(const char *path_p, char *buf, size_t size)
{
    std::string path_str = search(path_p);
    const char *path = path_str.c_str();
    int res = errno_handler(readlink(path, buf, size));
    if (res < 0)
        return res;
    buf[res] = '\0';
    return 0;
}

// Generate a neat default operator. :)
#define GEN_DEF_OP(out, in...) [](const char *path_p, in) { \
    std::string path_str = search(path_p);                  \
    const char *path = path_str.c_str();                    \
    return errno_handler(out);                              \
}

// Generate a neat creating operator. :)
#define GEN_CRE_OP(out, in...) [](const char *path_p, in) { \
    std::string path_str = search_or_create(path_p);        \
    const char *path = path_str.c_str();                    \
    return errno_handler(out);                              \
}

// Generate a neat open/close operator. :(
#define GEN_OPCL_OP(out, ret, in...) [](const char *path, in) -> ret { \
    int fd;                                                            \
    if (fi == NULL)                                                    \
        fd = open(search(path).c_str(), O_RDONLY);                     \
    else                                                               \
        fd = fi->fh;                                                   \
    if (fd == -1)                                                      \
        return -errno;                                                 \
    ret res = out;                                                     \
    if (res == -1)                                                     \
        res = -errno;                                                  \
    if (fi == NULL)                                                    \
        close(fd);                                                     \
    return res;                                                        \
}

static const struct fuse_operations op = {
    .getattr = GEN_DEF_OP(lstat(path, stbuf), struct stat *stbuf,
                          fuse_file_info *f),
    .readlink = modfs_readlink,
    .mknod = GEN_CRE_OP(mknod(path, mode, rdev), mode_t mode, dev_t rdev),
    .mkdir = GEN_CRE_OP(mkdir(path, mode), mode_t mode),
    .symlink = GEN_CRE_OP(symlink(path, to), const char *to),
    .link = GEN_CRE_OP(link(path, to), const char *to),
    .chmod = GEN_DEF_OP(chmod(path, mode), mode_t mode, fuse_file_info *fi),
    .chown = GEN_DEF_OP(lchown(path, uid, gid), uid_t uid, gid_t gid,
                        fuse_file_info *fi),
    .truncate = GEN_DEF_OP(truncate(path, size), off_t size,
                           fuse_file_info *fi),
    .open = modfs_open,
    .read = GEN_OPCL_OP(pread(fd, buf, size, offset), int, char *buf,
                        size_t size, off_t offset, fuse_file_info *fi),
    .write = modfs_write,
    .statfs = GEN_DEF_OP(statvfs(path, stbuf), struct statvfs *stbuf),
    .release = modfs_release,
    .readdir = modfs_readdir,
    .lseek = GEN_OPCL_OP(lseek(fd, off, whence), off_t, off_t off,
                         int whence, fuse_file_info *fi),
};

static struct options
{
    const char *cfg;
} options;

#define OPTION(t, p)                      \
    {                                     \
        t, offsetof(struct options, p), 1 \
    }
static const struct fuse_opt option_spec[] = {
    OPTION("--cfg=%s", cfg),
    FUSE_OPT_END};

int main(int argc, char **argv)
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    options.cfg = nullptr;

    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    if (options.cfg == nullptr)
    {
        printf("Yah need to specify the mod config using \"--cfg=<path>\"\n");
        return EXIT_FAILURE;
    }

    nlohmann::json j;
    std::ifstream i(options.cfg);
    i >> j;

    std::string mod_directory = j["mod_directory"].get<std::string>();
    std::string game_directory = j["game_directory"].get<std::string>();
    std::string overwrite_directory = j["overwrite_directory"].get<std::string>();
    std::vector<std::string> mods = j["mods"].get<std::vector<std::string>>();

    overwrite_manager = OverwriteManager(overwrite_directory);
    file_finder.add_source(overwrite_directory);
    for (std::string s : mods)
    {
        printf("Adding %s\n", s.c_str());
        file_finder.add_source(std::filesystem::path(mod_directory) / s);
    }
    file_finder.add_source(game_directory);

    overwrite_manager.set_case_insensitive(true);
    file_finder.set_case_insensitive(true);

    //search_and_copy("/Fallout4");

    int ret = fuse_main(args.argc, args.argv, &op, NULL);
    return ret;
}