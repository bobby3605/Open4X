#include "job.hpp"

void Job::execute() {
    if (function) {
        function(data);
    }
    finish();
}

void Job::finish() {
    if ((--unfinished_dependencies == 0) && parent) {
        parent->finish();
    }
}
