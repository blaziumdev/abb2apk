#include "config.h"
#include "aab_converter.h"
#include "process_runner.h"
#include "signing.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    try {
        // Parse configuration
        aab2apk::Config config = aab2apk::ConfigParser::parse(argc, argv);

        // Initialize components
        aab2apk::ProcessRunner runner;
        aab2apk::SigningManager signer(runner);
        aab2apk::AabConverter converter(runner, signer);

        // Perform conversion
        bool success = converter.convert(config);

        if (!success) {
            std::cerr << "Conversion failed\n";
            return 1;
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    catch (...) {
        std::cerr << "Fatal error: Unknown exception\n";
        return 1;
    }
}

