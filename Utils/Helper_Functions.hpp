#include <vector>
#include <iostream>
#include <sstream>
#include <map>

namespace smh
{
    std::vector<std::string> split_string(const std::string &input, char delimiter = ';')
    {
        std::vector<std::string> tokens;
        std::stringstream ss(input);
        std::string token;

        while (std::getline(ss, token, delimiter))
            if (!token.empty()) // skip empty tokens
                tokens.push_back(token);

        return tokens;
    }

} // namespace smh
