#pragma once

#include "synchronized.h"

#include <istream>
#include <ostream>
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <deque>
#include <future>

using namespace std;

struct Item {
    size_t docid;
    size_t hits;
};

class InvertedIndex {
public:
    InvertedIndex() = default;

    explicit InvertedIndex(istream& document_input);

    void Add(string&& document);

    const vector<Item>& Lookup(string_view word) const;

    const string& GetDocument(size_t docid) const {
        return docs[docid];
    }

    size_t GetDocsSize() const {
        return docs.size();
    }

    const string& Check() const {
        return docs.back();
    }

private:
    map<string_view, vector<Item>> index;
    deque<string> docs;

    static const vector<Item> empty_list;
};

class SearchServer {
public:
    SearchServer() = default;

    explicit SearchServer(istream &document_input);

    void UpdateDocumentBase(istream &document_input);

    void AddQueriesStream(istream &query_input, ostream &search_results_output);

    void Synchronize();
private:
    Synchronized<InvertedIndex> index;
    deque<future<void>> futures;

    bool firstUpdate = true;
};
