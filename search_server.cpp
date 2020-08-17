#include "search_server.h"
#include "iterator_range.h"
#include "profile.h"
#include "test_runner.h"// for cout

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <numeric> // iota

vector<string> SplitIntoWords(const string &line) {
    istringstream words_input(line);
    return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

SearchServer::SearchServer(istream &document_input) {
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream &document_input) {
    InvertedIndex new_index;

    for (string current_document; getline(document_input, current_document);) {
        new_index.Add(move(current_document));
    }

    index = move(new_index);
}

void SearchServer::AddQueriesStream(
        istream &query_input, ostream &search_results_output) {

    // duration tests
    TotalDuration lookup("search_server.cpp: Total lookup in an inverted index");
    TotalDuration split("search_server.cpp: Total splitting into words");
    TotalDuration sort("search_server.cpp: Total sorting of hits");
    TotalDuration response("search_server.cpp: Total forming a response");

    const size_t DOCS_NU = index.GetNumberOfDocs();
    const size_t MAX_REL_DOCS_NU = 5;
    vector<size_t> doc_counts(DOCS_NU, 0);
    vector<size_t> docids(DOCS_NU);

    for (string current_query; getline(query_input, current_query);) {

        fill(doc_counts.begin(), doc_counts.end(), 0);
        iota(docids.begin(), docids.end(), 0);

        vector<string> words;
        {
            ADD_DURATION(split);
            words = SplitIntoWords(current_query);
        }

        {
            for (const auto &word : words) {
                ADD_DURATION(lookup);
                for (const size_t docid : index.Lookup(word)) {
                    doc_counts[docid]++;
                }
            }
        }

        {
            ADD_DURATION(sort);
            partial_sort(docids.begin(),
                         Head(docids, 5).end(), // in case of docids.size() < 5
                         docids.end(),
                         [&doc_counts](size_t lhs, size_t rhs) {
                             return make_pair(doc_counts[lhs], rhs) // does not work with minus because size_t is unsigned
                                    > make_pair(doc_counts[rhs], lhs);
                         });
        }

        {
            ADD_DURATION(response);
            search_results_output << current_query << ':';
            for (auto docid : Head(docids, 5)) {
                if (doc_counts[docid] == 0) break;
                search_results_output << " {"
                                      << "docid: " << docid << ", "
                                      << "hitcount: " << doc_counts[docid] << '}';
            }
            search_results_output << endl;
        }
    }
}

void InvertedIndex::Add(string document) { // do not forget !
    docs.push_back(document);

    const size_t docid = docs.size() - 1;
    for (auto &word : SplitIntoWords(move(document))) {
        index[move(word)].push_back(docid);
    }
}

list<size_t> InvertedIndex::Lookup(const string &word) const {
    if (auto it = index.find(word); it != index.end()) {
        return it->second;
    } else {
        return {};
    }
}
