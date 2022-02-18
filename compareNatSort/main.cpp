#include "../src/DataSource.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "strnatcmp.hpp"

#include "utils.hpp"

#include "common.hpp"

int main(int argc, char **argv) {
  // std::cout << argc << std::endl;
  // std::cout << argv << std::endl;
  // FolderDataSource f(argc, argv, 0);
  // std::cout << f.files.size() << std::endl;
  // for (auto& file : f.files) {
  //   std::cout << "1" << std::endl;
  // }

  if (argc < 2) {
    std::cout << "This command prints all files matching a certain extension in a directory given. It does this printing in a \"natural\"/\"human-readable\" way with respect to numbers in that it prints numbers contained in filenames in increasing order instead of lexicographically, while also printing the filenames as alphabetically sorted.\n\tUsage: " << argv[0] << " <folder path> [optional extensions to match, else .mp4 is matched]\n\tExample: " << argv[0] << " folder . ''      (to match all extensions)" << std::endl;
    exit(1);
  }

  std::vector<std::string> files;
  std::string folderPath = argv[1];
  std::string extMatch = ".mp4";
  if (argc >= 3) {
    extMatch = argv[2];
  }
  for (const auto &entry : fs::directory_iterator(folderPath)) {
    // Ignore files we treat specially:
    // if (entry.is_directory() || endsWith(entry.path().string(),
    // ".keypoints.txt") || endsWith(entry.path().string(), ".matches.txt") ||
    // entry.path().filename().string() == ".DS_Store") {
    //     continue;
    // }
    if (!endsWith(entry.path().string(), extMatch)) {
      continue;
    }
    files.push_back(entry.path().string());
  }

  // https://stackoverflow.com/questions/9743485/natural-sort-of-directory-filenames-in-c
  std::sort(files.begin(), files.end(), compareNat);

  for (auto &file : files) {
    // std::cout << "1" << std::endl;
    std::cout << file << std::endl;
  }
}
