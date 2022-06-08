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
  program.add_argument("level_file").help("load and display sokoban file");
  program.add_argument("-d", "--dot").help("dot file to output debug graph to");
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
    std::ifstream levelFile(program.get("level_file"));
    if (!levelFile.good()) {
      throw std::invalid_argument("bad level file: "s +
                                  program.get("level_file"));
    }
    Board board = Board::ParseFromText(levelFile);
    levelFile.close();

    // Open the graph output if present.
    std::unique_ptr<std::ofstream> graphOutput;
    if (program.present("-d")) {
      graphOutput.reset(new std::ofstream(program.get("-d")));
      if (!graphOutput->good()) {
        throw std::invalid_argument("bad graph file: "s + program.get("-d"));
      }
    }

    // Run the solver.
    auto timeStart = std::chrono::system_clock::now();
    Solver solver(board, program.get<int>("-m"));
    SolveResult result = solver.Solve(graphOutput.get());
    auto timeEnd = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> elapsed = timeEnd - timeStart;
    std::cout << "solved: " << (result.solved ? "true" : "false") << std::endl;
    std::cout << "states: " << result.statesVisited << std::endl;
    std::cout << "pushes: " << result.pushesRequired << std::endl;
    std::cout << "elapsed: " << elapsed.count() << " ms" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    std::exit(1);
  }

  return 0;
}
