//
// Test utility to check for a specified REF signal source being available to an SDR.
// Reports an error if unable to lock.
//

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <complex>
#include <thread>
#include <chrono>

namespace po = boost::program_options;

const size_t MAX_RETRY = 60;
const size_t DELAY_MS = 1000;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    std::string ref;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args [ex: addr=192.168.40.2]")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "clock reference (internal, external, mimo, gpsdo)")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Test REF Input %s") % desc << std::endl;
        std::cout
                << std::endl
                << "Tests a specified REF clock source for an SDR. Will report an error if unable to lock."
                << std::endl
                << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;


	//Lock mboard clocks
	try
	{
		//Lock mboard clocks
		usrp->set_clock_source(ref);
		std::cout << boost::format("Clock source set to: %s") % ref << std::endl;

		//Check Ref and LO Lock detect
		const size_t mboard_sensor_idx = 0;

		for (size_t i = 0; i < MAX_RETRY; i++)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

			std::vector<std::string> sensor_names = usrp->get_mboard_sensor_names(mboard_sensor_idx);

			if ((ref == "internal") and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end()))
			{
				uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked", mboard_sensor_idx);
				std::cout << boost::format("Checking Internal %s ...") % ref_locked.to_pp_string() << std::endl;

				if (!ref_locked.to_bool())
				{
					continue;
				}
			}
			if ((ref == "external") and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end()))
			{
				uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked", mboard_sensor_idx);
				std::cout << boost::format("Checking External %s ...") % ref_locked.to_pp_string() << std::endl;

				if (!ref_locked.to_bool())
				{
					continue;
				}
			}
			if ((ref == "gpsdo") and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end()))
			{
				uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked", mboard_sensor_idx);
				std::cout << boost::format("Checking GPSDO %s ...") % ref_locked.to_pp_string() << std::endl;

				if (!ref_locked.to_bool())
				{
					continue;
				}
			}
			if ((ref == "mimo") and (std::find(sensor_names.begin(), sensor_names.end(), "mimo_locked") != sensor_names.end()))
			{
				uhd::sensor_value_t mimo_locked = usrp->get_mboard_sensor("mimo_locked", mboard_sensor_idx);
				std::cout << boost::format("Checking MIMO %s ...") % mimo_locked.to_pp_string() << std::endl;

				if (!mimo_locked.to_bool())
				{
					continue;
				}
			}

			//Report Success
			std::cout << "Success!" << std::endl << std::endl;
			return EXIT_SUCCESS;
		}

		std::cout << "Failed to lock!" << std::endl << std::endl;
		return 2;

	}
	catch(std::runtime_error& e)
	{
		//Report Failure
		std::cout << "Failed to set clock source to: " << ref << std::endl;
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}

}