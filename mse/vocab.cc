#include "vocab.h"

#include <iostream>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include "rawtext-splitter.h"

using namespace std;

namespace ari = boost::algorithm;
namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

Vocab::Vocab(const string & corpus_path, const unsigned size,
         boost::shared_ptr<RawtextSplitter> text_splitter)
: text_splitter_(text_splitter) {
  map<string, unsigned> frequency;
  unsigned num_lines = 0;
  unsigned num_words = 0;

  const fs::path path(corpus_path);
  using recur_it = fs::recursive_directory_iterator;
  for (const auto & p: boost::make_iterator_range(recur_it(path), {})) {
    if (fs::is_directory(p)) {
      cout << p << endl;
    } else {
      ifstream ifs(p.path().string());
      string line;
      while (getline(ifs, line)) {
        // for empty line, header and footer
        if (line.empty() || line.find("<doc") == 0 || line.find("</doc") == 0) {
          continue;
        }
        auto tokens = text_splitter_->Split(line);
        for (const auto & t: tokens) {
          ++frequency[ari::to_lower_copy(t)];
        }
        ++num_lines;
        num_words += tokens.size();
      }
    }
  }
  num_words_ = num_words;

  vector<pair<unsigned, string>> frequencies;
  for (const auto & f: frequency) {
    frequencies.emplace_back(f.second, f.first);
  }

  sort(frequencies.begin(), frequencies.end(), [](const auto & lhs, const auto & rhs) {
    if (lhs.first > rhs.first) { return true; }
    else if (lhs.first < rhs.first) { return false; }
    else { return lhs.second < rhs.second; }
  });

  frequencies_.emplace_back(num_words);
  frequencies_.emplace_back(num_lines);
  frequencies_.emplace_back(num_lines);
  num_to_words_.emplace_back("<UNK>");
  num_to_words_.emplace_back("<s>");
  num_to_words_.emplace_back("</s>");
  word_to_nums_["<UNK>"] = 0;
  word_to_nums_["<s>"] = 1;
  word_to_nums_["</s>"] = 2;
  unsigned num_kind_of_words = 3;
  for (const auto & f: frequencies) {
    frequencies_.emplace_back(f.first);
    frequencies_[0] -= f.first;
    num_to_words_.emplace_back(f.second);
    word_to_nums_[f.second] = num_kind_of_words++;
    if (frequencies_.size() >= size + 3) { break; }
  }
  size_ = frequencies_.size();
}

unsigned Vocab::frequency(const unsigned id) const {
  if (id > frequencies_.size()) {
    ostringstream oss;
    oss << __FILE__ << ": " << __LINE__ << "ERROR: "
        << "Id{" << id << "} is out of range";
    throw range_error(oss.str());
  } else {
    return frequencies_[id];
  }
}

string Vocab::word(const unsigned id) const {
  if (id > num_to_words_.size()) {
    ostringstream oss;
    oss << __FILE__ << ": " << __LINE__ << "ERROR: "
        << "Id{" << id << "} is out of range";
    throw range_error(oss.str());
  } else {
    return num_to_words_[id];
  }
}

unsigned Vocab::id(const string & word) const {
  auto it = word_to_nums_.find(word);
  if (it == word_to_nums_.end()) {
    return 0;  // <UNK>
  } else {
    return it->second;
  }
}

vector<string> Vocab::ConvertToWords(const vector<unsigned> ids) const {
  vector<string> words;
  for (const auto & id: ids) {
    words.emplace_back(word(id));
  }
  return words;
}

vector<unsigned> Vocab::ConvertToIds(const vector<string> & words) const {
  vector<unsigned> ids;
  for (const auto & word: words) {
    ids.emplace_back(id(word));
  }
  return ids;
}
