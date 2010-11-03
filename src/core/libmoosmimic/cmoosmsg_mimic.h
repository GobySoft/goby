// copyright 2010 t. schneider tes@mit.edu
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#ifndef CMOOSMSGMIMIC20101026H
#define CMOOSMSGMIMIC20101026H

#include <boost/lexical_cast.hpp>

#include "goby/core/proto/cmoosmsg_mimic.pb.h"

namespace goby
{
    namespace core
    {
        const int MOOS_SKEW_TOLERANCE = 5;

        class CMOOSMsg
        {
          public:
            CMOOSMsg() {}
          CMOOSMsg(const proto::CMOOSMsgBase& heart) : heart_(heart)
            { }
            
            
            CMOOSMsg(char cMsgType,const std::string &sKey,double dfVal,double dfTime=-1)
            {
                heart_.set_msg_type(cMsgType);
                heart_.set_key(sKey);
                heart_.set_d_val(dfVal);
                heart_.set_time(dfTime);
            }
            
            CMOOSMsg(char cMsgType,const std::string &sKey,const std::string & sVal,double dfTime=-1)
            {
                heart_.set_msg_type(cMsgType);
                heart_.set_key(sKey);
                heart_.set_s_val(sVal);
                heart_.set_time(dfTime);
            }
            

            bool IsDataType (char cDataType)const
            {
                return cDataType == heart_.data_type();
            }
            
            bool IsDouble()const{return IsDataType('D');}
            bool IsString()const{return IsDataType('S');}    

            bool IsSkewed(double dfTimeNow, double * pdfSkew = NULL)
            {
                double dfSkew = fabs(dfTimeNow - heart_.time());
                if(pdfSkew != NULL)
                {
                    *pdfSkew = dfSkew;
                }
                return (dfSkew > MOOS_SKEW_TOLERANCE) ? true : false;   
            }


            bool IsYoungerThan(double dfAge)const
            {
                return heart_.time() >= dfAge;   
            }
            

            bool IsType (char  cType)const
            {
                return cType == heart_.msg_type();
            }
            
            
            double GetTime()const {return heart_.time();};
            double GetDouble()const {return heart_.d_val();};

            std::string GetString()const {return heart_.s_val();};
            std::string GetKey()const {return heart_.key();};
            std::string GetName()const{return GetKey();};

            std::string GetSource()const {return heart_.src();};
            std::string GetSourceAux()const {return heart_.src_aux();};
            void SetSourceAux(const std::string & sSrcAux){heart_.set_src_aux(sSrcAux);}
            
            std::string GetCommunity()const {return heart_.originating_community();};

            std::string GetAsString(int nFieldWidth=12, int nNumDP=5)
            {
                if(IsDataType('D'))
                    return boost::lexical_cast<std::string>(heart_.d_val());
                else
                    return heart_.s_val();
            }

            // print summary
            void Trace()
            {}
            
            void SetDouble(double dfD){ heart_.set_d_val(dfD);}

            const proto::CMOOSMsgBase& heart() const { return heart_; }
            proto::CMOOSMsgBase* mutable_heart() { return &heart_; }            
            
          private:
            proto::CMOOSMsgBase heart_;
            
        };
    }
}

inline std::ostream& operator<<(std::ostream& os, const goby::core::CMOOSMsg& msg)
{
    return (os << msg.heart());
}




    /* /\**standard construction destruction*\/ */
    /* CMOOSMsg(); */
    /* virtual ~CMOOSMsg(); */

    /* /\** specialised construction*\/ */
    /* CMOOSMsg(char cMsgType,const std::string &sKey,double dfVal,double dfTime=-1); */

    /* /\** specialised construction*\/ */
    /* CMOOSMsg(char cMsgType,const std::string &sKey,const std::string & sVal,double dfTime=-1); */


    /* /\**check data type (MOOS_STRING or MOOS_DOUBLE) *\/ */
    /* bool IsDataType (char cDataType)const; */

    /* /\**check data type is double*\/ */
    /* bool IsDouble()const{return IsDataType(MOOS_DOUBLE);} */

    /* /\**check data type is string*\/ */
    /* bool IsString()const{return IsDataType(MOOS_STRING);}     */

    /* /\**return true if mesage is substantially (SKEW_TOLERANCE) older than dfTimeNow */
    /*    if pdfSkew is not NULL, the time skew is returned in *pdfSkew*\/ */
    /* bool IsSkewed(double dfTimeNow, double * pdfSkew = NULL); */

    /* /\**return true if message is younger that dfAge*\/ */
    /* bool IsYoungerThan(double dfAge)const; */

    /* /\**check message type MOOS_NOTIFY, REGISTER etc*\/ */
    /* bool IsType (char  cType)const; */

    /* /\**return time stamp of message*\/ */
    /* double GetTime()const {return m_dfTime;}; */

    /* /\**return double val of message*\/ */
    /* double GetDouble()const {return m_dfVal;}; */

    /* /\**return string value of message*\/ */
    /* std::string GetString()const {return m_sVal;}; */

    /* /\**return the name of the message*\/ */
    /* std::string GetKey()const {return m_sKey;}; */
    /* std::string GetName()const{return GetKey();}; */

    /* /\**return the name of the process (as registered with the DB) which */
    /* posted this notification*\/ */
    /* std::string GetSource()const {return m_sSrc;}; */
    /* std::string GetSourceAux()const {return m_sSrcAux;}; */
    /*     void SetSourceAux(const std::string & sSrcAux){m_sSrcAux = sSrcAux;} */

    /* /\**return the name of the MOOS community in which the orginator lives*\/ */
    /* std::string GetCommunity()const {return m_sOriginatingCommunity;}; */

    /* /\**format the message as string regardless of type*\/ */
    /* std::string GetAsString(int nFieldWidth=12, int nNumDP=5); */

    /* /\**print a summary of the message*\/ */
    /* void Trace(); */

    /* /\** set the Double value *\/ */
    /* void SetDouble(double dfD){m_dfVal = dfD;} */

    /* /\**what type of message is this? Notification,Command,Register etc*\/ */
    /* char m_cMsgType; */
    
    /* /\**what kind of data is this? String,Double,Array?*\/ */
    /* char m_cDataType; */
    
    /* /\**what is the variable name?*\/ */
    /* std::string m_sKey; */
    
    /* /\**ID of message*\/ */
    /* int m_nID; */

    /* /\** double precision time stamp (UNIX time)*\/ */
    /* double m_dfTime; */
    
    /* //DATA VARIABLES */
    
    /* //a) numeric */
    /* double m_dfVal; */
    /* double m_dfVal2; */
    
    /* //b) string */
    /* std::string m_sVal; */

    /* //who sent this message? */
    /* std::string m_sSrc; */
	
    /*     //extra info on source (optional payload) */
    /*     std::string m_sSrcAux; */

    /* //what community did it originate in? */
    /* std::string m_sOriginatingCommunity; */

    /* //serialise this message into/outof a character buffer */
    /* int Serialize(unsigned char *  pBuffer,int  nLen,bool bToStream=true); */

    /* //comparsion operator for sorting and storing */
    /* bool operator <(const CMOOSMsg & Msg) const{ return m_dfTime<Msg.m_dfTime;}; */

#endif
