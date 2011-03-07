#ifndef NODEREPORTER20101225H
#define NODEREPORTER20101225H

#include "goby/core/core.h"
#include "config.pb.h"

#include "gps_nmea.pb.h"
#include "depth_reading.pb.h"

class NodeReporter : public goby::core::ApplicationBase
{
public:
    NodeReporter();
    ~NodeReporter();
    
    
private:
    void create_node_report(const GPSSentenceGGA& gga,
                            const DepthReading& depth);
    
    void handle_depth(const DepthReading& reading)
    {
        create_node_report(newest<GPSSentenceGGA>(), reading);
    }

    void handle_gps(const GPSSentenceGGA& gga)
    {
        create_node_report(gga, newest<DepthReading>());
    }
    
    static NodeReporterConfig cfg_;    
};

#endif
