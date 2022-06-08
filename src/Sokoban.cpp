#include <stdio.h>

#include <argparse/argparse.hpp>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Board.h"
#include "Solver.h"

int main(int argc, char *argv[]) {
  // Parse arguments.
  argparse::ArgumentParser program("Sokoban");
  program.add_argument("level_file").help("load and display sokoban file");
  program.add_argument("-d", "--dot").help("dot file to output debug graph to");
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  // Load the board.
  std::ifstream levelFile(program.get("level_file"));
  Board board = Board::ParseFromText(levelFile);
  levelFile.close();

  // Open the graph output if present.
  std::unique_ptr<std::ofstream> graphOutput;
  if (program.present("-d")) {
    graphOutput.reset(new std::ofstream(program.get("-d")));
  }

  // Run the solver.
  Solver solver(board);
  int statesVisited = solver.Solve(graphOutput.get());
  std::cout << "states: " << statesVisited << std::endl;

  return 0;
}
