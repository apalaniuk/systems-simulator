// UnmannedSimulation.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <future>
#include <map>
#include <chrono>
#include <ctime>
#include <thread>
#include <experimental/filesystem>

#include "server.h++"
#include "SimEntity.h++"

using namespace std::string_literals;

boost::system::error_code error = boost::asio::error::host_not_found;

std::unique_ptr<std::thread> listener_thread;

int main()
{
	boost::asio::io_service ioService;

	sim::networking::server server(ioService);

	// Do Python stuff
	Py_Initialize();

	std::map< std::string, std::unique_ptr<SimEntity> > simEntities;

	// Hack in a few entities to ensure that they load their respective scripts/etc. 
	simEntities.insert(std::make_pair("contact::1"s, std::unique_ptr<SimEntity>(new SimEntity("uav::type::1"s, "contact::1"s, "../sim_entity.py"))));
	simEntities.insert(std::make_pair("contact::2"s, std::unique_ptr<SimEntity>(new SimEntity("uav::type::2"s, "contact::2"s, "../sim_entity2.py"))));
	simEntities.insert(std::make_pair("contact::3"s, std::unique_ptr<SimEntity>(new SimEntity("uav::type::1"s, "contact::3"s, "../sim_entity.py"))));
	simEntities.insert(std::make_pair("contact::4"s, std::unique_ptr<SimEntity>(new SimEntity("uav::type::2"s, "contact::4"s, "../sim_entity2.py"))));

	std::cout << "Starting.." << std::endl;

	// Run IO services in separate thread
	std::thread t([&ioService]() {
		while (true) {
			ioService.run();
		}
	});

	boost::process::child childProc("D:\\Development\\src\\SimClient\\x64\\Debug\\SimClient.exe");

	while (true) {
		for (const auto& simEntityEntry : simEntities) {
			simEntityEntry.second->updatePhysics();

			std::cout << "Ran physics step for " << simEntityEntry.first << ", a " << simEntityEntry.second->m_type << std::endl;
		}

		std::chrono::milliseconds timespan(1000);

		std::this_thread::sleep_for(timespan);
	}

    return 0;
}
