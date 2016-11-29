#ifndef WARTHOG_FCH_CPG_EXPANSION_POLICY_H
#define WARTHOG_FCH_CPG_EXPANSION_POLICY_H

// contraction/fch_cpg_expansion_policy.h
//
// Forward-driven search in contraction hiearchies applied
// to corner point graphs.
//
// @author: dharabor
// @created: 2016-011-29
//

#include "expansion_policy.h"
#include <vector>

namespace warthog
{

namespace graph
{
class corner_point_graph;
}

class problem_instance;
class search_node;

class fch_cpg_expansion_policy : public expansion_policy
{
    public:
        fch_cpg_expansion_policy(
                warthog::graph::corner_point_graph* graph,
                std::vector<uint32_t>* rank);

        ~fch_cpg_expansion_policy();

		virtual void 
		expand(warthog::search_node*, warthog::problem_instance*);

        virtual void
        get_xy(uint32_t node_id, int32_t& x, int32_t& y);

        virtual inline size_t
        mem()
        {
            return expansion_policy::mem() +
                sizeof(this);
        }

    private:
        std::vector<uint32_t>* rank_;
        warthog::graph::corner_point_graph* g_;

        inline uint32_t
        get_rank(uint32_t id)
        {
            return rank_->at(id);
        }

        void
        init();
};
}

#endif
