#include <algorithm>
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <cs19_wordle.h>

// Create the word set at the start of the program
// Create a backup for quick recreation of the set after each game
void createWordSet(std::unordered_set<std::string>& possibleWords,
      std::unordered_set<std::string>& possibleWordsBackup) {
  {
    std::ifstream words("/srv/datasets/wordle_words.txt");
    for (std::string word; std::getline(words, word);) {
      possibleWords.insert(word);
      possibleWordsBackup.insert(word);
    }
  }
}

// Find how common each letter is in the entire set of words
std::map<char, int> letterFrequencies(std::unordered_set<std::string> possibleWords) {
  std::map<char, int> letterFreqs;
  for (char c = 65; c < 91; c++) {
    letterFreqs[c] = 0;
  }
  for (std::string word : possibleWords) {
    for (char c = 65; c < 91; c++) {
      if (word.find(c) != std::string::npos) {
        letterFreqs[c]++;
      }
    }
  }
  return letterFreqs;
}

// This first helper function allows us to check if a word has all of the letters that we know to
// be in the word, but whose location is unknown
bool hasAllYellows(std::string word, std::unordered_set<char> yellows) {
  if (yellows.empty()) {
    return false;
  }
  for (char yellow : yellows) {
    if (word.find(yellow) == std::string::npos) {
      return false;
    }
  }
  return true;
}

// A function that checks if all of the letters in a word are different
// This makes sure we maximize the value of our guesses
bool hasUniqueLetters(std::string word) {
  std::unordered_set<char> letters;
  for (char letter : word) {
    letters.insert(letter);
  }
  return (letters.size() == 5);
}

// Rank each word by the summed frequency of its letter
std::map<std::string, int> wordRankings(std::unordered_set<std::string> candidates,
      std::map<char, int> letterFreqs) {
  std::map<std::string, int> wordRanks;
  for (std::string candidate : candidates) {
    wordRanks[candidate] = 0;
    for (char letter : candidate) {
      wordRanks[candidate] += letterFreqs[letter];
    }
  }
  return wordRanks;
}

// The best candidate has the greatest summed letter frequency
std::string bestCandidate(std::unordered_set<std::string> candidates,
      std::map<std::string, int> wordRanks) {
  int max = 0;
  std::string bestCandidate = "";
  for (std::string word : candidates) {
    if (wordRanks[word] > max) {
      max = wordRanks[word];
      bestCandidate = word;
    }
  }
  return bestCandidate;
}

// Uses hasAllYellows() and hasUniqueLetters() to find a good candidate word
// A good candidate contains letters that are all different, and are all known to be in the word
// Then, check all the candidates against each other to find which one has the most common letters
std::string findBestCandidate(std::unordered_set<std::string> possibleWords,
      std::unordered_set<char> yellows, std::map<char, int> letterFreqs) {
  std::unordered_set<std::string> candidates;
  for (std::string word : possibleWords) {
    if (hasUniqueLetters(word) && hasAllYellows(word, yellows)) {
      candidates.insert(word);
    }
  }
  // Check the list of candidates to see which one has the highest rank
  if (!candidates.empty()) {
    std::map<std::string, int> wordRanks = wordRankings(candidates, letterFreqs);
    return bestCandidate(candidates, wordRanks);
  }
  // Sometimes, the best we can have is only one of these conditions
  for (std::string word : possibleWords) {
    if (hasAllYellows(word, yellows)) {
      candidates.insert(word);
    }
  }
  if (!candidates.empty()) {
    std::map<std::string, int> wordRanks = wordRankings(candidates, letterFreqs);
    return bestCandidate(candidates, wordRanks);
  }
  for (std::string word : possibleWords) {
    if (hasUniqueLetters(word)) {
      candidates.insert(word);
    }
  }
  if (!candidates.empty()) {
    std::map<std::string, int> wordRanks = wordRankings(candidates, letterFreqs);
    return bestCandidate(candidates, wordRanks);
  }
  // We still have to return something if the for loop does not
  std::map<std::string, int> wordRanks = wordRankings(possibleWords, letterFreqs);
  return bestCandidate(possibleWords, wordRanks);
}

int main() {
  // A mapping of `cs19_wordle::letter_status_t` keys to ANSI escape code values:
  std::map<cs19_wordle::letter_status_t, const std::string> ansiColors{
      {cs19_wordle::GREEN, "\033[1;30;42m"},
      {cs19_wordle::YELLOW, "\033[1;30;43m"},
      {cs19_wordle::GRAY, "\033[1;30;243m"},
      {cs19_wordle::ERROR, "\033[1;30;41m"}};
  cs19_wordle::Wordle game;  // get an instance of our playable Wordle class
  std::cout << "CS 19 Wordle Demo!\n";

  // Instantiate all of our variables
  std::unordered_set<std::string> possibleWords;
  std::unordered_set<std::string> possibleWordsBackup;
  createWordSet(possibleWords, possibleWordsBackup);
  std::map<char, int> letterFreqs = letterFrequencies(possibleWords);
  std::unordered_map<char, std::unordered_set<int>> greens;
  std::unordered_set<char> yellows;
  int numGuesses = 0;
  std::string userGuess;

  while (true) {
    // Using four known words narrows down the list quickly
    // These words use 20 letters in four guesses, giving us lots of information
    // This also speeds up the program (less decisions have to be made)
    if (numGuesses == 0) {
      userGuess = "FADES";
    } else if (numGuesses == 1) {
      userGuess = "BROWN";
    } else if (numGuesses == 2) {
      userGuess = "MIGHT";
    } else if (numGuesses == 3) {
      userGuess = "PLUCK";
    } else {
      userGuess = findBestCandidate(possibleWords, yellows, letterFreqs);
    }
    int previous_games = game.total_games();
    auto result = game.guess(userGuess);

    // Clear the set of yellows
    // We only want to do this after we have used our first three words; otherwise, we lose
    // valuable information
    // Yellows may end up being "converted" to greens, or added back to yellows if they are not
    // in the right place; either way, not removing them can cause issues
    if (numGuesses > 3) {
      yellows.erase(yellows.begin(), yellows.end());
    }

    // Here, we look ahead to see if there are any greens or yellows
    // This helps in certain scenarios; imagine the real word is "SCENT", and our guess is "SEEDY";
    // In this case, the first "E" would come up grey, and the second "E" would be green
    // With how the code was previously written, this scenario would cause any word with "E" to be
    // deleted before we find out "E" is actually in the word
    for (int i = 0; i < 5; ++i) {
      if (result[i] == cs19_wordle::GREEN) {
        greens[userGuess[i]].insert(i);
      } else if (result[i] == cs19_wordle::YELLOW) {
        yellows.insert(userGuess[i]);
      }
    }

    if (game.total_games() == previous_games) {
      for (int i = 0; i < 5; ++i){
        std::cout << ansiColors[result[i]] << userGuess[i] << "\033[0m";

        // If our letter is in the right place, remove all words that do NOT have that letter
        // in the same place
        if (result[i] == cs19_wordle::GREEN) {
          greens[userGuess[i]].insert(i);
          std::erase_if(possibleWords, [&](std::string word) {
            return static_cast<int>(word.find(userGuess[i], i)) != i; });

        // If our letter is in the wrong place, add it to our set of yellows, but remove all words
        // with the letter in the known wrong place
        } else if (result[i] == cs19_wordle::YELLOW) {
          std::erase_if(possibleWords, [&](std::string word) {
            return (word[i] == userGuess[i] || word.find(userGuess[i]) == std::string::npos); });

        // If our letter is definitely not anywhere else in the word, remove any word that contains
        // that letter anywhere
        } else if (result[i] == cs19_wordle::GRAY && !greens.contains(userGuess[i]) &&
              !yellows.contains(userGuess[i])) {
          std::erase_if(possibleWords, [&](std::string word) {
            return word.find(userGuess[i]) != std::string::npos; });

        // If our letter already has a place in the word, but cannot be anywhere else in the word,
        // remove any word that has that letter in the wrong place
        } else if (result[i] == cs19_wordle::GRAY) {
          // Only remove words if we know for certain that letter is not in the word
          if (!yellows.contains(userGuess[i])) {
            std::erase_if(possibleWords, [&](std::string word) {
              return static_cast<int>(!greens[userGuess[i]].contains(word.find(userGuess[i]))); });
            }
        }
      }
      possibleWords.erase(userGuess);
      numGuesses++;
    } else {
      std::cout << "(new game—word was \"" << game.previous_word() << "\")";
      possibleWords = possibleWordsBackup;
      yellows.erase(yellows.begin(), yellows.end());
      numGuesses = 0;
      greens.clear();
    }
    std::cout << " — win rate " << game.wins() << '/' << game.total_games() << '\n';
  }
}
