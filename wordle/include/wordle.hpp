#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>

namespace wordle {

// 5-letter words
// Stored with A->0, Z->26 instead of ASCII
class Word {
public:
  using iterator = std::array<uint8_t, 5>::iterator;
  using const_iterator = std::array<uint8_t, 5>::const_iterator;

  Word() {
    std::fill(data.begin(), data.end(), 0xff);
  }
  Word(const char *word) {
    for (auto &c : data) {
      assert('A' <= *word && *word <= 'Z');
      c = *word++ - 'A';
    }
  }
  Word(uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4, uint8_t c5) :
    data({c1, c2, c3, c4, c5}) {}

  bool operator==(const Word &w) const { return this->data == w.data; }
  bool operator!=(const Word &w) const { return this->data != w.data; }

  uint8_t& operator[](size_t i) { return data.at(i); }
  const uint8_t &operator[](size_t i) const { return data.at(i); }
  constexpr iterator begin() { return data.begin(); }
  constexpr iterator end() { return data.end(); }
  constexpr const_iterator begin() const { return data.begin(); }
  constexpr const_iterator end() const { return data.end(); }
  constexpr size_t size() const { return data.size(); }
private:
  std::array<uint8_t, 5> data;
};

std::ostream& operator<<(std::ostream& out, const Word& w);

// The response color
enum class Color : uint8_t {
  GRAY = 0,
  YELLOW = 1,
  GREEN = 2
};

std::ostream& operator<<(std::ostream& out, Color c);

// Response sent back from the computer
class Response : public std::array<Color, 5> {
public:
  // Prints the word with ANSI colors based on the response
  std::ostream& write_ansi(std::ostream &out, const Word& w);
};

std::ostream& operator<<(std::ostream& out, const Response &r);

// Gamestate
// The public information at a given point in the game
class State {
public:
  State();

  // Update the state to account for the guess and response
  // Returns false if the pair is nonsensical
  // Note: the internals are unpredictable if update returns false
  // Use an assert() or copy the State before calling.
  bool update(const Word &w, const Response &r);

  // Determine if the given word is a possible secret
  bool matches(const Word &w) const;

  // Returns the built-up green word, or nullopt if it's incomplete
  std::optional<Word> final() const;
private:
  friend std::ostream& operator<<(std::ostream& out, const State& s);
  struct Yellow {
    uint8_t min : 2; // min occurrances of the letter
    uint8_t strict : 1; // is the above constraint strict?
    uint8_t indices : 5; // Bitset of the fields the letter is not
  };
  std::array<Yellow, 26> yellow;
  Word green;
};

std::ostream& operator<<(std::ostream& out, const State& s);

}