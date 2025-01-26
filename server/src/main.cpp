#include <iostream>
#include "networking/server_networking/network.hpp"
#include "utility/fixed_frequency_loop/fixed_frequency_loop.hpp"
#include "utility/fixed_frequency_processor/fixed_frequency_processor.hpp"

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
    Network network(7777, sinks);
    network.initialize_network();

    int test_client_id = -1;

    std::function<void(unsigned int)> on_client_connect = [&](unsigned int client_id) {
        /*physics.create_character(client_id);*/
        /*connected_character_data[client_id] = {client_id};*/
        spdlog::info("just registered a client with id {}", client_id);
        // TODO no need to broadcast this to everyone, only used once on the connecting client
        /*network.reliable_send(client_id, &client_id, sizeof(unsigned int));*/
        test_client_id = client_id;
    };

    network.set_on_connect_callback(on_client_connect);

    double position = 0;
    double velocity = 1;

    std::function<void(int, double)> physics_tick = [&](int id, double dt) {
        position += velocity * dt;
        std::cout << "processing id: " << id << " with dt: " << dt << " new position: " << position << std::endl;
    };

    FixedFrequencyProcessor physics_engine(60, physics_tick);
    PeriodicSignal send_signal(20);

    std::function<void(double)> tick = [&](double dt) {
        std::vector<PacketWithSize> received_packets = network.get_network_events_since_last_tick();
        for (const auto &packet : received_packets) {
            const int *received_id = reinterpret_cast<const int *>(packet.data.data());
            std::cout << "just got id: " << *received_id << std::endl;
            physics_engine.add_id(*received_id);
        }

        if (physics_engine.attempt_to_process()) {
            std::cout << "^^^ just processed ^^^" << std::endl;
        }

        if (test_client_id != -1) {
            if (physics_engine.processed_at_least_one_id and send_signal.process_and_get_signal()) {
                GameUpdate gu(position, physics_engine.get_last_processed_id());
                std::cout << "SENDING PACKET: " << gu << std::endl;
                network.unreliable_send(test_client_id, &gu, sizeof(GameUpdate));
            }
        }
    };
    std::function<bool()> termination = [&]() { return false; };
    FixedFrequencyLoop ffl;
    ffl.start(240, tick, termination);
}
