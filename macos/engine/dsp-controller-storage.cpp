#include "dsp-controller.h"
#include <filesystem>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#ifdef FXSOUND_HAVE_DSP
#include "float32-engine.h"
#endif

namespace fxsound {
void DSPController::savePreset(const std::string& path) {
#ifdef FXSOUND_HAVE_DSP
    namespace fs = std::filesystem;
    const fs::path target(path);
    if (target.extension() != ".fac" || target.filename().empty() || !fs::is_directory(target.parent_path()))
        throw std::runtime_error("preset must be a .fac file in an existing directory");
    std::string pattern = (target.parent_path() / ".fxsound-preset-XXXXXX").string();
    if (!mkdtemp(pattern.data())) throw std::runtime_error("could not stage preset save");
    struct Cleanup {
        fs::path directory;
        ~Cleanup() { std::error_code error; fs::remove_all(directory, error); }
    } cleanup{pattern};
    if (dsp_->controls().savePreset(target.stem().wstring(), cleanup.directory.wstring()) != 0)
        throw std::runtime_error("preset save failed");
    const fs::path staged = cleanup.directory / target.filename();
    int fd = open(staged.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) throw std::runtime_error("could not verify saved preset");
    int synced = fsync(fd); int closed = close(fd);
    if (synced || closed) throw std::runtime_error("could not flush saved preset");
    fs::rename(staged, target);
#else
    (void)path; throw std::runtime_error("DSP unavailable in this build");
#endif
}
}
