#include "jobs/job_system.hpp"
#include "open4x.hpp"
#include <iostream>

using namespace std;

void empty_job(void* data) {}

int main(int argc, char* argv[]) {

    try {
        //        Open4X game;
        //       game.run();
        JobSystem job_system(16, 16);
        auto start = std::chrono::high_resolution_clock::now();
        int worker = 0;
        for (int i = 0; i < 60000; ++i) {
            job_system.workers[worker]->submit(job_system.workers[worker]->create_job(empty_job, &worker));
            worker = (worker + 1) & (16 - 1);
        }
        std::cout
            << "job creation time: "
            << std::chrono::duration<float, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() - start).count()
            << "ms" << std::endl;
        job_system.stop();
        std::cout
            << "job complete time: "
            << std::chrono::duration<float, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() - start).count()
            << "ms" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
