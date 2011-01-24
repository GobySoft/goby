#ifndef NurcMoosApp_HEADER
#define NurcMoosApp_HEADER

#include "MOOSLIB/MOOSLib.h"

/**
 * A subclass of CMOOSApp that invites clients to implement two methods,
 * readMissionParameters and registerMoosVariables, rather than OnConnectToServer and OnStartUp.
 * The reason for this is, that the order in which OnConnectToServer and OnStartUp are called, may vary.
 * This class guarantees that first the mission parameters are read, and only then the MOOS variables are registered.
 * Each of the two methods is called exactly once.
 */
class NurcMoosApp : public CMOOSApp
{
public:

  NurcMoosApp ();
  virtual ~NurcMoosApp ();

  virtual bool OnConnectToServer ();
  virtual bool OnStartUp ();

  /**
   * The name of the application as returned by CProcessConfigReader::GetAppName.
   * Useful for logging purposes.
   */
  virtual std::string appName ();

  /**
   * Whether this MOOS application is to be verbose in its output to the terminal, defaults to \c false.
   * Its value is read from the mission file.
   */
  virtual bool verbose ();

protected:

  /**
   * Implement this method to read the parameters from the mission file here.
   * The default implementation does nothing other than return \c true.
   *
   * Would have declared the CProcessConfigReader parameters a const parameter,
   * but the implementation of CProcessConfigReader does not allow for this.
   * It seems to read the entire mission file on every call to its GetValue method.
   *
   * @return \c true if the parameters were succesfully read from the mission file, \c false otherwise.
   * Mind that a return value of \c false will tell the parent class that it should abort.
   */
  virtual bool readMissionParameters (CProcessConfigReader& processConfigReader);

  /**
   * Implement this method to register your MOOS variables here.
   * The default implementation does nothing other than return \c true.
   *
   * @return \c true if the MOOS variables were succesfully registered, \c false otherwise.
   * Mind that a return value of \c false will tell the parent class that it should abort.
   */
  virtual bool registerMoosVariables ();

private:

  /**
   * \c True if \c CMOOSApp::OnConnectToServer has been called, \c false otherwise.
   */
  bool _connected;

  /**
   * \c True if \c CMOOSApp::OnStartUp has been called, \c false otherwise.
   */
  bool _startedUp;

  bool _verbose;
  std::vector <CMOOSMsg> _initialisers;

  bool _readInitialisers ();
  bool _readInitialisers (const std::list <std::string>& configurationParameters);
  bool _registerMoosVariables ();
  bool _postInitialisers ();

  bool _getNameValue (const std::string& text, std::string& name, std::string& value) const;
};

#endif
