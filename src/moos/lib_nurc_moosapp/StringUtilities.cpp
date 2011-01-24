#include <iomanip>

#include "StringUtilities.h"

std::string
StringUtilities::pluralS
   (const int value)
{
  std::string result ("");

  if (value != 1) {
    result = "s";
  }
  return result;
}

std::string
StringUtilities::intToString
   (const int value)
{
  std::ostringstream outputStream;

  outputStream << value;
  return outputStream.str ();
}

std::string
StringUtilities::intToString
   (const int value,
    const int minimumWidth)
throw (std::invalid_argument)
{
  std::ostringstream outputStream;

  outputStream.width (minimumWidth);
  outputStream << value;
  return outputStream.str ();
}

std::string
StringUtilities::intToString
   (const int value,
    const int minimumWidth,
    const int radix)
throw (std::invalid_argument)
{
  std::ostringstream outputStream;

  outputStream.width (minimumWidth);
  outputStream.fill ('0');

  if (radix == 8) {
    outputStream.setf (std::ios::oct, std::ios::basefield);
  }
  else if (radix == 10) {
    outputStream.setf (std::ios::dec, std::ios::basefield);
  }
  else if (radix == 16) {
    outputStream.setf (std::ios::hex, std::ios::basefield);
  }
  else {
    throw std::invalid_argument
       ("radix " + intToString (radix) + " not supported");
  }
  outputStream << value;
  return outputStream.str ();
}

std::string
StringUtilities::floatToString
   (const float value)
{
  std::ostringstream outputStream;

  outputStream << value;
  return outputStream.str ();
}

std::string
StringUtilities::doubleToString
   (const double value)
{
  std::ostringstream outputStream;

  outputStream << value;
  return outputStream.str ();
}

std::string StringUtilities::timeToString (const double time)
{
  std::ostringstream outputStream;
  const time_t rawtime = (time_t) time;

  outputStream << asctime (localtime (&rawtime));
  return outputStream.str ();
}

int
StringUtilities::stringToInt
   (const std::string& value)
throw (std::invalid_argument)
{
  std::istringstream inputStream (value);
  int result;

  inputStream >> result;

  if (inputStream.fail ()) {
    throw std::invalid_argument ("cannot convert to an int (" + value + ")");
  }
  return result;
}

float
StringUtilities::stringToFloat
   (const std::string& value)
throw (std::invalid_argument)
{
  std::istringstream inputStream (value);
  float result;

  inputStream >> result;

  if (inputStream.fail ()) {
    throw std::invalid_argument ("cannot convert to a float (" + value + ")");
  }
  return result;
}

double
StringUtilities::stringToDouble
   (const std::string& value)
throw (std::invalid_argument)
{
  std::istringstream inputStream (value);
  double result;

  inputStream >> result;

  if (inputStream.fail ()) {
    throw std::invalid_argument ("cannot convert to a double (" + value + ")");
  }
  return result;
}
