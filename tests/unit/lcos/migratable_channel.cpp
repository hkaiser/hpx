//  Copyright (c) 2016-2017 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_main.hpp>
#include <hpx/include/apply.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/serialization.hpp>
#include <hpx/util/lightweight_test.hpp>

#include <boost/atomic.hpp>

#include <numeric>
#include <string>
#include <utility>
#include <vector>

typedef std::string string_type;

HPX_REGISTER_MIGRATABLE_CHANNEL(int);
HPX_REGISTER_MIGRATABLE_CHANNEL(string_type);
HPX_REGISTER_MIGRATABLE_CHANNEL(void);

///////////////////////////////////////////////////////////////////////////////
void sum(std::vector<int> const& s, hpx::lcos::migratable_channel<int> c)
{
    c.set(std::accumulate(s.begin(), s.end(), 0));      // send sum to channel
}
HPX_PLAIN_ACTION(sum);

void calculate_sum(hpx::id_type const& loc)
{
    std::vector<int> s = { 7, 2, 8, -9, 4, 0 };
    hpx::lcos::migratable_channel<int> c (loc);

    hpx::apply(sum_action(), loc,
        std::vector<int>(s.begin(), s.begin() + s.size()/2), c);
    hpx::apply(sum_action(), loc,
        std::vector<int>(s.begin() + s.size()/2, s.end()), c);

    int x = c.get(hpx::launch::sync);    // receive from c
    int y = c.get(hpx::launch::sync);

    int expected = std::accumulate(s.begin(), s.end(), 0);
    HPX_TEST_EQ(expected, x + y);
}

///////////////////////////////////////////////////////////////////////////////
void ping(
    hpx::lcos::migratable_send_channel<std::string> pings,
    std::string const& msg)
{
    pings.set(msg);
}

void pong(
    hpx::lcos::migratable_receive_channel<std::string> pings,
    hpx::lcos::migratable_send_channel<std::string> pongs)
{
    std::string msg = pings.get(hpx::launch::sync);
    pongs.set(msg);
}

void pingpong(hpx::id_type const& here, hpx::id_type const& there)
{
    hpx::lcos::migratable_channel<std::string> pings(here);
    hpx::lcos::migratable_channel<std::string> pongs(there);

    ping(pings, "passed message");
    pong(pings, pongs);

    std::string result = pongs.get(hpx::launch::sync);
    HPX_TEST_EQ(std::string("passed message"), result);
}

///////////////////////////////////////////////////////////////////////////////
void ping_void(hpx::lcos::migratable_send_channel<> pings)
{
    pings.set();
}

void pong_void(
    hpx::lcos::migratable_receive_channel<> pings,
    hpx::lcos::migratable_send_channel<> pongs,
    bool& pingponged)
{
    pings.get(hpx::launch::sync);
    pongs.set();
    pingponged = true;
}

void pingpong_void(hpx::id_type const& here, hpx::id_type const& there)
{
    hpx::lcos::migratable_channel<> pings(here);
    hpx::lcos::migratable_channel<> pongs(there);

    bool pingponged = false;

    ping_void(pings);
    pong_void(pings, pongs, pingponged);

    pongs.get(hpx::launch::sync);
    HPX_TEST(pingponged);
}

///////////////////////////////////////////////////////////////////////////////
std::pair<int, bool>
dispatched_work(hpx::lcos::migratable_channel<int> jobs,
    hpx::lcos::migratable_channel<> done)
{
    int received_jobs = 0;
    bool was_closed = false;

    while(true)
    {
        hpx::error_code ec(hpx::lightweight);
        int job = jobs.get(hpx::launch::sync, ec);
        (void)job;

        if (!ec)
        {
            ++received_jobs;
        }
        else
        {
            was_closed = true;
            done.set();
            break;
        }
    }

    return std::make_pair(received_jobs, was_closed);
}
HPX_PLAIN_ACTION(dispatched_work);

void dispatch_work(hpx::id_type const& loc)
{
    hpx::lcos::migratable_channel<int> jobs(loc);
    hpx::lcos::migratable_channel<> done(loc);

    hpx::future<std::pair<int, bool> > f =
        hpx::async(dispatched_work_action(), loc, jobs, done);

    for (int j = 1; j <= 3; ++j)
    {
        jobs.set(j);
    }

    jobs.close();
    done.get(hpx::launch::sync);

    auto p = f.get();

    HPX_TEST_EQ(p.first, 3);
    HPX_TEST(p.second);
}

///////////////////////////////////////////////////////////////////////////////
void channel_range(hpx::id_type const& loc)
{
    boost::atomic<int> received_elements(0);

    hpx::lcos::migratable_channel<std::string> queue(loc);
    queue.set("one");
    queue.set("two");
    queue.set("three");
    queue.close();

    for (auto const& elem : queue)
    {
        (void)elem;
        ++received_elements;
    }

    HPX_TEST_EQ(received_elements.load(), 3);
}

void channel_range_void(hpx::id_type const& loc)
{
    boost::atomic<int> received_elements(0);

    hpx::lcos::migratable_channel<> queue(loc);
    queue.set();
    queue.set();
    queue.set();
    queue.close();

    for (auto const& elem : queue)
    {
        (void)elem;
        ++received_elements;
    }

    HPX_TEST_EQ(received_elements.load(), 3);
}

///////////////////////////////////////////////////////////////////////////////
void closed_channel_get(hpx::id_type const& loc)
{
    bool caught_exception = false;
    try {
        hpx::lcos::migratable_channel<int> c(loc);
        c.close();

        int value = c.get(hpx::launch::sync);
        (void)value;
        HPX_TEST(false);
    }
    catch(hpx::exception const&) {
        caught_exception = true;
    }
    HPX_TEST(caught_exception);
}

void closed_channel_get_generation(hpx::id_type const& loc)
{
    bool caught_exception = false;
    try {
        hpx::lcos::migratable_channel<int> c(loc);
        c.set(42, 122);         // setting value for generation 122
        c.close();

        HPX_TEST_EQ(c.get(hpx::launch::sync, 122), 42);

        int value = c.get(hpx::launch::sync, 123); // asking for generation 123
        HPX_TEST(false);
        (void)value;
    }
    catch(hpx::exception const&) {
        caught_exception = true;
    }
    HPX_TEST(caught_exception);
}

void closed_channel_set(hpx::id_type const& loc)
{
    bool caught_exception = false;
    try {
        hpx::lcos::migratable_channel<int> c(loc);
        c.close();

        c.set(42);
        HPX_TEST(false);
    }
    catch(hpx::exception const&) {
        caught_exception = true;
    }
    HPX_TEST(caught_exception);
}

///////////////////////////////////////////////////////////////////////////////
int return42()
{
    return 42;
}
HPX_PLAIN_ACTION(return42);

void channel_as_lco(hpx::id_type const& here, hpx::id_type const& there)
{
    hpx::lcos::migratable_channel<int> lco(here);

    hpx::apply_c(return42_action(), lco.get_id(), there);

    HPX_TEST_EQ(lco.get(hpx::launch::sync), 42);
}

///////////////////////////////////////////////////////////////////////////////
void trigger_send(hpx::lcos::migratable_send_channel<hpx::id_type> c)
{
    c.set(hpx::launch::sync, hpx::find_here());
}
HPX_PLAIN_ACTION(trigger_send);

// void test_migrate_channel(hpx::id_type source, hpx::id_type target)
// {
//     // create channel on given locality
//     hpx::lcos::migratable_channel<hpx::id_type> t1(source);
//     HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());
//
//     // get to receive end
//     hpx::lcos::migratable_receive_channel<hpx::id_type> rc(t1);
//
//     // make sure this channel was created on source
//     hpx::future<void> f1 = hpx::async<trigger_send_action>(source, t1);
//     HPX_TEST_EQ(rc.get(hpx::launch::sync), source);
//     f1.get();
//
//     // migrate t1 to the target
//     hpx::lcos::migratable_channel<hpx::id_type> t2(
//         hpx::components::migrate(t1, target));
//
//     // wait for migration to be done
//     HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());
//
//     // the migrated object should have the same id as before
//     HPX_TEST_EQ(t1.get_id(), t2.get_id());
//
//     // tell the migrated object
//     hpx::future<void> f2 = hpx::async<trigger_send_action>(target, t1);
//     HPX_TEST_EQ(rc.get(hpx::launch::sync), target);
//     f2.get();
// }


///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    hpx::id_type here = hpx::find_here();

    calculate_sum(here);
    pingpong(here, here);
    pingpong_void(here, here);
    dispatch_work(here);
    channel_range(here);
    channel_range_void(here);

    closed_channel_get(here);
    closed_channel_get_generation(here);
    closed_channel_set(here);

    channel_as_lco(here, here);

    std::vector<hpx::id_type> remote_localities = hpx::find_remote_localities();
    for (hpx::id_type id : remote_localities)
    {
        pingpong(id, here);
        pingpong(here, id);
        pingpong_void(id, here);
        pingpong_void(here, id);

        channel_as_lco(id, here);
        channel_as_lco(here, id);

//         test_migrate_channel(here, id);
//         test_migrate_channel(id, here);
    }

    return hpx::util::report_errors();
}
