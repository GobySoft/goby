#ifndef __StringUtilities_h__
#define __StringUtilities_h__

#include <iterator>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "CMOOSMsgOperator.h"

/**
 * A collection of utility functions related to <code>std::string</code>s.
 */
class StringUtilities
{
  public:

    static std::string pluralS (const int value);
    static std::string intToString (const int value);
    static std::string intToString (const int value, const int minimumWidth)
      throw (std::invalid_argument);
    static std::string
      intToString (const int value, const int minimumWidth, const int radix)
      throw (std::invalid_argument);
    static std::string floatToString (const float value);
    static std::string doubleToString (const double value);
    static std::string timeToString (const double time);

    template <typename T> static std::string listToString (const std::list <T>& value)
    {
      return rangeToString (value.begin (), value.end (), ", ");
    };

    template <typename T> static std::string listToString (const std::list <T>& value, const std::string& separator)
    {
      return rangeToString (value.begin (), value.end (), separator);
    };

    template <typename T> static std::string vectorToString (const std::vector <T>& value)
    {
      return rangeToString (value.begin (), value.end (), ", ");
    };

    template <typename T> static std::string vectorToString (const std::vector <T>& value, const std::string& separator)
    {
      return rangeToString (value.begin (), value.end (), separator);
    }

    template <typename InputIterator> static std::string rangeToString (const InputIterator first, const InputIterator last)
    {
      return rangeToString (first, last, ", ");
    };

    template <typename InputIterator> static std::string rangeToString (const InputIterator first, const InputIterator last, const std::string& separator)
    {
      bool isFirst = true;
      std::ostringstream outputStream;

      for (InputIterator iterator = first; (iterator != last); iterator++) {
        if (isFirst) {
          isFirst = (! isFirst);
        }
        else {
          outputStream << separator;
        }
        outputStream << *iterator;
      }
      return outputStream.str ();
    };

    static int stringToInt (const std::string& value)
      throw (std::invalid_argument);
    static float stringToFloat (const std::string& value)
      throw (std::invalid_argument);
    static double stringToDouble (const std::string& value)
      throw (std::invalid_argument);

  private:

    StringUtilities ();
};

#endif
