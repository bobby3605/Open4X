#include "base64.hpp"
#include <bitset>
#include <sstream>

std::vector<unsigned char> base64ToUChar(std::string base64) {
    std::stringstream b64(base64);
    std::vector<unsigned char> output;
    std::string buffer;
    int index = 0;
    int padding = 0;
    char c;
    while (b64.get(c)) {
        index += 6;
        if (c != '=') {
            auto it = decodeTable.find(c);
            if (it != decodeTable.end()) {
                buffer.append(it->second);
            }
        } else {
            padding++;
        }
        if (index == 24) {
            index = 0;
            output.push_back((unsigned char)std::bitset<8>(buffer.substr(0, 8)).to_ulong());
            if (padding == 1 || padding == 0) {
                output.push_back((unsigned char)std::bitset<8>(buffer.substr(8, 8)).to_ulong());
            }
            if (padding == 0) {
                output.push_back((unsigned char)std::bitset<8>(buffer.substr(16, 8)).to_ulong());
            }
            buffer.clear();
        }
    }
    return output;
}
