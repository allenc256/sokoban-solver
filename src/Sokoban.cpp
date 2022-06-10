#include <stdio.h>

#include <argparse/argparse.hpp>
#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Board.h"
#include "Solver.h"

using namespace std::string_literals;

int main(int argc, char *argv[]) {
  // Parse arguments.
  argparse::ArgumentParser program("Sokoban");
  program.add_argument("level_file").help("sokoban level file");
  program.add_argument("-d", "--debug").help("debug log file");
  program.add_argument("-t", "--tabular")
      .help("tabular output")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-m", "--max-states")
      .help("maximum number of states to limit search to")
      .default_value(1000000)
      .scan<'i', int>();
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  try {
    // Load the board.
    std::string levelFileName = program.get("level_file");
    std::ifstream levelFile(levelFileName);
    if (!levelFile.good()) {
      throw std::invalid_argument("bad level file: "s + levelFileName);
    }
    Board board = Board::ParseFromText(levelFile);
    levelFile.close();

    // Open the graph output if present.
    std::unique_ptr<std::ofstream> debugFile;
    if (program.present("-d")) {
      debugFile.reset(new std::ofstream(program.get("-d")));
      if (!debugFile->good()) {
        throw std::invalid_argument("bad debug file: "s + program.get("-d"));
      }
    }

    // Run the solver.
    auto timeStart = std::chrono::system_clock::now();
    Solver solver(board, program.get<int>("-m"));
    SolveResult result = solver.Solve(debugFile.get());
    auto timeEnd = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> elapsed = timeEnd - timeStart;

    // Output results.
    if (program["-t"] == true) {
      std::cout << levelFileName << '\t';
      std::cout << (result.solved ? "true" : "false") << '\t';
      std::cout << result.statesVisited << '\t';
      std::cout << result.pushesRequired << '\t';
      std::cout << elapsed.count() << " ms" << std::endl;
    } else {
      std::cout << "solved: " << (result.solved ? "true" : "false")
                << std::endl;
      std::cout << "states: " << result.statesVisited << std::endl;
      std::cout << "pushes: " << result.pushesRequired << std::endl;
      std::cout << "elapsed: " << elapsed.count() << " ms" << std::endl;
    }

  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    std::exit(1);
  }

  return 0;
}
