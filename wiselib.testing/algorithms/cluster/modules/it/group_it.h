/*
 * File:   sema_it.h
 * Author: amaxilat
 */

#ifndef __GROUP_IT_H_
#define __GROUP_IT_H_

#include "util/pstl/vector_static.h"
#include "util/pstl/map_static_vector.h"
#include "util/delegates/delegate.hpp"
#include "util/pstl/iterator.h"
#include "util/pstl/pair.h"

namespace wiselib {

    /**
     * \ingroup it_concept
     *
     * Group iterator module.
     */
    template<typename OsModel_P, typename Radio_P, typename Semantics_P>
    class GroupIterator {
    public:
        //TYPEDEFS
        typedef OsModel_P OsModel;
        // os modules
        typedef Radio_P Radio;
        //typedef typename OsModel::Radio Radio;
        typedef typename OsModel::Timer Timer;
        typedef typename OsModel::Debug Debug;
        typedef Semantics_P Semantics_t;
        typedef typename Semantics_t::predicate_t semantic_id_t; //need to remove it
        typedef typename Semantics_t::predicate_t predicate_t;
        typedef typename Semantics_t::value_t value_t;
        typedef typename Semantics_t::group_container_t group_container_t;
        typedef typename Semantics_t::value_container_t value_container_t;
#ifdef ANSWERING
        typedef typename Semantics_t::predicate_container_t predicate_container_t;
#endif
        typedef typename Semantics_t::group_entry_t group_entry_t;

        //        struct semantic_head {
        //            semantic_id_t semantic_id_;
        //            value_t semantic_value_;
        //        };
        //

        struct semantic_head_item {
            block_data_t data[20];
            predicate_t predicate;
            value_t value;
        };
        typedef semantic_head_item semantic_head_item_t;
        typedef wiselib::pair<semantic_head_item_t, semantic_head_item_t> semantic_head_entry_t;
        typedef wiselib::vector_static<OsModel, semantic_head_entry_t, 10 > semanticHeadVector_t;

        typedef GroupIterator<OsModel_P, Radio_P, Semantics_P> self_t;

        // data types
        typedef int cluster_id_t;
        typedef typename Radio::node_id_t node_id_t;
        typedef typename Radio::block_data_t block_data_t;
        //
        //        typedef wiselib::vector_static<OsModel, node_id_t, 20 > vector_t;
        //        typedef wiselib::pair<node_id_t, cluster_id_t> gateway_entry_t;
        //        //        typedef wiselib::MapStaticVector<OsModel, node_id_t, cluster_id_t, 20 > gateway_vector_t;
        //        typedef wiselib::vector_static<OsModel, wiselib::pair<node_id_t, node_id_t>, 20 > tree_childs_t;


        typedef wiselib::vector_static<OsModel, node_id_t, 10 > groupMembers_t;

        struct groupsJoinedEntry {
            group_entry_t group;
            node_id_t parent;
            groupMembers_t groupMembers;
        };
        typedef struct groupsJoinedEntry groupsJoinedEntry_t;
        typedef wiselib::vector_static<OsModel, groupsJoinedEntry_t, 10 > groupsVector_t;
        typedef typename groupsVector_t::iterator groupsVectorIterator_t;

        /*
         * Constructor
         */
        GroupIterator() :
        node_type_(UNCLUSTERED),
        isGateway_(false) {
            groupsVector_.clear();
            lastNotified_ = groupsVector_.begin();
        }

        /*
         * Destructor
         * */
        ~GroupIterator() {
        }

        /*
         * INIT
         * initializes the values of radio timer and debug
         */
        void init(Radio& radio, Timer& timer, Debug& debug, Semantics_t &semantics) {
            radio_ = &radio;
            timer_ = &timer;
            debug_ = &debug;
            isGateway_ = false;
            semantics_ = &semantics;
        }

        /**
         * SET functions : node_type_
         */
        inline void set_node_type(int nodeType) {
            node_type_ = nodeType;
        }

        /**
         * GET functions : parent_
         */
        inline node_id_t parent(group_entry_t gi) {
            if (!groupsVector_.empty()) {
                for (groupsVectorIterator_t it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
                    //if of the same size maybe the same
                    if (gi == it->group) {
                        return it->parent;
                    }
                }
            }
            return Radio::NULL_NODE_ID;
        }

        /**
         * GET functions : node_type_
         */
        inline int node_type() {
            return node_type_;
        }

        /**
         * Enable
         */
        void reset(void) {
            node_type_ = UNCLUSTERED;
            isGateway_ = false;
            groupsVector_.clear();
            group_container_t mygroups = semantics_->get_groups();

            for (typename group_container_t::iterator gi = mygroups.begin(); gi != mygroups.end(); ++gi) {
                add_group(*gi, radio().id());
            }
        }

        bool is_gateway() {
            return isGateway_;
        }

        void parse_join(block_data_t * data, size_t len) {
            SemaGroupsMsg_t * msg = (SemaGroupsMsg_t *) data;
            uint8_t group_count = msg->contained();
            //            debug_->debug("contains %d ,len : %d", group_count, len);

            for (uint8_t i = 0; i < group_count; i++) {
                group_entry_t gi = group_entry_t(msg->get_statement_data(i), msg->get_statement_size(i));
                if (semantics_-> has_group(gi)) {
                    addNode2Members(gi, msg->node_id());
                }
            }
        }

        void addNode2Members(group_entry_t gi, node_id_t node) {
            for (groupsVectorIterator_t it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
                if (it->group == gi) {
                    //                    debug().debug("Node %x for group %s has %d members", radio().id(), it->group.c_str(), it->groupMembers.size());
                    for (typename groupMembers_t::iterator gmit = it->groupMembers.begin(); gmit != it->groupMembers.end(); ++gmit) {
                        if (node == *gmit) {
                            return;
                        }
                    }
                    it->groupMembers.push_back(node);
                }
            }
        }

        //return the number of nodes known

        inline size_t node_count(int type) {
            if (type == 1) {
                //inside cluster
                //                return cluster_neighbors_.size();
            } else if (type == 0) {
                //outside cluster
                return 0;
                //                return non_cluster_neighbors_.size();
            }
            return 0;
        }

        SemaResumeMsg_t get_resume_payload() {
            SemaResumeMsg_t msg;
            msg.set_node_id(radio().id());

            msg.set_group((block_data_t*) (lastNotified_->group.data()), lastNotified_->group.size());

#ifdef ANSWERING
            predicate_container_t my_predicates = semantics_->get_predicates();

            for (typename predicate_container_t::iterator it = my_predicates.begin(); it != my_predicates.end(); ++it) {
                value_container_t myvalues = semantics_->get_values(*it);
                for (typename value_container_t::iterator gi = myvalues.begin(); gi != myvalues.end(); ++gi) {
                    msg.add_predicate(it->data(), it->size(), gi->data(), gi->size());
                }
            }
#endif

            lastNotified_++;
            return msg;
        }

        void eat_resume(SemaResumeMsg_t * msg) {


            node_id_t sender = msg->node_id();
            group_entry_t gi = group_entry_t(msg->group_data(), msg->group_size());

            debug().debug("received resume for %s from %x", gi.c_str(), sender);

            //            //            size_t count = msg->contained();
            //            //            for (size_t i = 0; i < count; i++) {
            //            //#ifdef ANSWERING
            //            //
            //            //                predicate_t predicate = predicate_t(msg->get_predicate_data(i), msg->get_predicate_size(i), semantics_->get_allocator());
            //            //                value_t value = value_t(msg->get_value_data(i), msg->get_value_size(i), semantics_->get_allocator());
            //            //                add_semantic_value(predicate, value);
            //            //                //                debug().debug("Received a resume with %d|%d statement from %x", predicate, value, sender);
            //            //
            //            //#endif
            //            //            }
            //            //            node_joined(sender);
        }

        //        void add_semantic_value(predicate_t predicate, value_t value) {//NEDDDSS TOO RREEE TTHHIIINNKKK
        //            if (!semanticHeadVector_.empty()) {
        //                for (typename semanticHeadVector_t::iterator it = semanticHeadVector_.begin();
        //                        it != semanticHeadVector_.end(); ++it) {
        //                    if (semantics_->cmp(predicate, it->first.predicate, predicate) == 0) {
        //                        semantics_->aggregate(it->second.value, value, predicate);
        //                    }
        //                }
        //            }
        //            semantic_head_entry_t newentry;
        //            newentry.first.predicate = predicate;
        //            newentry.second.value = value;
        //            semanticHeadVector_.push_back(newentry);
        //        }

        bool add_group(group_entry_t gi, node_id_t parent) {
            bool same = false;
            if (!groupsVector_.empty()) {
                for (groupsVectorIterator_t it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
                    //if of the same size maybe the same
                    if (gi == it->group) {
                        same = true;
                        return !same;
                    }
                }
            }
            groupsJoinedEntry_t newgroup;
            newgroup.group = gi;
            newgroup.parent = parent;
            newgroup.groupMembers.clear();
            if (parent != radio().id()) {
                newgroup.groupMembers.push_back(parent);
            }
            groupsVector_.push_back(newgroup);
            return !same;
        }

        size_t get_group_count() {
            return groupsVector_.size();
        }

        bool node_lost(node_id_t from) {
            bool changed = false;
            if (!groupsVector_.empty()) {
                for (groupsVectorIterator_t it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
                    //if in this group the lost node is my parent
                    if (it->parent == from) {
                        changed = true;
                        it->parent = radio().id();
                    }
                }
            }
            return changed;
        }

        /* SHOW all the known nodes */
        void present_neighbors() {
            //            if (!groupsVector_.empty()) {
            //                char buffer[1024];
            //                int bytes_written = 0;
            //                bytes_written += sprintf(buffer + bytes_written, "Groups(%x)", radio().id());
            //
            //                for (groupsVectorIterator_t it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
            //                    predicate_t pred = predicate_t(it->data, it->size_);
            //                    bytes_written += sprintf(buffer + bytes_written, "%d-%x-%s|", it->groupId_, it->parent_, pred.c_str());
            //                }
            //
            //                //            for (typename vector_t::iterator cni = cluster_neighbors_.begin(); cni != cluster_neighbors_.end(); ++cni) {
            //
            //                //            }
            //                buffer[bytes_written] = '\0';
            //                debug_->debug("%s", buffer);
            //            }
        }

        semanticHeadVector_t semanticHeadVector_;

    private:
        groupsVector_t groupsVector_;
        int node_type_, lastid_;
        bool isGateway_;
        Semantics_t * semantics_;

        groupsVectorIterator_t lastNotified_;

        Radio * radio_;

        inline Radio & radio() {
            return *radio_;
        }
        Timer * timer_;

        inline Timer & timer() {
            return *timer_;
        }
        Debug * debug_;

        inline Debug & debug() {
            return *debug_;
        }

    };
}
#endif //__FRONTS_IT_H_