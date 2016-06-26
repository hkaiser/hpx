//  Copyright (c) 2016 Satyaki Upadhyay
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_SUDOKU_EXAMPLE_SERVER)
#define HPX_SUDOKU_EXAMPLE_SERVER

#include <hpx/include/async.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/include/util.hpp>

#include <boost/cstdint.hpp>

namespace sudoku {
class cancellation_token
{
public:
    bool cancel;
    cancellation_token()
    {
        cancel = false;
    }

    friend class hpx::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& cancel;
    }
};

typedef std::vector<boost::uint8_t> board_type;

namespace server {
    class HPX_COMPONENT_EXPORT board
        : public hpx::components::component_base<board>
    {
    private:
        board_type board_config;
        std::size_t level_;
        std::size_t size_;
        std::size_t count_;

    public:
        board()
          : board_config(0)
          , level_(0)
          , size_(0)
        {
        }

        bool check_board(std::size_t level, boost::uint8_t value);

        board_type access_board();

        void update_board(std::size_t pos, boost::uint8_t val);

        board_type solve_board(board_type const& board);

        HPX_DEFINE_COMPONENT_ACTION(board, access_board, access_action);
        HPX_DEFINE_COMPONENT_ACTION(board, update_board, update_action);
        HPX_DEFINE_COMPONENT_ACTION(board, check_board, check_action);
        HPX_DEFINE_COMPONENT_ACTION(board, solve_board, solve_action);
    };
}
}

// Declaration of serialization support for the board actions
typedef sudoku::server::board component_type;
HPX_REGISTER_ACTION_DECLARATION(component_type::init_action, board_init_action);
HPX_REGISTER_ACTION_DECLARATION(
    component_type::check_action, board_check_action);
HPX_REGISTER_ACTION_DECLARATION(
    component_type::access_action, board_access_action);
HPX_REGISTER_ACTION_DECLARATION(
    component_type::update_action, board_update_action);
HPX_REGISTER_ACTION_DECLARATION(
    component_type::solve_action, board_solve_action);

#endif    // HPX_SUDOKU_EXAMPLE_SERVER
