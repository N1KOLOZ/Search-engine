#include "profile.h"

#include "search_server.h"
#include "iterator_range.h"
#include "parse.h"

#include <algorithm>
#include <numeric>
#include <functional> //ref

const vector<Item> InvertedIndex::empty = {};

InvertedIndex::InvertedIndex(istream &document_input) {
    for (string current_document; getline(document_input, current_document); ) {
        Add(move(current_document));
    }
}

void InvertedIndex::Add(string&& document) {
    docs.push_back(move(document));

    const size_t docid = docs.size() - 1;

    map<string_view, size_t> word_to_hits;
    for (string_view word : SplitIntoWordsView(docs.back())) {
        ++word_to_hits[word];
    }

    for(const auto& [word, hits] : word_to_hits) {
        index[word].push_back({docid, hits});
    }
}

const vector<Item>& InvertedIndex::Lookup(string_view word) const {
    if (auto it = index.find(word); it != index.end()) {
        return it->second;
    } else {
        return empty;
    }
}

SearchServer::SearchServer(istream &document_input) {
    UpdateDocumentBase(document_input);
}

void UpdateDocumentBaseSingleThread(istream& document_input, Synchronized<InvertedIndex>& sync_index) {
    InvertedIndex new_index(document_input);

    auto access = sync_index.GetAccess();
    auto& index = access.ref_to_value;
    index = move(new_index);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {
    futures.push_back(async(UpdateDocumentBaseSingleThread, ref(document_input), ref(sync_index)));

    if (firstUpdate) {
        firstUpdate = false;
        futures.back().get();
        futures.pop_back();
    }
}

void AddQueriesStreamSingleThread(
        istream &query_input, ostream &search_results_output, Synchronized<InvertedIndex>& sync_index) {

    // duration tests
    TotalDuration lookup("search_server.cpp: Total lookup in an inverted index");
    TotalDuration sort("search_server.cpp: Total sorting of hits");
    TotalDuration response("search_server.cpp: Total forming a response");

    const size_t MAX_REL_DOCS_NUM = 5;

    vector<size_t> doc_counts;
    vector<size_t> docids;

    for (string current_query; getline(query_input, current_query); ) {

        const size_t DOCS_NUM = sync_index.GetAccess().ref_to_value.GetDocsSize();
        doc_counts.assign(DOCS_NUM, 0);
        docids.resize(DOCS_NUM);
        iota(docids.begin(), docids.end(), 0);

        vector<string_view> words = SplitIntoWordsView(current_query);

        {
            auto access = sync_index.GetAccess();
            auto& index = access.ref_to_value;
            {
                for (auto word : words) {
                    ADD_DURATION(lookup);
                    for (auto [docid, hits] : index.Lookup(word)) {
                        doc_counts[docid] += hits;
                    }
                }
            }
        }

        {
            ADD_DURATION(sort);
            partial_sort(docids.begin(),
                         Head(docids, MAX_REL_DOCS_NUM).end(), // in case of docids.size() < 5
                         docids.end(),
                         [&doc_counts](size_t lhs, size_t rhs) {
                             return make_pair(doc_counts[lhs], rhs) // does not work with minus because of size_t
                                    > make_pair(doc_counts[rhs], lhs);
                         });
        }

        {
            ADD_DURATION(response);
            search_results_output << current_query << ':';
            for (auto docid : Head(docids, MAX_REL_DOCS_NUM)) {
                if (doc_counts[docid] == 0) break;
                search_results_output << " {"
                                      << "docid: " << docid << ", "
                                      << "hitcount: " << doc_counts[docid] << '}';
            }
            search_results_output << '\n';
        }
    }
}

void SearchServer::AddQueriesStream(
        istream& query_input, ostream& search_results_output) {

    futures.push_back(async(AddQueriesStreamSingleThread,
                      ref(query_input),
                      ref(search_results_output),
                      ref(sync_index))
                      );
}

void SearchServer::Synchronize() {
    for (auto& future: futures) {
        future.get();
    }

    futures.clear();
}
