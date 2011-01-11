#include "NurcMoosApp.h"
#include "StringUtilities.h"

NurcMoosApp::NurcMoosApp ()
  : _connected (false),
    _startedUp (false),
    _verbose (false),
    _initialisers ()
{
}

NurcMoosApp::~NurcMoosApp ()
{
}

bool NurcMoosApp::OnConnectToServer ()
{
  std::cout << this->m_MissionReader.GetAppName () << ", connected to server." << std::endl;
  this->_connected = true;
  return _registerMoosVariables ();
}

bool NurcMoosApp::OnStartUp ()
{
  std::cout << this->m_MissionReader.GetAppName () << ", starting ..." << std::endl;
  this->m_MissionReader.GetConfigurationParam ("verbose", this->_verbose);
  std::cout << this->m_MissionReader.GetAppName () << ", verbose is " << (this->_verbose ? "on" : "off") << "." << std::endl;
  bool result = _readInitialisers ();

  if (result) {
    result = readMissionParameters (this->m_MissionReader);
  }
  if (result) {
    this->_startedUp = true;
    result = _registerMoosVariables ();
  }
  return result;
}

std::string NurcMoosApp::appName ()
{
  return this->m_MissionReader.GetAppName ();
}

bool NurcMoosApp::verbose ()
{
  return this->_verbose;
}

bool NurcMoosApp::readMissionParameters (CProcessConfigReader& processConfigReader)
{
  return true;
}

bool NurcMoosApp::registerMoosVariables ()
{
  return true;
}

bool NurcMoosApp::_readInitialisers ()
{
  std::list <std::string> configurationParameters;

  this->m_MissionReader.GetConfiguration (this->m_MissionReader.GetAppName (), configurationParameters);
  return _readInitialisers (configurationParameters);
}

bool NurcMoosApp::_readInitialisers (const std::list <std::string>& configurationParameters)
{
  bool result = true;

  //
  // The configuration parameters are in the list in reverse document order,
  // hence the use of a const_reverse_iterator.  I asked Paul Newman whether
  // this feature can be relied upon.
  //
  // There is a bug (?) in some of the STL implementations concerning const_reverse_iterator and operator!=,
  // see e.g. http://bytes.com/groups/cpp/621462-operator-const_reverse_iterator for an explanation.
  // It also says:
  //
  //   Note that the standard library containers in C++0x are expected
  //   to include new member functions: cbegin(), crend() etc that
  //   always return const_ versions of the iterators.
  //
  // A workaround is to make configurationParameters a reference to a const.
  //
  for (std::list <std::string>::const_reverse_iterator iterator = configurationParameters.rbegin ();
       (iterator != configurationParameters.rend ());
       iterator++) {
    std::string token;
    std::string text;

    if (this->m_MissionReader.GetTokenValPair (*iterator, token, text)) {
      if (MOOSStrCmp ("initialiser", token)
          || MOOSStrCmp ("initialiser.string", token)
          || MOOSStrCmp ("initializer", token)
          || MOOSStrCmp ("initializer.string", token)) {
        std::string name;
        std::string value;

        result = _getNameValue (text, name, value);

        if (! result) {
          std::cout << this->m_MissionReader.GetAppName () << ", cannot decode initialiser (" << *iterator << ")." << std::endl;
          break;
        }
        this->_initialisers.push_back (CMOOSMsg ('N' /* Notify */, name, value));
      }
      else if (MOOSStrCmp ("initialiser.double", token)
               || MOOSStrCmp ("initializer.double", token)) {
        std::string name;
        std::string value;

        result = _getNameValue (text, name, value);

        if (! result) {
          std::cout << this->m_MissionReader.GetAppName () << ", cannot decode initialiser (" << *iterator << ")." << std::endl;
          break;
        }

        double doubleValue;

        try {
          doubleValue = StringUtilities::stringToDouble (value);
        }
        catch (std::invalid_argument e) {
          std::cout << this->m_MissionReader.GetAppName () << ", cannot decode initialiser (" << *iterator << "), value (" << value << ") is not a double." << std::endl;
          result = false;
          break;
        }
        this->_initialisers.push_back (CMOOSMsg ('N' /* Notify */, name, doubleValue));
      }
    }
  }
  if (result && verbose ()) {
    std::cout << this->m_MissionReader.GetAppName () << ", initialisers (" << this->_initialisers.size () << ") are " << StringUtilities::vectorToString (this->_initialisers) << "." << std::endl;
  }
  return result;
}

bool NurcMoosApp::_registerMoosVariables ()
{
  bool result = true;

  if (this->_connected && this->_startedUp) {
    result = _postInitialisers ();

    if (result) {
      result = registerMoosVariables ();
    }
  }
  return result;
}

bool NurcMoosApp::_postInitialisers ()
{
  bool result = true;

  // Cannot use a const_iterator, because the call to Post can modify a message (!).
  for (std::vector <CMOOSMsg>::iterator iterator = this->_initialisers.begin ();
       (iterator != this->_initialisers.end ());
       iterator++) {
    result = this->m_Comms.Post (*iterator);

    if (! result) {
      std::cout << this->m_MissionReader.GetAppName () << ", could not post message (" << *iterator << ")." << std::endl;
      break;
    }
  }
  return result;
}

bool NurcMoosApp::_getNameValue (const std::string& text, std::string& name, std::string& value) const
{
  bool result = true;

  value = text;
  name = MOOSChomp (value, "=");

  if (value == "") {
    result = false;
  }
  if (result) {
    MOOSTrimWhiteSpace (name);
    MOOSTrimWhiteSpace (value);
  }
  return result;
}
