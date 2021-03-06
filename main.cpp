// Copyright © 2016 Martin Ueding <dev@martin-ueding.de>

#include "hybrid-monte-carlo.hpp"
#include "sanity-checks.hpp"

#include <boost/format.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <random>

//#define OUTPUT

namespace ptree = boost::property_tree;

void abort_if_dirty() {
    if (boost::filesystem::exists("plaquette.tsv")) {
        std::cerr << "File plaquette.tsv exists, aborting. To re-run this, delete the "
                     "output files."
                  << std::endl;
        abort();
    }
}

int main() {
    abort_if_dirty();

    std::cout << "sizeof(value_type): " << sizeof(Configuration::value_type) << std::endl;

    ptree::ptree config;
    try {
        ptree::read_ini("hmc.ini", config);
    } catch (ptree::ini_parser::ini_parser_error e) {
        std::cerr << e.what() << std::endl;
        abort();
    }

    bool const do_write_config = config.get<bool>("output.links", false);
    double const beta = config.get<double>("md.beta");
    double const time_step = config.get<double>("md.time_step");
    int const chain_skip = config.get<int>("chain.skip");
    int const chain_total = config.get<int>("chain.total");
    int const length_space = config.get<int>("lattice.length_space");
    int const length_time = config.get<int>("lattice.length_time");
    int const md_steps = config.get<int>("md.steps");

    auto links = make_hot_start(length_space, length_time,
                                config.get<double>("init.hot_start_std"),
                                config.get<int>("init.seed"));


    std::ofstream ofs_accept("accept.tsv");
    std::ofstream ofs_boltzmann("boltzmann.tsv");
    std::ofstream ofs_plaquette("plaquette.tsv");
    std::ofstream ofs_plaquette_reject("plaquette-reject.tsv");

    int number_stored = 0;
    int number_computed = 0;
    int number_accepted = 0;

    boost::format config_filename_format("gauge-links-%04d.bin");

    std::mt19937 engine;
    std::normal_distribution<double> dist(0, 1);
    std::uniform_real_distribution<double> uniform(0, 1);

    while (number_computed < chain_total) {
        Configuration const old_links = links;
        double const energy_difference =
            md_evolution(links, engine, dist, time_step, md_steps, beta);
        ++number_computed;

        // Accept-Reject.
        const bool accepted =
            energy_difference <= 0 || std::exp(-energy_difference) >= uniform(engine);
        if (accepted) {
            std::cout << "Accepted.\n";
            ++number_accepted;
        } else {
            std::cout << "Rejected.\n";

            ofs_plaquette_reject << number_computed << "\t"
                                 << get_plaquette_trace_average(links).real()
                                 << std::endl;

            links = old_links;
        }

        if (do_write_config &&
            (chain_skip == 0 || number_computed % chain_skip == 0)) {
            std::string filename = (config_filename_format %
           number_stored).str();
            links.save(filename);
            ++number_stored;
        }

        auto const acceptance_rate =
            static_cast<double>(number_accepted) / number_computed;
        std::cout << "Acceptance rate: " << number_accepted << " / " << number_computed
                  << " = " << acceptance_rate << "\n"
                  << std::endl;

        double const average_plaquette = get_plaquette_trace_average(links).real();
        ofs_accept << (accepted ? 1 : 0) << std::endl;
        ofs_boltzmann << energy_difference << std::endl;
        ofs_plaquette << number_computed << "\t" << average_plaquette << std::endl;
    }
}
