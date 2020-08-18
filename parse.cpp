#include "parse.h"

#include <iterator>

string_view Strip(string_view s) {
    while (!s.empty() && isspace(s.front())) {
        s.remove_prefix(1);
    }
    while (!s.empty() && isspace(s.back())) {
        s.remove_suffix(1);
    }
    return s;
}

vector<string_view> SplitBy(string_view s, char sep) {
    vector<string_view> result;

    while (!s.empty() && s.front() == sep) {
        s.remove_prefix(1);
    }

    while (!s.empty()) {
        size_t sep_pos = s.find(sep);
        result.push_back(s.substr(0, sep_pos));
        s.remove_prefix(sep_pos != s.npos ? sep_pos + 1 : s.size());

        while (!s.empty() && s.front() == sep) {
            s.remove_prefix(1);
        }
    }
    return result;
}