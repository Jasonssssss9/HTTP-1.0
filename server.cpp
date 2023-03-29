#include "Log.hpp"
#include <iostream>
#include <memory>
#include "HTTPServer.hpp"
#include "Util.hpp"


int main()
{
    HttpServer* ph = new(HttpServer);
    ph->Loop();

    delete ph;

    return 0;
}