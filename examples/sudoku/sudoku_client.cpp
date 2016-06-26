//  Copyright (c) 2016 Satyaki Upadhyay
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/include/util.hpp>

#include <boost/cstdint.hpp>

#include "sudoku.hpp"

#define SUDOKU_BOARD_SIZE_DIM (9)
#define SUDOKU_BOARD_SIZE (SUDOKU_BOARD_SIZE_DIM * SUDOKU_BOARD_SIZE_DIM)

std::vector<boost::uint8_t> init_board(std::size_t dim)
{
    // initialize the starting state of the puzzle
    // 0 denotes empty positions
    // non-zero values denote already-filled cells

    std::vector<boost::uint8_t> board_config(dim * dim);

    hpx::cout << "Press 1 to enter the starting state of the board, "
              << "or 2 to use the default" << hpx::endl;

    int choice;
    std::cin >> choice;

    if (choice == 1)
    {
        hpx::cout << "Please enter the initial state of the board as a " << dim
                  << " x " << dim << " matrix using 0 to denote empty cells."
                  << hpx::endl;

        for (std::size_t r = 0; r != dim; ++r)
        {
            for (std::size_t c = 0; c != dim; ++c)
            {
                int a;
                std::cin >> a;
                board_config[dim * r + c] = a;
            }
        }
    }
    else
    {
        board_config.at(1) = 2;
        board_config.at(5) = 4;
        board_config.at(6) = 3;
        board_config.at(9) = 9;
        board_config.at(13) = 2;
        board_config.at(17) = 8;
        board_config.at(21) = 6;
        board_config.at(23) = 9;
        board_config.at(25) = 5;
        board_config.at(35) = 1;
        board_config.at(37) = 7;
        board_config.at(38) = 2;
        board_config.at(39) = 5;
        board_config.at(41) = 3;
        board_config.at(42) = 6;
        board_config.at(43) = 8;
        board_config.at(45) = 6;
        board_config.at(55) = 8;
        board_config.at(57) = 2;
        board_config.at(59) = 5;
        board_config.at(63) = 1;
        board_config.at(67) = 9;
        board_config.at(71) = 3;
        board_config.at(74) = 9;
        board_config.at(75) = 8;
        board_config.at(79) = 6;
    }
}

void print_board(std::size_t dim, std::string caption,
    std::vector<boost::uint8_t> const& board)
{
    hpx::cout << caption << hpx::endl;
    for (std::size_t r = 0; r != dim; ++r)
    {
        for (std::size_t c = 0; c != dim; ++c)
        {
            if (board[r * dim + c] == 0)
            {
                hpx::cout << "_";
            }
            else
            {
                hpx::cout << unsigned(board[r * dim + c]);
            }
            hpx::cout << " ";
        }
        hpx::cout << hpx::endl;
    }
    hpx::cout << hpx::endl;
}

int hpx_main(boost::program_options::variables_map&)
{
    const std::size_t default_size = SUDOKU_BOARD_SIZE;
    hpx::naming::id_type locality = hpx::find_here();

    sudoku::board new_board = hpx::new_<sudoku::board>(locality);

    std::vector<boost::uint8_t> board_config = init_board(SUDOKU_BOARD_SIZE);

    print_board(SUDOKU_BOARD_SIZE_DIM, "Initial state of the board:", board_config);

    // check whether the puzzle was solved
    std::vector<boost::uint8_t> final_board = new_board.solve_board(board_config);

    bool no_solution = false;
    for (std::size_t r = 0; r != SUDOKU_BOARD_SIZE_DIM; ++r)
    {
        for (std::size_t c = 0; c != SUDOKU_BOARD_SIZE_DIM; ++c)
        {
            if (final_board[SUDOKU_BOARD_SIZE_DIM * r + c] == 0)
            {
                no_solution = true;
                break;
            }
        }
    }

    if (no_solution)
    {
        hpx::cout << "The given sudoku puzzle has no solution" << hpx::endl;
    }
    else
    {
        print_board(SUDOKU_BOARD_SIZE_DIM, "Completed puzzle:", board_config);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    boost::program_options::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    return hpx::init(desc_commandline, argc, argv);
}
