#include "parse.h"

#include <iterator>

string_view Strip(string_view sv) {
    while (!sv.empty() && isspace(sv.front())) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && isspace(sv.back())) {
        sv.remove_suffix(1);
    }
    return sv;
}

vector<string_view> SplitBy(string_view sv, char sep) {
    vector<string_view> result;
    while (!sv.empty()) {
        size_t sep_pos = sv.find(sep);
        result.push_back(sv.substr(0, sep_pos));
        sv.remove_prefix(sep_pos != sv.npos ? sep_pos + 1 : sv.size());
    }
    return result;
}

void LeftStrip(string_view& sv, char sep) {
    while (!sv.empty() && sv.front() == sep) {
        sv.remove_prefix(1);
    }
}

vector<string_view> SplitIntoWordsView(string_view sv, char sep) {
    vector<string_view> result;
    LeftStrip(sv, sep);
    while (!sv.empty()) {
        size_t sep_pos = sv.find(sep);
        result.push_back(sv.substr(0, sep_pos));
        sv.remove_prefix(sep_pos != sv.npos ? sep_pos + 1 : sv.size());
        LeftStrip(sv, sep);
    }
    return result;
}