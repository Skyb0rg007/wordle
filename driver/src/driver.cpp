#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <wordle.hpp>

static std::optional<wordle::Word> input(const std::vector<wordle::Word>& wordlist);
static std::vector<wordle::Word> load_wordlist(const char *filename);

class Strategy {
public:
  Strategy(const std::vector<wordle::Word>& wl) : wordlist(wl) {}
  virtual wordle::Response respond(const wordle::State& state, const wordle::Word& guess) = 0;
protected:
  const std::vector<wordle::Word>& wordlist;
};

class Standard : public Strategy {
public:
  Standard(const std::vector<wordle::Word>& wl);
  wordle::Response respond(const wordle::State&, const wordle::Word& guess) override;
  wordle::Word get_secret() const { return secret; }
private:
  wordle::Word secret;
};

class Absurd : public Strategy {
public:
  Absurd(const std::vector<wordle::Word>& wl);
  wordle::Response respond(const wordle::State&, const wordle::Word& guess) override;
};

int main(int argc, char *argv[]) {
  std::srand(std::time(nullptr));

  // Argument parsing
  auto usage = [progname=argv[0]]() {
    std::cerr << "Usage: " << progname << " <strategy> <wordlist>" << std::endl;
  };
  if (argc < 3) {
    usage();
    return 1;
  }
  const char *filename = argv[2];

  auto wordlist = load_wordlist(filename);
  if (wordlist.size() == 0) {
    std::cerr << "Error loading wordlist" << std::endl;
    return 1;
  }

  std::unique_ptr<Strategy> strat;
  if (std::strcmp(argv[1], "standard") == 0) {
    strat = std::make_unique<Standard>(wordlist);
  } else if (std::strcmp(argv[1], "absurd") == 0) {
    strat = std::make_unique<Absurd>(wordlist);
  } else {
    usage();
    return 1;
  }

  wordle::State state{};
  std::vector<std::pair<wordle::Word, wordle::Response>> history;
  while (true) {
    for (auto [g, r] : history) {
      r.write_ansi(std::cout, g) << std::endl;
    }
    auto w = input(wordlist);
    if (!w.has_value()) {
      return 1;
    }
    auto guess = w.value();
    auto response = strat->respond(state, guess);
    history.emplace_back(guess, response);
    state.update(guess, response);
    std::clog << state << std::endl;

    auto final = state.final();
    if (final.has_value() && final.value() == guess) {
      break;
    }

    if (auto std = dynamic_cast<Standard&>(*strat); !state.matches(std.get_secret())) {
      std::clog << "Doesn't match secret!" << std::endl;
      return 2;
    } else {
      std::clog << "Matches secret" << std::endl;
    }

    auto possible = std::find_if(
        wordlist.begin(),
        wordlist.end(), 
        [&](const wordle::Word& w) { return state.matches(w); });

    if (possible == wordlist.end()) {
      std::cout << "No possible words!!" << std::endl;
    } else {
      std::cout << "Possible: " << *possible << std::endl;
    }
  }
  std::cout << "Good job! Solved in " << history.size() << " guesses" << std::endl;
  return 0;
}

static std::vector<wordle::Word> load_wordlist(const char *filename)
{
  std::vector<wordle::Word> wordlist;
  std::ifstream in(filename);
  std::array<char, 6> buf{};
  while (true) {
    if (!in.read(buf.data(), buf.size())) {
      break;
    }
    wordlist.emplace_back(buf.data());
  } 
  return wordlist;
}

static std::optional<wordle::Word> input(const std::vector<wordle::Word>& wordlist) {
  std::string line;
  wordle::Word guess;
  while (true) {
    std::cout << "Enter guess: ";
    std::getline(std::cin, line);
    if (!std::cin) {
      return std::nullopt;
    }

    if (line.size() > 5) {
      std::cout << "Invalid line: too many characters" << std::endl;
      continue;
    } else if (line.size() < 5) {
      std::cout << "Invalid line: too few characters" << std::endl;
      continue;
    }

    if (std::any_of(line.begin(), line.end(), [](char c) { return c < 'A' || 'Z' < c; })) {
      std::cout << "Invalid line: characters must be in range 'A'-'Z'" << std::endl;
      continue;
    } else {
      guess = line.data();
    }

    if (std::find(wordlist.begin(), wordlist.end(), guess) == wordlist.end()) {
      std::cout << "Word is not in the wordlist" << std::endl;
      continue;
    }

    return guess;
  }
}

Standard::Standard(const std::vector<wordle::Word>& wl) : Strategy(wl) {
  if (wl.size() == 0)
    throw new std::runtime_error("Invalid wordlist!");
  secret = wl[std::rand() % wl.size()];
  std::clog << "Secret: " << secret << std::endl;
}

wordle::Response Standard::respond(const wordle::State& state, const wordle::Word& guess) {
  if (!state.matches(secret)) {
    throw new std::runtime_error("State doesn't match secret");
  }
  wordle::Response r{};
  std::array<uint8_t, 26> s_occurs{};
  for (size_t i = 0; i < secret.size(); i++) {
    if (secret[i] == guess[i]) {
      r[i] = wordle::Color::GREEN;
    } else {
      s_occurs[secret[i]]++;
    }
  }
  for (size_t i = 0; i < secret.size(); i++) {
    if (r[i] == wordle::Color::GRAY && s_occurs[guess[i]] > 0) {
      s_occurs[guess[i]]--;
      r[i] = wordle::Color::YELLOW;
    }
  }
  return r;
}

Absurd::Absurd(const std::vector<wordle::Word>& wl) : Strategy(wl) {
}

static int response_colors(const wordle::Response& r) {
  return std::count_if(
    r.begin(), 
    r.end(), 
    [](const wordle::Color& c) { return c != wordle::Color::GRAY; });
}

static int response_greens(const wordle::Response& r) {
  return std::count_if(
    r.begin(), 
    r.end(), 
    [](const wordle::Color& c) { return c == wordle::Color::GREEN; });
}

wordle::Response Absurd::respond(const wordle::State& state, const wordle::Word& guess) {
  {
    std::cout << "Wordlist size: " << wordlist.size() << std::endl;
    auto w = std::find_if(wordlist.begin(), wordlist.end(), [&](const wordle::Word& word) { return state.matches(word); });
    if (w == wordlist.end())
      throw new std::runtime_error("State has no matches");
    std::clog << "Current state matches something, ex. " << *w << std::endl;
  }

  using T = std::pair<wordle::Response, int>;
  std::vector<T> ranks;
  wordle::Response r;
  int not_invalid = 0;
  do {
    wordle::State newstate = state;
    bool ok = newstate.update(guess, r);
    if (!ok)
      continue;
    not_invalid++;
    int rank =
      std::count_if(
        wordlist.begin(),
        wordlist.end(),
        [&newstate](const wordle::Word& w) {
          return newstate.matches(w);
        });
    if (rank != 0)
      ranks.emplace_back(r, rank);
  } while (r.next_combination());

  auto cmp = [](const T& p1, const T& p2) {
    if (p1.second != p2.second)
      return p1.second < p2.second;
    if (response_greens(p1.first) != response_greens(p2.first))
      return response_greens(p2.first) < response_greens(p1.first);
    return response_colors(p2.first) < response_colors(p1.first);
  };
  auto max = std::max_element(ranks.begin(), ranks.end(), cmp);
  if (max == ranks.end()) {
    std::cout << "No best response! " << ranks.size() << " " << not_invalid << std::endl;
    throw new std::runtime_error("No best response");
  } else {
    std::clog << "Ranks: " << ranks.size() << ", not invalid: " << not_invalid << std::endl;
    std::clog << "Best rank: " << max->second << std::endl;
    return max->first;
  }
}
