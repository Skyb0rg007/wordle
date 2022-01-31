
file(STRINGS ${INPUT_FILE} words)
list(LENGTH words num_words)

string(CONFIGURE [[
// This file is auto-generated, do not modify!
#pragma once
#include <array>
#include <wordle.hpp>

extern const std::array<wordle::Word, @num_words@> wordlist;
]] header)
file(WRITE ${OUTPUT_HEADER} "${header}")

string(CONFIGURE [[
// This file is auto-generated, do not modify!
#include <array>
#include <wordle.hpp>
#include <resources.hpp>

const std::array<wordle::Word, @num_words@> wordlist = {
]] file_header)
file(WRITE ${OUTPUT_FILE} "${file_header}")

math(EXPR last_idx "${num_words} - 1")
list(GET words ${last_idx} last_word)

foreach(word IN LISTS words)
    file(APPEND ${OUTPUT_FILE} "    wordle::Word(\"${word}\")")
    if(NOT word STREQUAL last_word)
        file(APPEND ${OUTPUT_FILE} ",")
    endif()
    file(APPEND ${OUTPUT_FILE} "\n")
endforeach()

file(APPEND ${OUTPUT_FILE} "};\n")

