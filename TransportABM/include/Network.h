/* Network.h */

#ifndef NETWORK
#define NETWORK

#include "repast_hpc/SharedContext.h"
#include "repast_hpc/SharedNetwork.h"


/* Custom Network Components */
template<typename V>
class ModelCustomEdge : public repast::RepastEdge<V>
{
private:
    int confidence;

public:
    ModelCustomEdge(){}
    ModelCustomEdge(V* source, V* target) : repast::RepastEdge<V>(source, target) {}
    ModelCustomEdge(V* source, V* target, double weight) : repast::RepastEdge<V>(source, target, weight) {}
    ModelCustomEdge(V* source, V* target, double weight, int confidence) : repast::RepastEdge<V>(source, target, weight), confidence(confidence) {}

    ModelCustomEdge(boost::shared_ptr<V> source, boost::shared_ptr<V> target) : repast::RepastEdge<V>(source, target) {}
    ModelCustomEdge(boost::shared_ptr<V> source, boost::shared_ptr<V> target, double weight) : repast::RepastEdge<V>(source, target, weight) {}
    ModelCustomEdge(boost::shared_ptr<V> source, boost::shared_ptr<V> target, double weight, int confidence) : repast::RepastEdge<V>(source, target, weight), confidence(confidence) {}


    int getConfidence(){ return confidence; }
    void setConfidence(int con){ confidence = con; }

};

/* Custom Edge Content */
template<typename V>
struct ModelCustomEdgeContent : public repast::RepastEdgeContent<V>
{

    friend class boost::serialization::access;

public:
    int confidence;
    ModelCustomEdgeContent(){}
    ModelCustomEdgeContent(ModelCustomEdge<V>* edge): repast::RepastEdgeContent<V>(edge), confidence(edge->getConfidence()){}

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        repast::RepastEdgeContent<V>::serialize(ar, version);
        ar & confidence;
    }

};

/* Custom Edge Content Manager */
template<typename V>
class ModelCustomEdgeContentManager
{
public:
    ModelCustomEdgeContentManager(){}
    virtual ~ModelCustomEdgeContentManager(){}
    ModelCustomEdge<V>* createEdge(ModelCustomEdgeContent<V>& content, repast::Context<V>* context)
    {
        return new ModelCustomEdge<V>(context->getAgent(content.source), context->getAgent(content.target), content.weight, content.confidence);
    }
    ModelCustomEdgeContent<V>* provideEdgeContent(ModelCustomEdge<V>* edge)
    {
        return new ModelCustomEdgeContent<V>(edge);
    }
};

#endif
