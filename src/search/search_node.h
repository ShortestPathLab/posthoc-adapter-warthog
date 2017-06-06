#ifndef WARTHOG_SEARCH_NODE_H
#define WARTHOG_SEARCH_NODE_H

// search_node.h
//
// @author: dharabor
// @created: 10/08/2012
//

#include "constants.h"
#include "cpool.h"
#include "jps.h"

#include <iostream>

namespace warthog
{
	
const uint32_t NODEID_AND_STATUS_MASK = (1 << 24) - 1;
const uint32_t STATUS_MASK = 1;
class search_node
{
	public:
		search_node(uint32_t id) 
			: id_and_status_(id << 1), f_(warthog::INF), g_(warthog::INF), parent_(0), 
			priority_(warthog::INF), searchid_(0)
		{
			assert(this->get_id() < ((1ul<<31)-1));
            set_pdir(warthog::jps::direction::NONE);
			refcount_++;
		}

		~search_node()
		{
			refcount_--;
		}

		inline void
		init(uint32_t searchid, warthog::search_node* parent, double g, double f)
		{
			id_and_status_ &= ~1;
            parent_ = parent;
            f_ = f;
            g_ = g;
			searchid_ = searchid;
		}

		inline uint32_t
		get_search_id()
		{
			return searchid_;
		}

		inline void
		set_searchid(uint32_t searchid)
		{
			searchid_ = searchid;
		}

		inline uint32_t 
		get_id() const { return (id_and_status_ & NODEID_AND_STATUS_MASK) >> 1; }

		inline void
		set_id(uint32_t id) 
		{ 
			id = ((id << 1) | (id_and_status_ & 1)) & NODEID_AND_STATUS_MASK;
			id_and_status_ |= id;
		} 

		inline warthog::jps::direction
		get_pdir() const
		{
			return (warthog::jps::direction)
			   	*(((uint8_t*)(&id_and_status_))+3);
		}

		inline void
		set_pdir(warthog::jps::direction d)
		{
			*(((uint8_t*)(&id_and_status_))+3) = d;
		}

		inline bool
		get_expanded() const { return (id_and_status_ & STATUS_MASK); }

		inline void
		set_expanded(bool expanded) 
		{ 
			id_and_status_ &= ~STATUS_MASK; // reset bit0
			id_and_status_ ^= (uint32_t)(expanded?1:0); // set it anew
		}

		inline warthog::search_node* 
		get_parent() const { return parent_; }

		inline void
		set_parent(warthog::search_node* parent) { parent_ = parent; } 

		inline uint32_t
		get_priority() const { return priority_; }

		inline void
		set_priority(uint32_t priority) { priority_ = priority; } 

		inline double
		get_g() const { return g_; }

		inline void
		set_g(double g) { g_ = g; }

		inline double 
		get_f() const { return f_; }

		inline void
		set_f(double f) { f_ = f; }

		inline void 
		relax(double g, warthog::search_node* parent)
		{
			assert(g < g_);
			f_ = (f_ - g_) + g;
			g_ = g;
			parent_ = parent;
		}

		inline bool
		operator<(const warthog::search_node& other) const
		{
			if(f_ < other.f_)
			{
				return true;
			}
			if(f_ > other.f_)
			{
				return false;
			}

			// break ties in favour of larger g
			if(g_ > other.g_)
			{
				return true;
			}
			return false;
		}

		inline bool
		operator>(const warthog::search_node& other) const
		{
			if(f_ > other.f_)
			{
				return true;
			}
			if(f_ < other.f_)
			{
				return false;
			}

			// break ties in favour of larger g
			if(g_ > other.g_)
			{
				return true;
			}
			return false;
		}

		inline bool
		operator==(const warthog::search_node& other) const
		{
			if( !(*this < other) && !(*this > other))
			{
				return true;
			}
			return false;
		}

		inline bool
		operator<=(const warthog::search_node& other) const
		{
			if(*this < other)
			{
				return true;
			}
			if(!(*this > other))
			{
				return true;
			}
			return false;
		}

		inline bool
		operator>=(const warthog::search_node& other) const
		{
			if(*this > other)
			{
				return true;
			}
			if(!(*this < other))
			{
				return true;
			}
			return false;
		}

		inline void 
		print(std::ostream&  out) const
		{
			out << "search_node id:" << get_id();
            out << " p_id: ";
            if(parent_)
            {
                out << parent_->get_id();
            }
            else
            {
                out << -1;
            }
            out << " g: "<<g_ <<" f: "<<this->get_f() 
            << " expanded: " << get_expanded() << " " 
            << " searchid: " << searchid_ 
            << " pdir: "<< get_pdir() << " ";
		}

		static uint32_t 
		get_refcount() { return refcount_; }

		uint32_t
		mem()
		{
			return sizeof(*this);
		}

	private:
		uint32_t id_and_status_; // bit 0 is expansion status; 1-31 are id
		double f_;
		double g_;
		warthog::search_node* parent_;
		uint32_t priority_; // expansion priority
		uint32_t searchid_;

		static uint32_t refcount_;
};

}

#endif

