/* Main.cpp */

#include <boost/mpi.hpp> //include the boost mpi wrapper
#include "repast_hpc/RepastProcess.h" //Include RepastProcess header file

#include "Model.h"


int main(int argc, char** argv)
{
	std::string configFile = argv[1]; // The name of the configuration file is Argument 1
	std::string propsFile  = argv[2]; // The name of the properties file is Argument 2
	boost::mpi::communicator world; // Creates a boost mpi 'communicator' instance - world. This acts as the gateway to retrieving info about process role in greater mpi scheme.
	boost::mpi::environment env(argc, argv); // Creates a boost mpi 'environment' instance initialised with argc and argv

	repast::RepastProcess::init(configFile); // init is a static method, called from the RepastProcess class directly without the need of an instance. Repast HPC requires info regarding aspects of simulation in form of config file.

	RepastHPCModel* model = new RepastHPCModel(propsFile, argc, argv, &world); //instantiate model object from the RepastHPC model class taking properties file, main arguments and mpi communicator as arguments.
	repast::ScheduleRunner& runner = repast::RepastProcess::instance()->getScheduleRunner(); // retrieves a handle to an object that manages the timing of events.

	model->init(); //runs model init method that populates model with agents - the number of which is specified in the properties file.
	model->initSchedule(runner); //runner object handle passed to initSchedule method so that the model can schedule events

	runner.run(); // starts the processing of events on the schedule

	delete model; //model object deleted as each object instantiation must have an associated delete to prevent memory leaks as is not deleted automatically.

	repast::RepastProcess::instance()->done();

}
