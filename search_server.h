#pragma once

#include "synchronized.h"

#include <istream>
#include <ostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <string_view>
#include <deque>
#include <future>

struct Item {
    size_t docid;
    size_t hits;
};

class InvertedIndex {
public:
    InvertedIndex() = default;

    explicit InvertedIndex(std::istream& document_input);

    void Add(std::string&& document);

    const std::vector<Item>& Lookup(std::string_view word) const;

    const std::string& GetDocument(size_t docid) const {
        return docs[docid];
    }

    size_t GetDocsSize() const {
        return docs.size();
    }

private:
    std::map<std::string_view, std::vector<Item>> index;
    std::deque<std::string> docs;
};

class SearchServer {
public:
    SearchServer() = default;

    explicit SearchServer(std::istream& document_input);

    void UpdateDocumentBase(std::istream& document_input);

    void AddQueriesStream(std::istream& query_input, std::ostream& search_results_output);

    void Synchronize();
private:
    Synchronized<InvertedIndex> sync_index;
    std::deque<std::future<void>> futures;

    bool firstUpdate = true;
};
