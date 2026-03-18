#include "compiler.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <sys/wait.h>

namespace {

int runCommand(const std::string& command) {
    const int status = std::system(command.c_str());
    if (status == -1) {
        return status;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return status;
}

std::string shellQuote(const std::string& input) {
    std::string quoted = "'";
    for (char ch : input) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

} // namespace

namespace hypreact::runtime {

namespace fs = std::filesystem;

RuntimeCompiler::RuntimeCompiler(std::string configRoot, std::string cacheRoot)
    : configRoot_(std::move(configRoot)),
      cacheRoot_(std::move(cacheRoot)) {}

const std::string& RuntimeCompiler::configRoot() const {
    return configRoot_;
}

const std::string& RuntimeCompiler::cacheRoot() const {
    return cacheRoot_;
}

std::string RuntimeCompiler::authoredLayoutBase(std::string_view layoutName) const {
    return (fs::path(configRoot_) / "layouts" / std::string(layoutName) / "index").string();
}

std::string RuntimeCompiler::compiledLayoutEntry(std::string_view layoutName) const {
    return (fs::path(cacheRoot_) / "layouts" / std::string(layoutName) / "index.js").string();
}

CompileResult RuntimeCompiler::prepareLayout(std::string_view layoutName) const {
    if (layoutName.empty()) {
        return {false, {}, std::string("layout name is empty")};
    }

    const fs::path authoredBase(authoredLayoutBase(layoutName));
    const fs::path authoredJs = authoredBase.string() + ".js";
    const fs::path authoredTs = authoredBase.string() + ".ts";
    const fs::path authoredJsx = authoredBase.string() + ".jsx";
    const fs::path authoredTsx = authoredBase.string() + ".tsx";
    const fs::path outputPath(compiledLayoutEntry(layoutName));
    fs::path sourcePath;

    std::error_code error;
    fs::create_directories(outputPath.parent_path(), error);
    if (error) {
        return {false, {}, std::string("failed to create cache directories: ") + error.message()};
    }

    if (fs::exists(authoredTsx)) {
        sourcePath = authoredTsx;
    } else if (fs::exists(authoredJsx)) {
        sourcePath = authoredJsx;
    } else if (fs::exists(authoredTs)) {
        sourcePath = authoredTs;
    } else if (fs::exists(authoredJs)) {
        sourcePath = authoredJs;
    } else {
        return {
            false,
            {},
            std::string("no authored layout source found for '") + std::string(layoutName) + "'"
        };
    }

    const bool needsRebuild = !fs::exists(outputPath)
        || fs::last_write_time(outputPath, error) < fs::last_write_time(sourcePath, error);

    if (error) {
        error.clear();
    }

    if (!needsRebuild) {
        return {true, outputPath.string(), std::nullopt};
    }

    const std::string command =
        "esbuild " + shellQuote(sourcePath.string()) +
        " --bundle --format=esm --platform=neutral --jsx=automatic --jsx-import-source=hypreact --outfile=" +
        shellQuote(outputPath.string());

    const int exitCode = runCommand(command);
    if (exitCode != 0) {
        return {
            false,
            {},
            std::string("esbuild failed for '") + std::string(layoutName) + "' with exit code " + std::to_string(exitCode)
        };
    }

    return {true, outputPath.string(), std::nullopt};
}

} // namespace hypreact::runtime
