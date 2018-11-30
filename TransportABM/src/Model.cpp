/* Model.cpp */

#include <stdio.h>// include standard c input output library
#include <vector> // includes vector header file so can be used to store agents. Enables easy agent iteration
#include <boost/mpi.hpp> //include boost mpi wrapper
#include "repast_hpc/AgentId.h"
#include "repast_hpc/RepastProcess.h"
#include "repast_hpc/Utilities.h" // Provides string to int function for properties files
#include "repast_hpc/Properties.h" // Enables use of the properties class so that props file can be used to load data
#include "repast_hpc/initialize_random.h" // Provides methods for generating pseudo random numbers
#include "repast_hpc/SVDataSetBuilder.h" // Used to build SVDataSets to record data in plain text tabular format

#include "Model.h"


BOOST_CLASS_EXPORT_GUID(repast::SpecializedProjectionInfoPacket<DemoModelCustomEdgeContent<RepastHPCAgent> >, "SpecializedProjectionInfoPacket_CUSTOM_EDGE");

RepastHPCDemoAgentPackageProvider::RepastHPCDemoAgentPackageProvider(repast::SharedContext<RepastHPCAgent>* agentPtr): agents(agentPtr){ }

void RepastHPCDemoAgentPackageProvider::providePackage(RepastHPCAgent * agent, std::vector<RepastHPCAgentPackage>& out)
{
    repast::AgentId id = agent->getId();
    RepastHPCAgentPackage package(id.id(), id.startingRank(), id.agentType(), id.currentRank(), agent->getC(), agent->getTotal());
    out.push_back(package);
}

void RepastHPCDemoAgentPackageProvider::provideContent(repast::AgentRequest req, std::vector<RepastHPCAgentPackage>& out)
{
    std::vector<repast::AgentId> ids = req.requestedAgents();
    for(size_t i = 0; i < ids.size(); i++)
    {
        providePackage(agents->getAgent(ids[i]), out);
    }
}


RepastHPCDemoAgentPackageReceiver::RepastHPCDemoAgentPackageReceiver(repast::SharedContext<RepastHPCAgent>* agentPtr): agents(agentPtr){}

RepastHPCAgent * RepastHPCDemoAgentPackageReceiver::createAgent(RepastHPCAgentPackage package)
{
    repast::AgentId id(package.id, package.rank, package.type, package.currentRank);
    return new RepastHPCAgent(id, package.c, package.total);
}

void RepastHPCDemoAgentPackageReceiver::updateAgent(RepastHPCAgentPackage package)
{
    repast::AgentId id(package.id, package.rank, package.type);
    RepastHPCAgent * agent = agents->getAgent(id);
    agent->set(package.currentRank, package.c, package.total);
}



DataSource_AgentTotals::DataSource_AgentTotals(repast::SharedContext<RepastHPCAgent>* c) : context(c){ }

int DataSource_AgentTotals::getData()
{
	int sum = 0;
	repast::SharedContext<RepastHPCAgent>::const_local_iterator iter    = context->localBegin();
	repast::SharedContext<RepastHPCAgent>::const_local_iterator iterEnd = context->localEnd();
	while( iter != iterEnd)
    {
		sum+= (*iter)->getTotal();
		iter++;
	}
	return sum;
}

DataSource_AgentCTotals::DataSource_AgentCTotals(repast::SharedContext<RepastHPCAgent>* c) : context(c){ }

int DataSource_AgentCTotals::getData()
{
	int sum = 0;
	repast::SharedContext<RepastHPCAgent>::const_local_iterator iter    = context->localBegin();
	repast::SharedContext<RepastHPCAgent>::const_local_iterator iterEnd = context->localEnd();
	while( iter != iterEnd)
    {
		sum+= (*iter)->getC();
		iter++;
	}
	return sum;
}


RepastHPCModel::RepastHPCModel(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm): context(comm) //argc argv added so that properties can be entered via command line if wanted.
{
	props = new repast::Properties(propsFile, argc, argv, comm); // property object instantiated  with propsfile name, mpi communicator and main arguments. Mpi comm added so props file only has to be read once
	stopAt = repast::strToInt(props->getProperty("stop.at")); // stopAt var initialised based on property file value.
	countOfAgents = repast::strToInt(props->getProperty("count.of.agents"));
	initializeRandom(*props, comm); //initialises random number generator and takes mpi communicator to pass random seed across processes.
	if(repast::RepastProcess::instance()->rank() == 0) props->writeToSVFile("./output/record.csv"); // writes the properties from the props file to csv file each time simulation is run.
	provider = new RepastHPCDemoAgentPackageProvider(&context);
	receiver = new RepastHPCDemoAgentPackageReceiver(&context);

    agentNetwork = new repast::SharedNetwork<RepastHPCAgent, DemoModelCustomEdge<RepastHPCAgent>, DemoModelCustomEdgeContent<RepastHPCAgent>, DemoModelCustomEdgeContentManager<RepastHPCAgent> >("agentNetwork", false, &edgeContentManager);
	context.addProjection(agentNetwork);

	// Data collection
	// Create the data set builder
	std::string fileOutputName("./output/agent_total_data.csv"); // string to hold file directory data should be written to
	repast::SVDataSetBuilder builder(fileOutputName.c_str(), ",", repast::RepastProcess::instance()->getScheduleRunner().schedule()); // instantiate SVDataSetBuilder, specifying file to write to.

	// Create the individual data sets to be added to the builder
	DataSource_AgentTotals* agentTotals_DataSource = new DataSource_AgentTotals(&context);
	builder.addDataSource(createSVDataSource("Total", agentTotals_DataSource, std::plus<int>()));

	DataSource_AgentCTotals* agentCTotals_DataSource = new DataSource_AgentCTotals(&context);
	builder.addDataSource(createSVDataSource("C", agentCTotals_DataSource, std::plus<int>()));

	// Use the builder to create the data set
	agentValues = builder.createDataSet();

}

RepastHPCModel::~RepastHPCModel() // Model destructor to run on program completion to delete objects
{
	delete props;
	delete provider;
	delete receiver;
	delete agentValues;
}

void RepastHPCModel::init() //initialise the repast model. Populates model with agents
{
	int rank = repast::RepastProcess::instance()->rank(); //gets process rank
	for(int i = 0; i < countOfAgents; i++) //iterates based on number of agents
    {
		repast::AgentId id(i, rank, 0); // instantiates agent id with agent number, rank and type
		id.currentRank(rank);
		RepastHPCAgent* agent = new RepastHPCAgent(id); //instantiate agent objects with id
		context.addAgent(agent); //adds agent to the context
    }
}

void RepastHPCModel::requestAgents()
{
	int rank = repast::RepastProcess::instance()->rank();
	int worldSize = repast::RepastProcess::instance()->worldSize();
	repast::AgentRequest req(rank);
	for (int i = 0; i < worldSize; i++) // For each process
	{
        if(i != rank) // ... except this one
        {
			std::vector<RepastHPCAgent*> agents;
			context.selectAgents(5, agents); // Choose 5 local agents randomly
			for(size_t j = 0; j < agents.size(); j++)
			{
				repast::AgentId local = agents[j]->getId(); // Transform each local agent's id into a matching non-local one
				repast::AgentId other(local.id(), i, 0);
				other.currentRank(i);
				req.addRequest(other); // Add it to the agent request
			}
		}
	}
    repast::RepastProcess::instance()->requestAgents<RepastHPCAgent, RepastHPCAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, req, *provider, *receiver, *receiver);
}

void RepastHPCModel::connectAgentNetwork()
{
	repast::SharedContext<RepastHPCAgent>::const_local_iterator iter    = context.localBegin();
	repast::SharedContext<RepastHPCAgent>::const_local_iterator iterEnd = context.localEnd();
	while(iter != iterEnd)
    {
		RepastHPCAgent* ego = &**iter;
		std::vector<RepastHPCAgent*> agents;
		agents.push_back(ego);                          // Omit self
		context.selectAgents(5, agents, true);          // Choose 5 other agents randomly
		// Make an undirected connection
		for(size_t i = 0; i < agents.size(); i++){
            if(ego->getId().id() < agents[i]->getId().id()){
              std::cout << "CONNECTING: " << ego->getId() << " to " << agents[i]->getId() << std::endl;
              boost::shared_ptr<DemoModelCustomEdge<RepastHPCAgent> > demoEdge(new DemoModelCustomEdge<RepastHPCAgent>(ego, agents[i], i + 1, i * i));
  	  	      agentNetwork->addEdge(demoEdge);
            }
		}
		iter++;
	}
}

void RepastHPCModel::cancelAgentRequests()
{
	int rank = repast::RepastProcess::instance()->rank();
	if(rank == 0) std::cout << "CANCELING AGENT REQUESTS" << std::endl;
	repast::AgentRequest req(rank);

	repast::SharedContext<RepastHPCAgent>::const_state_aware_iterator non_local_agents_iter  = context.begin(repast::SharedContext<RepastHPCAgent>::NON_LOCAL);
	repast::SharedContext<RepastHPCAgent>::const_state_aware_iterator non_local_agents_end   = context.end(repast::SharedContext<RepastHPCAgent>::NON_LOCAL);
	while(non_local_agents_iter != non_local_agents_end){
		req.addCancellation((*non_local_agents_iter)->getId());
		non_local_agents_iter++;
	}
    repast::RepastProcess::instance()->requestAgents<RepastHPCAgent, RepastHPCAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, req, *provider, *receiver, *receiver);

	std::vector<repast::AgentId> cancellations = req.cancellations();
	std::vector<repast::AgentId>::iterator idToRemove = cancellations.begin();
	while(idToRemove != cancellations.end()){
		context.importedAgentRemoved(*idToRemove);
		idToRemove++;
	}
}


void RepastHPCModel::removeLocalAgents()
{
	int rank = repast::RepastProcess::instance()->rank();
	if(rank == 0) std::cout << "REMOVING LOCAL AGENTS" << std::endl;
	for(int i = 0; i < 5; i++){
		repast::AgentId id(i, rank, 0);
		repast::RepastProcess::instance()->agentRemoved(id);
		context.removeAgent(id);
	}
  repast::RepastProcess::instance()->synchronizeAgentStatus<RepastHPCAgent, RepastHPCAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, *provider, *receiver, *receiver);
}

void RepastHPCModel::moveAgents()
{
	int rank = repast::RepastProcess::instance()->rank();
	if(rank == 0){
		repast::AgentId agent0(0, 0, 0);
		repast::AgentId agent1(1, 0, 0);
		repast::AgentId agent2(2, 0, 0);
		repast::AgentId agent3(3, 0, 0);
		repast::AgentId agent4(4, 0, 0);

		repast::RepastProcess::instance()->moveAgent(agent0, 1);
		repast::RepastProcess::instance()->moveAgent(agent1, 2);
		repast::RepastProcess::instance()->moveAgent(agent2, 3);
		repast::RepastProcess::instance()->moveAgent(agent3, 3);
		repast::RepastProcess::instance()->moveAgent(agent4, 1);
	}

  repast::RepastProcess::instance()->synchronizeAgentStatus<RepastHPCAgent, RepastHPCAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, *provider, *receiver, *receiver);

}

void RepastHPCModel::doSomething() //method to run simulation time step functionality
{
	int whichRank = 0;
	if(repast::RepastProcess::instance()->rank() == whichRank) std::cout << " TICK " << repast::RepastProcess::instance()->getScheduleRunner().currentTick() << std::endl; // if the process rank is the same as whichRank then output the current tick

	if(repast::RepastProcess::instance()->rank() == whichRank)
    {
		std::cout << "LOCAL AGENTS:" << std::endl;
		for(int r = 0; r < 4; r++)
		{
			for(int i = 0; i < 10; i++)
			{
				repast::AgentId toDisplay(i, r, 0); // instantiate 'toDisplay' AgentId object with number i, rank r and type 0
				RepastHPCAgent* agent = context.getAgent(toDisplay); // get agent with current toDisplay id
				if((agent != 0) && (agent->getId().currentRank() == whichRank)) // only prints agents that are on current process
				{
				    std::cout << agent->getId() << " " << agent->getC() << " " << agent->getTotal() << std::endl; // display associated agent information
				}

			}
		}

		std::cout << "NON LOCAL AGENTS:" << std::endl;
		for(int r = 0; r < 4; r++)
        {
			for(int i = 0; i < 10; i++)
			{
				repast::AgentId toDisplay(i, r, 0);
				RepastHPCAgent* agent = context.getAgent(toDisplay);
				if((agent != 0) && (agent->getId().currentRank() != whichRank))
				{
                    std::cout << agent->getId() << " " << agent->getC() << " " << agent->getTotal() << std::endl; // display associated agent information
                }
			}
		}
	}

	std::vector<RepastHPCAgent*> agents;
	context.selectAgents(repast::SharedContext<RepastHPCAgent>::LOCAL, countOfAgents, agents);
	std::vector<RepastHPCAgent*>::iterator it = agents.begin(); //create vector iterator to hold agents
	while(it != agents.end()) //iterate through each agent
    {
		(*it)->play(agentNetwork); // play the agent game with agentNetwork
		it++;
    }

	repast::RepastProcess::instance()->synchronizeAgentStates<RepastHPCAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(*provider, *receiver);

}

void RepastHPCModel::initSchedule(repast::ScheduleRunner& runner) //runner object used to schedule events in the simulation
{
	runner.scheduleEvent(1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCModel> (this, &RepastHPCModel::requestAgents))); // event runs at timestep 1 of simulation and second parameter is a special class FunctorPtr that allows model instance method to be called.
    runner.scheduleEvent(1.1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCModel> (this, &RepastHPCModel::connectAgentNetwork)));
	runner.scheduleEvent(2, 1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCModel> (this, &RepastHPCModel::doSomething))); // second parameter indicates that doSomething() is run every tick
	runner.scheduleEvent(3, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCModel> (this, &RepastHPCModel::moveAgents)));
	runner.scheduleEndEvent(repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCModel> (this, &RepastHPCModel::recordResults)));
	runner.scheduleStop(stopAt); // simulation stops at stopAt time specified in the properties file.

	// Data collection
	runner.scheduleEvent(1.5, 5, repast::Schedule::FunctorPtr(new repast::MethodFunctor<repast::DataSet>(agentValues, &repast::DataSet::record)));
	runner.scheduleEvent(10.6, 10, repast::Schedule::FunctorPtr(new repast::MethodFunctor<repast::DataSet>(agentValues, &repast::DataSet::write)));
	runner.scheduleEndEvent(repast::Schedule::FunctorPtr(new repast::MethodFunctor<repast::DataSet>(agentValues, &repast::DataSet::write)));
}

void RepastHPCModel::recordResults() //scheduled to run as an end event. Writes model data to a file
{
	if(repast::RepastProcess::instance()->rank() == 0)
    {
		props->putProperty("Result","Passed");
		std::vector<std::string> keyOrder; // string vector initialised named 'keyOrder'
		keyOrder.push_back("RunNumber"); //properties to be output added to vector
		keyOrder.push_back("stop.at");
		keyOrder.push_back("Result");
		props->writeToSVFile("./output/results.csv", keyOrder); // keyOrder prop values output to file
    }
}



