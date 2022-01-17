#include "../src/DataSource.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "strnatcmp.hpp"

#include "utils.hpp"

#include "common.hpp"

int main(int argc, char** argv) {
  // std::cout << argc << std::endl;
  // std::cout << argv << std::endl;
  //FolderDataSource f(argc, argv, 0);
  //std::cout << f.files.size() << std::endl;
  // for (auto& file : f.files) {
  //   std::cout << "1" << std::endl;
  // }

    std::vector<std::string> files;
  std::string folderPath = argv[1];
  for (const auto & entry : fs::directory_iterator(folderPath)) {
        // Ignore files we treat specially:
        if (entry.is_directory() || endsWith(entry.path().string(), ".keypoints.txt") || endsWith(entry.path().string(), ".matches.txt") || entry.path().filename().string() == ".DS_Store") {
            continue;
        }
        files.push_back(entry.path().string());
    }

    // https://stackoverflow.com/questions/9743485/natural-sort-of-directory-filenames-in-c
    std::sort(files.begin(),files.end(),compareNat);

  for (auto& file : files) {
    //std::cout << "1" << std::endl;
     std::cout << file << std::endl;
  }
    
}
