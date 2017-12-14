#pragma once 

#include <boost/format.hpp>
#include <iostream>

#include "csound.hpp"
#include "csound.h"

using namespace std;
using boost::format;

namespace mt{

    string op_setting( int sr, int ksmps, int nchnls, float zdbfs){
        string setting;
        setting += "sr     = " + to_string(sr)     + "\n";
        setting += "ksmps  = " + to_string(ksmps)  + "\n";
        setting += "nchnls = " + to_string(nchnls) + "\n";
        setting += "0dbfs  = " + to_string(zdbfs)  + "\n";        
        return setting;
    }
    
    
    //      variadic template for orchestra line
    //
    //      usage : mt::op_sco( "i1", 0, 1, 2, 4);
    //
    string op_sco(boost::format& fmt) {
        return fmt.str() + "\n";
    }
    
    template<typename TPrm, typename... TPrms>
    string op_sco(boost::format& fmt, TPrm prm, TPrms... prms) {
        fmt % prm;
        return op_sco(fmt, prms...);
    }
    
    template<typename... TPrms>
    string op_sco( TPrms... prms) {
        int nPrm = sizeof...(prms);
        string fmtstr;
        for( int i=0; i<nPrm; i++)
            fmtstr += "%" + to_string( i+1 ) + "%" + " ";
        
        boost::format fmt(fmtstr);
        return op_sco(fmt, prms...);
    }
    
    
    //      variadic template for orchestra line
    //
    //      usage : mt::op_orc( "aMixL", "reson", 1, 2, 3, 4);
    //
    string op_orc(boost::format& fmt) {
        string orc_fin = fmt.str();
        orc_fin = orc_fin.erase( orc_fin.size()-2, orc_fin.size()-1) + "\n";
        return orc_fin;
    }
    
    template<typename TPrm, typename... TPrms>
    string op_orc(boost::format& fmt, TPrm prm, TPrms... prms) {
        fmt % prm;
        return op_orc(fmt, prms...);
    }

    template<typename... TPrms>
    string op_orc( string op_out, string op_process, TPrms... prms) {
        int nPrm = sizeof...(prms);
        string fmtstr;
        for( int i=0; i<nPrm; i++)
            fmtstr += "%" + to_string( i+1 ) + "%" + ", ";
        
        boost::format fmt(fmtstr);
        return op_out + " " + op_process + " " +op_orc(fmt, prms...);
    }

}
