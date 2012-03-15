
// std
#include <set>

// third party
#include <boost/bind.hpp>

// local to goby
#include "util/flexcout.h"

// local to folder (part of same application / library)
#include "style_example.h"

// global using declaratives: use sparingly, if at all
// do not use `using namespace std`
using namespace nspace;

bool nspace::Class::push_message(micromodem::Message& new_message)
{
    return true;
}
