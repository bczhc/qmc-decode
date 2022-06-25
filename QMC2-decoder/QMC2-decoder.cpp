#include <iostream>
#include <filesystem>
#include <cassert>
#include "third_party/cpp-lib/string.hpp"
#include "third_party/cpp-lib/symbol_table.hpp"
#include "lib.h"
#include "qmc2.h"
#include "qmcflac.h"
#include "def.h"
#include <array>

using namespace std;
namespace fs = std::filesystem;

array<u8, 4> read_qmc2_tag(const char *file) {
    FILE *fp = fopen(file, "rb");
    if (fp == nullptr) {
        perror("Failed to open file");
    }
    fseek(fp, 4, SEEK_END);
    array<u8, 4> buf{};
    auto size = fread(buf.data(), 1, 4, fp);
    assert(size == 4);
    return buf;
}

int decode_qmc2(const char *input, const char *output, const char *ekey) {
    auto tag = read_qmc2_tag(input);

    if (memcmp(tag.data(), "QTag", 4) == 0) {

        return qmc2::decode(input, output);

    } else if (memcmp(tag.data(), "STag", 4) == 0) {

        if (ekey == nullptr) {
            cerr << "EKey is missing for decrypting files with STag!";
            return 1;
        }
        return qmc2::decodeWithEKey(input, output, ekey);

    } else {
        cerr << "Unknown qmc2 tag: ";
        for (const auto &b: tag) {
            cerr << b << ' ';
        }
        cerr << endl;
        return 1;
    }
}

SymbolTable<String, String> *extension_map;

SymbolTable<String, String> *create_extension_map() {
    auto map = new SymbolTable<String, String>;
    map->put("qmcflac", "flac");
    map->put("qmc0", "mp3");
    map->put("mflac0", "flac");
    map->put("mgg1", "ogg");
    return map;
}

template<typename K, typename V>
bool map_contains_key(const SymbolTable<K, V> &map, const K &key) {
    auto iter = extension_map->getIterator();
    while (iter.hasNext()) {
        if (iter.next().key == key) {
            return true;
        }
    }
    return false;
}

/**
 * 
 * @param input_path input file path, must be a regular file
 * @param output_path output file path, can be a directory (auto output filename assigned)
 * @param ekey nullable ekey
 * @return 
 */
int decode_single(const char *input_path, const char *output_path, const char *ekey) {
    auto extension = getFileExtension(input_path);
    if (extension.length() == 0) {
        cerr << "File extension not supported yet" << endl;
        return 1;
    }

    cout << "Decrypting " << input_path << "..." << endl;

    String output_file;
    if (fs::is_directory(output_path)) {
        auto replaced_ext = extension_map->get(extension);
        auto path = fs::path(output_path);

        auto out_filename = fs::path(input_path).replace_extension(
                fs::path(string(".") + replaced_ext.getCString())
        ).filename();
        path += out_filename;
        output_file = path.c_str();
    } else {
        output_file = output_path;
    }

    auto output_file_str = output_file.getCString();

    if (extension == "qmcflac" || extension == "qmc0") {
        auto r = qmcflac::decode(input_path, output_file_str);
        if (r != 0) return r;
    } else if (extension == "mflac0" || extension == "mgg1") {
        auto r = decode_qmc2(input_path, output_file_str, ekey);
        if (r != 0) return r;
    } else {
        assert(!"unreachable");
    }

    cout << "Done. Saved to " << output_file_str << endl;
    return 0;
}

int main(int argc, char **argv) {
    auto basename = fs::path(argv[0]).filename();
    if (argc - 1 != 2 && argc - 1 != 3) {
        cerr << "Usage: " << basename.c_str() << " <input> <output> [<ekey>]" << endl;
        return 1;
    }
    auto args = argv + 1;
    auto input_path = args[0];
    auto output_path = args[1];
    const char *ekey = nullptr;
    if (argc - 1 == 3) ekey = args[2];

    extension_map = create_extension_map();

    if (fs::is_directory(input_path)) {
        // batch decryption
        auto iter = fs::directory_iterator(input_path);
        for (const auto &entry: iter) {
            const string &extension = entry.path().extension().string();

            if (map_contains_key(*extension_map, String(extension.c_str()))
                && extension != "mflac0" && extension == "mgg1") {
                decode_single(entry.path().c_str(), output_path, ekey);
            }
        }
    } else {
        // for a single file
        return decode_single(input_path, output_path, ekey);
    }

    delete extension_map;

    return 0;
}
