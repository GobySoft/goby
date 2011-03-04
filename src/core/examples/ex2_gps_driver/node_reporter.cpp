
#include "node_reporter.h"

#include "node_report.pb.h"

using goby::core::operator<<;

NodeReporterConfig NodeReporter::cfg_;

int main(int argc, char* argv[])
{
    return goby::run<NodeReporter>(argc, argv);
}

NodeReporter::NodeReporter()
    : goby::core::ApplicationBase(&cfg_)
{
    // from Pressure Sensor Simulator
    subscribe<DepthReading>(&NodeReporter::handle_depth, this);

    // from GPS Driver
    subscribe<GPSSentenceGGA>(&NodeReporter::handle_gps, this);
}

NodeReporter::~NodeReporter()
{ }


void NodeReporter::create_node_report(const GPSSentenceGGA& gga,
                                      const DepthReading& depth_reading)
{
    if(!(gga.IsInitialized() && depth_reading.IsInitialized()))
    {
        glogger() << warn << "need both GPSSentenceGGA and DepthReading "
                  << "message to proceed" << std::endl;
        return;
    }

    glogger() << gga << depth_reading << std::flush;
    
    
    // make an abstracted position and pose aggregate from the newest
    // readings for consumption by other processes
    NodeReport report;

    // use the time from the GGA message as the base message time
    report.mutable_header()->set_iso_time(gga.header().iso_time());
    report.set_name(cfg_.base().platform_name());
    report.set_type(global_cfg().self().type());

    GeodeticCoordinate* global_fix = report.mutable_global_fix();
    global_fix->set_lat(gga.lat());
    global_fix->set_lon(gga.lon());

    // we set message time from GPS GGA, so no lag
    global_fix->set_lat_time_lag(0);
    global_fix->set_lon_time_lag(0);
    
    global_fix->set_lat_source(GPS);
    global_fix->set_lon_source(GPS);

    // set the depth sensor data
    global_fix->set_depth(depth_reading.depth());
    global_fix->set_depth_source(SIMULATION);
    global_fix->set_depth_time_lag(gga.header().unix_time()
                                   -depth_reading.header().unix_time());

    // TODO(tes): compute the local coordinates

    // in a better world we would want data for altitude, speed and
    // Euler angles too!
    glogger() << report << std::flush;

    publish(report);
    
}
