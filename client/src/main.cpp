/*#include <iostream>*/
/*#include <vector>*/
/*#include <chrono>*/
/*#include <functional>*/
/*#include <thread>*/
/*#include "utility/fixed_frequency_processor/fixed_frequency_processor.hpp"*/
/*#include "utility/rate_limited_function/rate_limited_function.hpp"*/
/*#include "utility/periodic_signal/periodic_signal.hpp"*/

#include "networking/client_networking/network.hpp"

#include "utility/fixed_frequency_loop/fixed_frequency_loop.hpp"
#include "utility/fixed_frequency_reprocessor/fixed_frequency_reprocessor.hpp"
#include "utility/periodic_signal/periodic_signal.hpp"
#include "utility/rate_limited_function/rate_limited_function.hpp"

#include <iostream>

struct GameUpdate {
    double position;
    int last_id_used_to_produce_this_update;

    // Overloading the << operator
    friend std::ostream &operator<<(std::ostream &os, const GameUpdate &update) {
        os << "GameUpdate { position: " << update.position
           << ", last_id_used_to_produce_this_update: " << update.last_id_used_to_produce_this_update << " }";
        return os;
    }
};

int main() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("network_logs.txt", true);
    file_sink->set_level(spdlog::level::info);

    std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};
    Network network("localhost", 7777, sinks);
    network.initialize_network();
    network.attempt_to_connect_to_server();

    std::unordered_map<int, double> id_to_position;

    double client_position = 0;
    double velocity = 1;

    auto client_process = [&](int id, double dt) {
        client_position += velocity * dt;
        id_to_position[id] = client_position;
        std::cout << "Client Processing ID: " << id << " with dt: " << dt << " - New Position: " << id_to_position[id]
                  << std::endl;
    };

    auto reprocess_function = [&](int id) {
        /*if (id_to_position.find(id) != id_to_position.end()) {*/
        /*    client_position = id_to_position[id];*/
        /*    std::cout << "Client Reprocessing ID: " << id << " - Position set to " << client_position << std::endl;*/
        /*}*/
    };

    int curr_id = 0;

    std::vector<int> ids_since_last_sever_send;

    FixedFrequencyReprocessor client_physics(60, client_process, reprocess_function);
    PeriodicSignal send_signal(60);

    int iters_before_termination = 100;
    std::function<bool()> termination = [&]() { return iters_before_termination == 0; };
    std::function<void(double)> tick = [&](double dt) {
        std::cout << "=== TICK START ===" << std::endl;
        client_physics.add_id(curr_id);
        ids_since_last_sever_send.push_back(curr_id);

        if (client_physics.attempt_to_process()) {
            std::cout << "^^^ just processed ^^^" << std::endl;
        }

        if (send_signal.process_and_get_signal()) {
            for (const auto &id : ids_since_last_sever_send) {
                std::cout << "SENDING PACKET: " << curr_id << std::endl;
                network.send_packet(&id, sizeof(int));
            }
            ids_since_last_sever_send.clear();
        }

        std::vector<GameUpdate> game_updates_this_tick;
        std::vector<PacketWithSize> packets = network.get_network_events_received_since_last_tick();

        if (packets.size() >= 1) {
            std::cout << "=== ITERATING OVER NEW PACKETS START ===" << std::endl;
            for (PacketWithSize pws : packets) {
                GameUpdate game_update;
                std::memcpy(&game_update, pws.data.data(), sizeof(GameUpdate));
                game_updates_this_tick.push_back(game_update);
            }
            std::cout << "=== ITERATING OVER NEW PACKETS END ===" << std::endl;
        }

        if (game_updates_this_tick.size() >= 1) {
            std::cout << "=== RECEIVED GAME UPDATE AND NOW RECONCILING START ===" << std::endl;
            GameUpdate last_received_game_update = game_updates_this_tick.back();

            std::cout << "position was: " << client_position << " now setting position to "
                      << last_received_game_update.position << std::endl;
            double predicted_position = client_position;
            // slam it in
            client_position = last_received_game_update.position;
            // reconcile
            client_physics.re_process_after_id(last_received_game_update.last_id_used_to_produce_this_update);

            std::cout << "position before reconciliation was: " << predicted_position << " after reconciliation was "
                      << client_position << std::endl;
            std::cout << "the delta (predicted - reconciled): " << predicted_position - client_position << std::endl;
            std::cout << "=== RECEIVED GAME UPDATE AND NOW RECONCILING END === " << std::endl;
        }

        curr_id++;
        std::cout << "=== TICK END ===" << std::endl;
    };

    FixedFrequencyLoop ffl;
    ffl.start(240, tick, termination);

    return 0;
    /**/
    /*FixedFrequencyReprocessor client(4, client_process, reprocess_function);*/
    /**/
    /*PeriodicSignal client_send_signal(2);*/
    /*std::vector<int> client_processed_ids_since_last_server_process;*/
    /**/
    /*PeriodicSignal server_send_signal(2);*/
    /**/
    /*double simulated_time = 0.0;*/
    /*const double delta_time = 0.1;*/
    /*int curr_id = 0;*/
    /**/
    /*bool server_just_processed = false;*/
    /**/
    /*while (true) {*/
    /*    id_to_delta_time[curr_id] = delta_time;*/
    /**/
    /*    client.add_id(curr_id);*/
    /*    client.attempt_to_process(delta_time);*/
    /**/
    /*    if (client_send_signal.process_and_get_signal()) {*/
    /*        std::cout << "sending to server start" << std::endl;*/
    /*        if (server.processed_at_least_one_id) {*/
    /*            server.add_ids(client.get_ids_processed_after(server.get_last_processed_id()));*/
    /*        } else {*/
    /*            server.add_ids(client.processed_ids);*/
    /*        }*/
    /*        std::cout << "sending to server end" << std::endl;*/
    /*    }*/
    /**/
    /*    server_just_processed = server.attempt_to_process(delta_time);*/
    /**/
    /*    if (server_send_signal.process_and_get_signal()) {*/
    /*        if (server.processed_at_least_one_id) {*/
    /*            std::cout << "client receiving server id and reprocessing start" << std::endl;*/
    /*            double before_pos = client_position;*/
    /*            client.re_process_after_id(server.get_last_processed_id());*/
    /*            double after_pos = client_position;*/
    /*            std::cout << "position before reconciliation: " << before_pos;*/
    /*            std::cout << " position after reconciliation: " << after_pos << std::endl;*/
    /*            if (after_pos - before_pos) {*/
    /*                std::cout << "WARNING DELTA DETECTED" << client_position << std::endl;*/
    /*            }*/
    /*        }*/
    /*    }*/
    /**/
    /*    ++curr_id;*/
    /**/
    /*    simulated_time += delta_time;*/
    /**/
    /*    if (simulated_time > 5.0) {*/
    /*        break;*/
    /*    }*/
    /**/
    /*    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate frame delay.*/
    /*}*/
    /**/
    /*// Display final positions of all IDs*/
    /*std::cout << "Final Positions of IDs:\n";*/
    /*for (const auto &[id, position] : id_to_position) {*/
    /*    std::cout << "ID: " << id << ", Position: " << position << std::endl;*/
    /*}*/
    /**/
    /*return 0;*/
}
