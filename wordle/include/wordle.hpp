// wordle.hpp
// This file defines classes for working with Wordle.
// Some are simply wrappers around std::array.
// This is to add additional methods, and to override
// operator<< for printing.

#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>

namespace wordle {

// 5-letter words
// Stored with A->0, Z->26 instead of ASCII
// Invalid/Underscore characters are stored as 0xff
class Word : public std::array<uint8_t, 5> {
public:
  // Construct a word with all underscores
  Word();
  // Construct a word from an ASCII-encoded string
  // Throws a runtime error if any char at index 0-4 isn't A-Z
  Word(const char *word);
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
  Response();
  // Produces the next combination. Returns false if there is no next combination.
  bool next_combination();
  // Prints the word with ANSI colors based on the response
  std::ostream& write_ansi(std::ostream &out, const Word& w) const;
};

std::ostream& operator<<(std::ostream& out, const Response &r);

// Gamestate
// The public information at a given point in the game
class State {
public:
  State();
  State(const State& s) : yellow(s.yellow), green(s.green) {}

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