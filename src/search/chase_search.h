#ifndef WARTHOG_CHASE_SEARCH_H
#define WARTHOG_CHASE_SEARCH_H

// search/chase_search.h
//
// An algorithm resembling CHASE. For theoretical details see:
//
// [Bauer, Delling, Sanders, Schieferdecker, Schultes and Wagner, 
// Combining Hierarchical and Goal-directed Speed-up Techniques 
// for Dijkstra's Algorithm, Journal of Experimental Algorithms,
// vol 15, 2010]
//
// NB: NOT FULLY IMPLEMENTED!! Only stalls nodes higher than 
// some cutoff and resumes the search from these nodes if no
// optimal path has been found.
//
// @author: dharabor
// @created: 2016-09-10
//

#include "constants.h"
//#include "expansion_policy.h"
#include "planar_graph.h"
#include "pqueue.h"
#include "search.h"
#include "search_node.h"
#include "timer.h"
#include "zero_heuristic.h"

#include "constants.h"
#include <cstdlib>
#include <stack>
#include <stdint.h>

namespace warthog
{

namespace graph
{
    class planar_graph;
}

class expansion_policy;
class pqueue;
class search_node;

typedef double (* heuristicFn)
(uint32_t nodeid, uint32_t targetid);

template<class E, class H>
class chase_search : public warthog::search
{
    public:
        chase_search(E* fexp, E* bexp, H* heuristic) 
            : fexpander_(fexp), bexpander_(bexp), heuristic_(heuristic)
        {
            fopen_ = new pqueue(512, true);
            bopen_ = new pqueue(512, true);
            
            dijkstra_ = false;
            if(typeid(*heuristic_) == typeid(warthog::zero_heuristic))
            {
                dijkstra_ = true;
            }
            
            max_phase1_rank_ = fexpander_->get_num_nodes()*0.95;
        }

        ~chase_search()
        {
            delete fopen_;
            delete bopen_;
        }

        void
        get_path(warthog::problem_instance& pi, warthog::solution& sol)
        {
            pi_ = pi;
            this->search(sol);
            if(best_cost_ != warthog::INF) 
            { 
                sol.sum_of_edge_costs_ = best_cost_;
                reconstruct_path(sol);
            }
            cleanup();

            #ifndef NDEBUG
            if(pi_.verbose_)
            {
                std::cerr << "path: \n";
                for(uint32_t i = 0; i < sol.path_.size(); i++)
                {
                    std::cerr << sol.path_.at(i) << std::endl;
                }
            }
            #endif
        }
            
        size_t
        mem()
        {
            return sizeof(*this) + 
                fopen_->mem() +
                bopen_->mem() +
                fexpander_->mem();
                bexpander_->mem();
        }

    private:
        warthog::pqueue* fopen_;
        warthog::pqueue* bopen_;
        E* fexpander_;
        E* bexpander_;
        H* heuristic_;
        bool dijkstra_;
        bool forward_next_;

        // CHASE-specific stuff
        uint32_t phase_;
        uint32_t max_phase1_rank_;
        std::vector<warthog::search_node*> fwd_norelax_;
        std::vector<warthog::search_node*> bwd_norelax_;

        // v is the section of the path in the forward
        // direction and w is the section of the path
        // in the backward direction. need parent pointers
        // of both to extract the actual path
        warthog::search_node* v_;
        warthog::search_node* w_;
        double best_cost_;
        warthog::problem_instance pi_;

        void
        reconstruct_path(warthog::solution& sol)
        {
            if(v_ && (&*v_ == &*bexpander_->generate(v_->get_id())))
            {
                warthog::search_node* tmp = v_;
                v_ = w_;
                w_ = tmp;
            }

            warthog::search_node* current = v_;
            while(current)
            {
               sol.path_.push_back(current->get_id());
               current = current->get_parent();
            }
            std::reverse(sol.path_.begin(), sol.path_.end());

            current = w_->get_parent();
            while(current)
            {  
               sol.path_.push_back(current->get_id());
               current = current->get_parent();
            }

        }

        void 
        search(warthog::solution& sol)
        {
            warthog::timer mytimer;
            mytimer.start();

            // init
            best_cost_ = warthog::INF;
            v_ = w_ = 0;
            forward_next_ = true;
            fwd_norelax_.clear();
            bwd_norelax_.clear();

            warthog::search_node *start, *target;
            start = fexpander_->generate_start_node(&pi_);
            target = bexpander_->generate_target_node(&pi_);
            pi_.start_id_ = start->get_id();
            pi_.target_id_ = target->get_id();
            
            #ifndef NDEBUG
            if(pi_.verbose_)
            {
                std::cerr << "chase_search. ";
                pi_.print(std::cerr);
                std::cerr << std::endl;
            }
            #endif

            // initialise the start and target
            start->init(pi_.instance_id_, 0, 0, 
                    heuristic_->h(pi_.start_id_, pi_.target_id_));
            target->init(pi_.instance_id_, 0, 0, 
                    heuristic_->h(pi_.start_id_, pi_.target_id_));

            // these variables help interleave the search, decide when to
            // switch phases and when to terminate
            phase_ = 1;
            bool cannot_improve = false;
            double fwd_core_lb = DBL_MAX;
            double bwd_core_lb = DBL_MAX;
            //uint32_t fwd_search_mask = UINT32_MAX;
            //uint32_t bwd_search_mask = UINT32_MAX;
            uint32_t search_direction = 1;

            // search begin
            fopen_->push(start);
            bopen_->push(target);
            while(true)
            {
                // expand something
                switch(search_direction)
                {
                    case 0:
                    {
                        cannot_improve = true;
                        break;
                    }
                    case 1:
                    {
                        warthog::search_node* current = fopen_->pop();
                        if(current->get_f() >= best_cost_) // terminate early
                        {
                            //fwd_search_mask = 0; // fwd search finished
                        }
                        else
                        {
                            expand(current, fopen_, fexpander_, bexpander_, 
                                    pi_.target_id_, fwd_norelax_, 
                                    fwd_core_lb, sol);
                        }
                        // switch directions
                        if(bopen_->size() > 0)  
                        { search_direction = 2; }
                        else if(fopen_->size() > 0) 
                        { search_direction = 1; }
                        else
                        { search_direction = 0; } 
                        break;
                    }
                    case 2:
                    {
                        warthog::search_node* current = bopen_->pop();
                        if(current->get_f() >= best_cost_) // terminate early
                        {
                            //bwd_search_mask = 0; // bwd search finished
                        }
                        else
                        {
                            expand(current, bopen_, bexpander_, fexpander_, 
                                    pi_.target_id_, bwd_norelax_, 
                                    bwd_core_lb, sol);
                        }
                        // switch directions
                        if(fopen_->size() > 0)  
                        { search_direction = 1; }
                        else if(bopen_->size() > 0) 
                        { search_direction = 2; }
                        else
                        { search_direction = 0; } 
                        break; 
                    }
                }
                if(pi_.verbose_)
                {
                    std::cerr 
                        << "best_cost " << best_cost_ 
                        << " fwd_ub: " << fwd_core_lb 
                        << " bwd_ub: " << bwd_core_lb
                        << std::endl;
                }


                if(cannot_improve)
                {
                    if(phase_ == 1)
                    {
                        uint32_t fwd_lower_bound = 
                            std::min(fwd_core_lb, fopen_->size() > 0 ?  
                                        fopen_->peek()->get_f() : DBL_MAX);

                        uint32_t bwd_lower_bound = 
                            std::min(bwd_core_lb, bopen_->size() > 0 ?  
                                        bopen_->peek()->get_f() : DBL_MAX);

                        uint32_t best_bound = 
                            std::min(fwd_lower_bound, bwd_lower_bound);
                        
                        // early terminate; optimal path does not involve
                        // any nodes from the core 
                        if(best_bound >= best_cost_)
                        {
                            #ifndef NDEBUG
                            if(pi_.verbose_)
                            {
                                std::cerr 
                                    << "provably-best solution found; "
                                    << "cost=" << best_cost_ << std::endl;
                            }
                            #endif
                            break; 
                        }

                        // early terminate if we can't reach the core
                        // in both directions
                        if(fwd_core_lb == DBL_MAX || bwd_core_lb == DBL_MAX)
                        { break; }
                        

                        // both directions can reach the core; time for phase2
                        fopen_->clear();
                        bopen_->clear();
                        for(uint32_t i = 0; i < fwd_norelax_.size(); i++)
                        {
                            fopen_->push(fwd_norelax_.at(i));
                        }
                        for(uint32_t i = 0; i < bwd_norelax_.size(); i++)
                        {
                            bopen_->push(bwd_norelax_.at(i));
                        }
                         
                        // reset variables that control the search
                        phase_ = 2;
                        cannot_improve = false;
                        //fwd_search_mask = bwd_search_mask = UINT32_MAX;
                        fwd_core_lb = bwd_core_lb = DBL_MAX;
                        search_direction = 1;

                        #ifndef NDEBUG
                        if(pi_.verbose_)
                        {
                            std::cerr << "=== PHASE2 ===" << std::endl;
                        }
                        #endif
                    }
                    else 
                    {
                        // phase 2 complete
                        #ifndef NDEBUG
                        if(pi_.verbose_)
                        {
                            if(best_cost_ != warthog::INF)
                            {
                                std::cerr 
                                    << "provably-best solution found; "
                                    << "cost=" << best_cost_ << std::endl;
                            }
                            else
                            {
                                std::cerr 
                                    << "no solution exists" << std::endl;
                            }
                        }
                        #endif
                        break; 
                    } 
                }
            }

			mytimer.stop();
			sol.time_elapsed_micro_= mytimer.elapsed_time_micro();
            assert(best_cost_ != warthog::INF || (v_ == 0 && w_ == 0));
        }

        void
        expand( warthog::search_node* current,
                warthog::pqueue* open,
                E* expander,
                E* reverse_expander, 
                uint32_t tmp_targetid,
                std::vector<warthog::search_node*>& norelax, 
                double&  norelax_distance_min,
                warthog::solution& sol)
        {
            // target not found yet; expand as normal
            current->set_expanded(true);
            expander->expand(current, &pi_);
            sol.nodes_expanded_++;

            #ifndef NDEBUG
            if(pi_.verbose_)
            {
                int32_t x, y;
                expander->get_xy(current->get_id(), x, y);
                std::cerr 
                    << sol.nodes_expanded_ 
                    << ". expanding " 
                    << (&*open == &*fopen_ ? "(f)" : "(b)")
                    << " ("<<x<<", "<<y<<")...";
                current->print(std::cerr);
                std::cerr << std::endl;
            }
            #endif
            
            // generate all neighbours
            warthog::search_node* n = 0;
            double cost_to_n = warthog::INF;
            for(expander->first(n, cost_to_n); n != 0; expander->next(n, cost_to_n))
            {
                sol.nodes_touched_++;
                if(n->get_expanded())
                {
                    // skip neighbours already expanded
                    #ifndef NDEBUG
                    if(pi_.verbose_)
                    {
                        int32_t x, y;
                        expander->get_xy(n->get_id(), x, y);
                        std::cerr << "  closed; (edgecost=" << cost_to_n << ") "
                            << "("<<x<<", "<<y<<")...";
                        n->print(std::cerr);
                        std::cerr << std::endl;

                    }
                    #endif
                    continue;
                }

                // relax (or generate) each neighbour
                double gval = current->get_g() + cost_to_n;
                if(open->contains(n))
                {
                    // update a node from the fringe
                    if(gval < n->get_g())
                    {
                        sol.nodes_updated_++;
                        n->relax(gval, current);
                        open->decrease_key(n);
                        #ifndef NDEBUG
                        if(pi_.verbose_)
                        {
                            int32_t x, y;
                            expander->get_xy(n->get_id(), x, y);
                            std::cerr << " updating "
                                << "(edgecost="<< cost_to_n<<") "
                                << "("<<x<<", "<<y<<")...";
                            n->print(std::cerr);
                            std::cerr << std::endl;
                        }
                        #endif
                    }
                    else
                    {
                        #ifndef NDEBUG
                        if(pi_.verbose_)
                        {
                            int32_t x, y;
                            expander->get_xy(n->get_id(), x, y);
                            std::cerr << " not updating "
                                << "(edgecost=" << cost_to_n<< ") "
                                << "("<<x<<", "<<y<<")...";
                            n->print(std::cerr);
                            std::cerr << std::endl;
                        }
                        #endif
                    }
                }
                else
                {
                    if(phase_ == 1 && 
                            n->get_search_id() == current->get_search_id())
                    {
                        // relax the g-value of the nodes not being
                        // expanded in phase1
                        // (these are not added to open yet)
                        if(gval < n->get_g())
                        {
                            n->relax(gval, current);
                            if(gval < norelax_distance_min)
                            {
                                norelax_distance_min = gval; 
                            }
                        }
                    }
                    else
                    {
                        // add a new node to the fringe
                        sol.nodes_inserted_++;
                        n->init(current->get_search_id(), current, gval,
                                gval + 
                                heuristic_->h(n->get_id(), tmp_targetid));

                        if( phase_ == 2 || 
                            expander->get_rank(n->get_id()) < max_phase1_rank_)
                        {
                            open->push(n);
                        }
                        else
                        {
                            norelax.push_back(n);
                            if(gval < norelax_distance_min)
                            {
                                norelax_distance_min = gval; 
                            }
                        }
                    }

                    #ifndef NDEBUG
                    if(pi_.verbose_)
                    {
                        if( phase_ == 1 &&
                            expander->get_rank(n->get_id()) >= max_phase1_rank_)
                        {
                            std::cerr << "phase2-list ";
                        }
                        else
                        {
                            std::cerr << "generating ";
                        }

                        int32_t x, y;
                        expander->get_xy(n->get_id(), x, y);
                        std::cerr 
                            << "(edgecost=" << cost_to_n<<") " 
                            << "("<<x<<", "<<y<<")...";
                        n->print(std::cerr);
                        std::cerr << std::endl;
                    }
                    #endif
                }

                // update the best solution if possible
                warthog::search_node* reverse_n = 
                    reverse_expander->generate(n->get_id());
                if(reverse_n->get_search_id() == n->get_search_id())
//                        && reverse_n->get_expanded())
                {
                    if((current->get_g() + cost_to_n + reverse_n->get_g()) < best_cost_)
                    {
                        v_ = n;
                        w_ = reverse_n;
                        best_cost_ = current->get_g() + cost_to_n + reverse_n->get_g();

                        #ifndef NDEBUG
                        if(pi_.verbose_)
                        {
                            int32_t x, y;
                            expander->get_xy(current->get_id(), x, y);
                            std::cerr <<"new best solution!  cost=" << best_cost_<<std::endl;
                        }
                        #endif
                    }
                }
            }

            #ifndef NDEBUG
            if(pi_.verbose_)
            {
                int32_t x, y;
                expander->get_xy(current->get_id(), x, y);
                std::cerr <<"closing ("<<x<<", "<<y<<")...";
                current->print(std::cerr);
                std::cerr << std::endl;
            }
            #endif
        }

        void
        cleanup()
        {
            fopen_->clear();
            bopen_->clear();
            fexpander_->clear();
            bexpander_->clear();
        }

};

}

#endif

