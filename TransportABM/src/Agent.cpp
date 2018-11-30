/* Agent.cpp */

#include "Agent.h" // include agent header file

RepastHPCAgent::RepastHPCAgent(repast::AgentId id): id_(id), c(100), total(200){ }

RepastHPCAgent::RepastHPCAgent(repast::AgentId id, double newC, double newTotal): id_(id), c(newC), total(newTotal){ }

RepastHPCAgent::~RepastHPCAgent(){ } //Agent destructor 


void RepastHPCAgent::set(int currentRank, double newC, double newTotal)
{
    id_.currentRank(currentRank);
    c     = newC;
    total = newTotal;
}

void initAgent() // Function to set initial state variable values
{
    // Set age
    
    // Set commuting distance

    // Set societal normality 

    // Set agent region

}

void updateDesires() // Function to update the agent desires based on current state variable values
{
    

}

bool RepastHPCAgent::cooperate()
{
	return repast::Random::instance()->nextDouble() < c/total;
}

void RepastHPCAgent::play(repast::SharedNetwork<RepastHPCAgent,
                              ModelCustomEdge<RepastHPCAgent>,
                              ModelCustomEdgeContent<RepastHPCAgent>,
                              ModelCustomEdgeContentManager<RepastHPCAgent> > *network)
{
    std::vector<RepastHPCAgent*> agentsToPlay; // vector to hold agent objects, <> specifies vector type
    network->successors(this, agentsToPlay);

    double cPayoff = 0;
    double totalPayoff = 0;
    std::vector<RepastHPCAgent*>::iterator agentToPlay = agentsToPlay.begin(); // declare and initialise an iterator 'agentToPlay' that holds value of iterator position in agentsToPlay vector
    while(agentToPlay != agentsToPlay.end()) // iterates through each agent
    {
        boost::shared_ptr<ModelCustomEdge<RepastHPCAgent> > edge = network->findEdge(this, *agentToPlay);
        double edgeWeight = edge->weight();
        int confidence = edge->getConfidence();

        bool iCooperated = cooperate(); // Do I cooperate?
        double payoff = (iCooperated ?
						 ((*agentToPlay)->cooperate() ?  7 : 1) :  // If I cooperated, did my opponent?
						 ((*agentToPlay)->cooperate() ? 10 : 3));  // If I didn't cooperate, did my opponent?
        if(iCooperated) cPayoff += payoff * confidence * confidence * edgeWeight;
        totalPayoff             += payoff * confidence * confidence * edgeWeight;

        agentToPlay++;
    }
    c += cPayoff;
    total += totalPayoff;

}


/* Serializable Agent Package Data */

RepastHPCAgentPackage::RepastHPCAgentPackage(){ }

RepastHPCAgentPackage::RepastHPCAgentPackage(int _id, int _rank, int _type, int _currentRank, double _c, double _total):
id(_id), rank(_rank), type(_type), currentRank(_currentRank), c(_c), total(_total){ }
