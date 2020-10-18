#pragma once

#include "iterator_range.h"

#include <string_view>
#include <sstream>
#include <vector>

template<typename Container>
std::string Join(char sep, const Container& cont) {
    std::ostringstream os;
    for (const auto& item : Head(cont, cont.size() - 1)) {
        os << item << sep;
    }
    os << *rbegin(cont);
    return os.str();
}

std::string_view Strip(std::string_view sv);

std::vector<std::string_view> SplitBy(std::string_view sv, char sep = ' ');

std::vector<std::string_view> SplitIntoWordsView(std::string_view line, char sep = ' ');


