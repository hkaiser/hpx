//  Copyright (c) 2016 Satyaki Upadhyay
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>

#include "server/sudoku.hpp"

namespace sudoku {
typedef std::vector<boost::uint8_t> board_type;

namespace server {
    bool board::check_board(std::size_t level, boost::uint8_t value)
    {
        std::size_t x = level / 9, y = level % 9;

        // no equal value in same row
        for (std::size_t col = 0; col < 9; col++)
        {
            if ((x * 9 + col) != unsigned(level) &&
                board_config.at(x * 9 + col) == value)
            {
                return false;
            }
        }

        // no equal value in same column
        for (std::size_t row = 0; row < 9; row++)
        {
            if ((row * 9 + y) != unsigned(level) &&
                board_config.at(row * 9 + y) == value)
            {
                return false;
            }
        }

        // no equal value in same mini-grid
        std::size_t minigrid_row = x / 3, minigrid_col = y / 3;
        for (std::size_t r = 3 * minigrid_row; r < 3 * (minigrid_row + 1); r++)
        {
            for (std::size_t c = 3 * minigrid_col; c < 3 * (minigrid_col + 1);
                 c++)
            {
                if ((r * 9 + c) != unsigned(level) &&
                    board_config.at(r * 9 + c) == value)
                {
                    return false;
                }
            }
        }

        return true;
    }

    board_type board::access_board()
    {
        return board_config;
    }

    void board::update_board(std::size_t pos, boost::uint8_t val)
    {
        board_config.at(pos) = val;
    }

    board_type board::solve_board_level(board_type const& board)
    {
    }

    board_type board::solve_board_level(
        std::size_t size, std::size_t level, cancellation_token ct)
    {
        if (level == size)
        {
            return board_config;
        }

        if (board_config[level] != 0)
        {
            return hpx::async(&board::solve_board_level, size, level + 1, ct)
                .get();
        }

        Container futures;
        cancellation_token next_ct;
        for (boost::uint8_t i = 0; i <= 9; ++i)
        {
            // try to assign value i at this position
            if (check_board(level, i))
            {
                update_board(level, i);
                future<board_type> f;
                f = hpx::async<action_type>(
                    this->get_id(), size, level + 1, next_ct);
                futures.push_back(std::move(f));
                if (ct.cancel)
                    hpx::this_thread::interrupt();
            }
        }

        // wait for the futures to become ready
        // when any of them becomes ready, if it holds a valid board config,
        // return it as the answer, else the board is unsolvable

        board_type ans(size, unsigned(0));
        while (!futures.empty())
        {
            hpx::when_any_result<Container> raw = hpx::when_any(futures).get();
            futures = std::move(raw.futures);
            std::size_t index = raw.index;

            board_type b = futures.at(index).get();
            futures.erase(futures.begin() + index);

            if (ct.cancel)
                hpx::this_thread::interrupt();

            bool reached_solution = true;
            for (std::size_t r = 0; r < 9 && reached_solution; r++)
            {
                for (std::size_t c = 0; c < 9; c++)
                    if (b.at(9 * r + c) == 0)
                    {
                        reached_solution = false;
                        break;
                    }
            }

            if (reached_solution)
            {
                ans = b;
                next_ct.cancel = true;
                break;
            }

            if (ct.cancel)
                hpx::this_thread::interrupt();
        }

        return ans;
    }
}
}

HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<sudoku::server::board> board_type;
HPX_REGISTER_COMPONENT(board_type, board);

// Serialization support for the board actions
typedef sudoku::server::board component_type;
HPX_REGISTER_ACTION(component_type::init_action, board_init_action);
HPX_REGISTER_ACTION(component_type::check_action, board_check_action);
HPX_REGISTER_ACTION(component_type::access_action, board_access_action);
HPX_REGISTER_ACTION(component_type::update_action, board_update_action);
HPX_REGISTER_ACTION(component_type::solve_action, board_solve_action);
