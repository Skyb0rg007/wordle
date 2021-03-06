#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <cstring>
#include <wordle.hpp>

using namespace wordle;

Word::Word() {
    std::fill(begin(), end(), 0xff);
}

Word::Word(const char *word) {
  for (auto &c : *this) {
    if (*word < 'A' || 'Z' < *word) {
      throw new std::runtime_error("Invalid word");
    }
    c = *word++ - 'A';
  }
}

std::ostream& wordle::operator<<(std::ostream& out, const Word& w) {
  for (uint8_t i : w) {
    if (i == 0xff) {
      out << '_';
    } else {
      out << char('A' + i);
    }
  }
  return out;
}

std::ostream& Word::serialize(std::ostream& out) const {
    std::array<char, sizeof(Word)> buf;
    std::memcpy(buf.data(), this, sizeof(Word));
    out.write(buf.data(), buf.size());
    return out;
}

Word Word::deserialize(std::istream& in) {
    std::array<char, sizeof(Word)> buf;
    in.read(buf.data(), buf.size());
    Word w;
    std::memcpy(&w, buf.data(), buf.size());
    return w;
}

std::ostream& wordle::operator<<(std::ostream& out, Color c) {
  switch (c) {
    case Color::GRAY:
      out << "_";
      break;
    case Color::YELLOW:
      out << "Y";
      break;
    case Color::GREEN:
      out << "G";
      break;
  }
  return out;
}

std::ostream& wordle::operator<<(std::ostream& out, const Response &r) {
  for (auto c : r) {
    out << c;
  }
  return out;
}

Response::Response() {
  std::fill(begin(), end(), Color::GRAY);
}

bool Response::next_combination() {
  for (auto it = rbegin(); it != rend(); it++) {
    switch (*it) {
      case Color::GRAY:
        *it = Color::YELLOW;
        return true;
      case Color::YELLOW:
        *it = Color::GREEN;
        return true;
      case Color::GREEN:
        *it = Color::GRAY;
        break;
    }
  }
  return false;
}

std::ostream& Response::write_ansi(std::ostream &out, const Word& w) const {
  for (size_t i = 0; i < w.size(); i++) {
    const char *code = "";
    switch (at(i)) {
      case Color::GRAY:
        code = "246";
        break;
      case Color::YELLOW:
        code = "190";
        break;
      case Color::GREEN:
        code = "47";
        break;
    }
    out << "\u001b[38;5;" << code << "m" << char('A' + w[i]);
  }
  out << "\u001b[0m";
  return out;
}

State::State() : yellow(), green() {
  for (auto g : green) {
    assert(g == 0xff);
  }
}

bool State::update(const Word &w, const Response &r) {
  std::array<uint8_t, 26> w_occurs{};
  std::array<uint8_t, 26> k_occurs{};

  // Initialize arrays
  // w_occurs maps letters to how many times they appear in the guess
  // k_occurs maps letters to how many times they appear in the response (yellow or green)
  for (size_t i = 0; i < w.size(); i++) {
    w_occurs[w[i]]++;
    if (r[i] != Color::GRAY) {
      k_occurs[w[i]]++;
    }
    if (w[i] == green[i] && r[i] != Color::GREEN) {
      // Response and guess are incompatible
      return false;
    }
  }

  for (size_t i = 0; i < w.size(); i++) {
    // Set minimum occurance for w[i]
    if (w_occurs[w[i]] > k_occurs[w[i]]) {
      // Letter occurs more in guess than response: strict bound
      if (yellow[w[i]].min > k_occurs[w[i]]) {
        // Attempt to set a strict bound when there is already a larger minimum!
        return false;
      }
      yellow[w[i]].strict = true;
      yellow[w[i]].min = k_occurs[w[i]];
      if (k_occurs[w[i]] == 0) {
        yellow[w[i]].indices = 0x1f;
      }
    } else {
      // Letter occurs equal in guess and response: weak bound
      yellow[w[i]].min = std::max(yellow[w[i]].min, k_occurs[w[i]]);
    }

    switch (r[i]) {
      case Color::GRAY:
        // Already handled by above
        break;
      case Color::YELLOW:
        // If yellow, character cannot appear at that index
        yellow[w[i]].indices |= 1 << i;
        break;
      case Color::GREEN:
        // If green, set the output and possibly update bounds
        if (green[i] == w[i]) {
          // Green already set
          break;
        }
        if (green[i] != 0xff) {
          // Conflicting green outputs
          return false;
        }
        green[i] = w[i];

        uint8_t occurs = std::count_if(
          green.begin(),
          green.end(),
          [k=w[i]](uint8_t c) { return c == k; });
        // There can't be less than 2 'A's if there are two green 'A's
        yellow[w[i]].min = std::max(yellow[w[i]].min, occurs);
        // If there was a strict bound on a letter and they're all guessed, remove the others
        // Ex. EERIE -> YY___ (This sets E strict bound 2); FLEES -> __GG_
        // We would update 'E' from YY___ to YY__Y since we got all the 
        if (yellow[w[i]].strict && occurs == yellow[w[i]].min) {
          for (size_t j = 0; j < green.size(); j++) {
            if (green[j] != w[i]) {
              yellow[w[i]].indices |= 1 << j;
            }
          }
        }
        break;
    }
  }

  // Deduce colors
  for (uint8_t i = 0; i < 26; i++) {
    // Ex. EERIE -> YY___, so E: XX__X 2+
    // Therefore word must have __EE__
    if (5 - yellow[i].min == std::popcount(yellow[i].indices)) {
      for (uint8_t j = 0; j < green.size(); j++) {
        if ((yellow[i].indices & (1 << j)) == 0) {
          green[j] = i;
        }
      }
    }
  }
  return true;
}

bool State::matches(const Word &w) const {
  // All greens match
  for (size_t i = 0; i < w.size(); i++) {
    if (green[i] != 0xff && green[i] != w[i]) {
      return false;
    }
  }

  // No yellows match
  for (size_t i = 0; i < w.size(); i++) {
    if (yellow[w[i]].indices & (1 << i)) {
      return false;
    }
  }

  // Each occurrance is compatible
  std::array<uint8_t, 26> occurs{};
  for (size_t i = 0; i < w.size(); i++) {
    occurs[w[i]]++;
  }
  for (uint8_t i = 0; i < 26; i++) {
    if (occurs[i] < yellow[i].min || (occurs[i] > yellow[i].min && yellow[i].strict)) {
      return false;
    }
  }
  return true;
}

std::optional<Word> State::final() const {
  if (std::any_of(green.begin(), green.end(), [](uint8_t c) { return c == 0xff; })) {
    return std::nullopt;
  }
  return green;
}

bool State::operator==(const State& s) const {
    return std::memcmp(yellow.data(), s.yellow.data(), sizeof yellow) == 0
        && green == s.green;
}

bool State::operator!=(const State& s) const {
    return !(*this == s);
}

std::ostream& State::serialize(std::ostream& out) const {
    std::array<char, sizeof(State)> buf;
    std::memcpy(buf.data(), this, buf.size());
    out.write(buf.data(), buf.size());
    return out;
}

State State::deserialize(std::istream& in) {
    std::array<char, sizeof(State)> buf;
    in.read(buf.data(), buf.size());
    State s;
    std::memcpy(&s, buf.data(), buf.size());
    return s;
}

std::ostream& wordle::operator<<(std::ostream& out, const State& s) {
  for (uint8_t i = 0; i < 26; i++) {
    if (s.yellow[i].indices == 0 && s.yellow[i].min == 0) {
      continue;
    }
    out << char('A' + i) << " ";
    for (size_t j = 0; j < 5; j++) {
      if (s.yellow[i].indices & (1 << j)) {
        out << 'X';
      } else {
        out << '_';
      }
    }
    out << " " << int(s.yellow[i].min);
    if (!s.yellow[i].strict) {
      out << "+";
    }
    out << std::endl;
  }

  out << "  " << s.green;
  return out;
}

size_t std::hash<Word>::operator()(const Word& w) const noexcept {
    size_t h = 0;
    std::hash<uint8_t> hasher;
    for (auto c : w) {
        h ^= hasher(static_cast<uint8_t>(c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

size_t std::hash<Response>::operator()(const Response& r) const noexcept {
    size_t h = 0;
    std::hash<uint8_t> hasher;
    for (auto c : r) {
        h ^= hasher(static_cast<uint8_t>(c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

size_t std::hash<State>::operator()(const State& s) const noexcept {
    std::array<char, sizeof(State)> buf;
    std::memcpy(buf.data(), &s, sizeof buf);
    size_t h = 0;
    std::hash<char> hasher;
    for (auto c : buf) {
        h ^= hasher(c) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}
