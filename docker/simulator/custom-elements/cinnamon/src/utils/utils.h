#ifndef _H_SST_CINNAMON_UTILS
#define _H_SST_CINNAMON_UTILS

#include "sst/core/sst_config.h"
#include <set>
#include <iostream>
#include <sstream>
#include <string>

namespace SST {
namespace Cinnamon {
namespace Utils {

template <typename T>
class DisjointIntervalSet;

template <typename T>
class Interval {


    private:
    SST::Cycle_t _start;
    SST::Cycle_t _end;
    T _val;
    friend class DisjointIntervalSet<T>;

    public:

    Interval(SST::Cycle_t start, SST::Cycle_t end) : _start(start), _end(end)  {
        if(_start >= _end){
            throw std::invalid_argument("Invalid Interval");
        }
    };

    Interval(SST::Cycle_t start, SST::Cycle_t end, T & val) : _start(start), _end(end), _val(val) {
        if(_start >= _end){
            throw std::invalid_argument("Invalid Interval");
        }
    };

    friend inline bool operator< (const Interval<T> & lhs, const Interval<T> & rhs) {
        if(lhs._end < rhs._start){
            return true;
        }
        return false;
    }

    friend inline bool operator== (const Interval<T> & lhs, const Interval<T> & rhs) {
        if(lhs._start < rhs._start){
            if (rhs._start <= lhs._end){
                return true;
            } else {
                return false;
            }
        } else if(rhs._start < lhs._start){
            if (lhs._start <= rhs._end){
                return true;
            } else {
                return false;
            }
        } else if(lhs._start == rhs._start) {
            return true;
        }
        return false;
    }

    friend std::ostream& operator<< (std::ostream& o, const Interval & lhs) {
        o << "[" << lhs._start << "," << lhs._end << "]";
        return o;
    }

    const T & value() const {
        return _val;
    }

    SST::Cycle_t start() const {
        return _start;
    }
    
    SST::Cycle_t end() const {
        return _end;
    }

    std::string getString() const {
        std::stringstream s;
        s << *this;
        return s.str();
    }

    void setValue(const T & val) {
        _val = val;
    }

    void setValue(const T && val) {
        _val = val;
    }


};

// inline bool operator< (const Interval & lhs, const Interval & right) {
//     if(lhs.end <= right.start){
//         return true;
//     }
//     return false;
// }

template <typename T>
class DisjointIntervalSet {
    std::set<Interval<T>> intervals;

    public:

    bool empty() const {
        return intervals.empty();
    }

    void insert(Interval<T> interval){
        intervals.insert(interval);
    }

    bool hasOverlap(const Interval<T> & interval){
       if(intervals.find(interval) != intervals.end() ){
        return true;
       } 
       return false;
    }

    void eraseIntervalsEndingBefore(SST::Cycle_t time){
        auto it = intervals.begin();
        for(;it != intervals.end();){
            if(it->end <= time){
                it = intervals.erase(it);
            } else {
                break;
            }
        }
    }

    std::string prettyPrint() const {
        if(intervals.empty()){
            return "{}";
        }
        auto it = intervals.begin();
        std::stringstream s;
        s << "{ ";
        s << *it;
        it++;
        for(;it != intervals.end(); it++){
            s << ", " << *it;
        }
        s << " }";
        return s.str();
    }

    Interval<T> front() const {
        if(intervals.empty()){
            throw std::invalid_argument("");
        }
        return *intervals.begin();
    }

    void popFront() {
        if(intervals.empty()){
            throw std::invalid_argument("");
        }
        auto it = intervals.begin();
        intervals.erase(it);
    }

};


} // Namespace Utils
} // Namespace Cinnamon
} // Namespace SST



#endif