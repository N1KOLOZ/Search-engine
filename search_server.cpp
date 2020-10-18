#include "profile.h"

#include "search_server.h"
#include "iterator_range.h"
#include "parse.h"

#include <algorithm>
#include <numeric>
#include <functional>

InvertedIndex::InvertedIndex(std::istream& document_input) {
    for (std::string current_document; getline(document_input, current_document); ) {
        Add(std::move(current_document));
    }
}

void InvertedIndex::Add(std::string&& document) {
    docs.push_back(std::move(document));

    const size_t docid = docs.size() - 1;

    std::map<std::string_view, size_t> words_to_hits;
    for (std::string_view word : SplitIntoWordsView(docs.back())) {
        ++words_to_hits[word];
    }

    for(const auto& [word, hits] : words_to_hits) {
        index[word].push_back({docid, hits});
    }
}

const std::vector<Item>& InvertedIndex::Lookup(std::string_view word) const {
    static const std::vector<Item> empty = {};

    if (auto it = index.find(word); it != index.end()) {
        return it->second;
    } else {
        return empty;
    }
}

SearchServer::SearchServer(std::istream& document_input) {
    UpdateDocumentBase(document_input);
}

void UpdateDocumentBaseSingleThread(std::istream& document_input, Synchronized<InvertedIndex>& sync_index) {
    InvertedIndex new_index(document_input);

    auto access = sync_index.GetAccess();
    auto& index = access.ref_to_value;
    index = std::move(new_index);
}

void SearchServer::UpdateDocumentBase(std::istream& document_input) {
    futures.push_back(async(UpdateDocumentBaseSingleThread, ref(document_input), std::ref(sync_index)));

    if (firstUpdate) {
        firstUpdate = false;
        futures.back().get();
        futures.pop_back();
    }
}

void AddQueriesStreamSingleThread(
        std::istream& query_input, std::ostream& search_results_output, Synchronized<InvertedIndex>& sync_index) {

    // duration tests
    TotalDuration lookup("search_server.cpp: Total lookup in an inverted index");
    TotalDuration sort("search_server.cpp: Total sorting of hits");
    TotalDuration response("search_server.cpp: Total forming a response");

    const unsigned MAX_REL_DOCS_NUM = 5;

    std::vector<size_t> doc_counts;
    std::vector<size_t> docids;

    for (std::string current_query; getline(query_input, current_query); ) {

        const size_t DOCS_NUM = sync_index.GetAccess().ref_to_value.GetDocsSize();

        doc_counts.assign(DOCS_NUM, 0);
        docids.resize(DOCS_NUM);
        iota(docids.begin(), docids.end(), 0);

        std::vector<std::string_view> words = SplitIntoWordsView(current_query);

        {
            // area under mutex
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
            std::partial_sort(docids.begin(),
                         Head(docids, MAX_REL_DOCS_NUM).end(), // if docids.size() < 5 Head will return full range
                         docids.end(),
                         [&doc_counts](size_t lhs, size_t rhs) {
                             return std::make_pair(doc_counts[lhs], rhs) // !!! size_t is unsigned
                                    > std::make_pair(doc_counts[rhs], lhs);
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
        std::istream& query_input, std::ostream& search_results_output) {

    futures.push_back(async(AddQueriesStreamSingleThread,
                      std::ref(query_input),
                      std::ref(search_results_output),
                      std::ref(sync_index))
                      );
}

void SearchServer::Synchronize() {
    for (auto& future: futures) {
        future.get();
    }

    futures.clear();
}
