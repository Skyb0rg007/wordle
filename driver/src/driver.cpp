#include <iostream>
#include <fstream>
#include <optional>
#include <string>
#include <algorithm>
#include <vector>
#include <wordle.hpp>

static std::optional<wordle::Word> input(const std::vector<wordle::Word>& wordlist);
static wordle::Response build_response(const wordle::Word& secret, const wordle::Word& guess);

int main(int argc, char *argv[]) {
  std::vector<wordle::Word> wordlist;
  {
    const char *filename = "words.txt";
    if (argc > 1)
      filename = argv[1];
    std::ifstream in(filename);
    std::array<char, 6> buf{};
    while (true) {
      if (!in.read(buf.data(), buf.size())) {
        break;
      }
      wordlist.emplace_back(buf.data());
    }
  }
  if (wordlist.size() == 0) {
    std::cerr << "Error loading wordlist" << std::endl;
    return 1;
  }

  wordle::Word secret;
  {
    std::srand(std::time(nullptr));
    secret = wordlist[std::rand() % wordlist.size()];
    std::clog << "Secret: " << secret << std::endl;
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
    auto response = build_response(secret, guess);
    history.emplace_back(guess, response);
    state.update(guess, response);
    // std::clog << state << std::endl;

    auto final = state.final();
    if (final.has_value() && final.value() == guess) {
      break;
    }
  }
  std::cout << "Good job! Solved in " << history.size() << " guesses" << std::endl;
  return 0;
}

static wordle::Response build_response(const wordle::Word& secret, const wordle::Word& guess)
{
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
