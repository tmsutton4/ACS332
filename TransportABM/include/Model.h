/* Model.h */

#ifndef MODEL
#define MODEL

#include <boost/mpi.hpp> // include boost mpi wrapper
#include "repast_hpc/Schedule.h"
#include "repast_hpc/Properties.h"
#include "repast_hpc/SharedContext.h" // templated class hence use <> to specify class to use.
#include "repast_hpc/AgentRequest.h"
#include "repast_hpc/TDataSource.h"
#include "repast_hpc/SVDataSet.h"
#include "repast_hpc/SharedNetwork.h"

#include "Network.h"
#include "Agent.h"


/* Agent Package Provider */
class RepastHPCAgentPackageProvider
{

    private:
        repast::SharedContext<RepastHPCAgent>* agents; // agent shared context object 'agents'

    public:

        RepastHPCAgentPackageProvider(repast::SharedContext<RepastHPCAgent>* agentPtr);

        void providePackage(RepastHPCAgent * agent, std::vector<RepastHPCAgentPackage>& out);

        void provideContent(repast::AgentRequest req, std::vector<RepastHPCAgentPackage>& out);

};

/* Agent Package Receiver */
class RepastHPCAgentPackageReceiver
{

    private:
        repast::SharedContext<RepastHPCAgent>* agents;

    public:

        RepastHPCAgentPackageReceiver(repast::SharedContext<RepastHPCAgent>* agentPtr);

        RepastHPCAgent * createAgent(RepastHPCAgentPackage package);

        void updateAgent(RepastHPCAgentPackage package);

};


/* Data Collection */
class DataSource_AgentTotals : public repast::TDataSource<int>
{
    private:
        repast::SharedContext<RepastHPCAgent>* context;

    public:
        DataSource_AgentTotals(repast::SharedContext<RepastHPCAgent>* c);
        int getData();
};


class DataSource_AgentCTotals : public repast::TDataSource<int>
{
    private:
        repast::SharedContext<RepastHPCAgent>* context;

    public:
        DataSource_AgentCTotals(repast::SharedContext<RepastHPCAgent>* c);
        int getData();
};

class RepastHPCModel
{
	int stopAt; //integer to define the stop time of the simulation. Indicated as a time step.
	int countOfAgents; // holds the number of agents in the model

	repast::Properties* props; //properties object
	repast::SharedContext<RepastHPCAgent> context;

	RepastHPCAgentPackageProvider* provider;
	RepastHPCAgentPackageReceiver* receiver;

    	ModelCustomEdgeContentManager<RepastHPCAgent> edgeContentManager;

	repast::SVDataSet* agentValues;
	repast::SharedNetwork<RepastHPCAgent, ModelCustomEdge<RepastHPCAgent>, ModelCustomEdgeContent<RepastHPCAgent>, ModelCustomEdgeContentManager<RepastHPCAgent> >* agentNetwork;

public:
	RepastHPCModel(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm); // model constructor that takes properties file filename and an mpi communicator object
	~RepastHPCModel(); // model destructor - necessary as instantiated objects on heap must be destroyed once used to prevent memory leakage.
	void init(); // initialises model and populates with agents.
	void requestAgents();
        void connectAgentNetwork();
	void cancelAgentRequests();
	void removeLocalAgents();
	void moveAgents();
	void doSomething(); //runs model dynamics
	void initSchedule(repast::ScheduleRunner& runner); //enables model to initialise a schedule
	void recordResults();
};

#endif
