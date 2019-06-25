#ifndef RESTBED_XMR_MYSQLCONNECTIONPOOL_H
#define RESTBED_XMR_MYSQLCONNECTIONPOOL_H

//#include "tools.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>

namespace xmreg
{

using namespace mysqlpp;
using namespace std;

#define MYSQL_EXCEPTION_MSG(sql_except) do { \
    cerr << "# ERR: SQLException in " << __FILE__ \
         << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl \
         << "# ERR: " << sql_except.what() \
         << endl; \
} while (false);


class MySqlConnectionPool : public ConnectionPool {
    static MySqlConnectionPool _inst;
public:
    static string url;
    static size_t port;
    static string username;
    static string password;
    static string dbname;

    static MySqlConnectionPool &get() {
        return _inst;
    }

    ~MySqlConnectionPool() {
        clear();
    }

protected:
    mysqlpp::Connection* create()
    {
        auto conn = new Connection(dbname.c_str(), url.c_str(), username.c_str(), password.c_str(), port);
        conn->set_option(new mysqlpp::ReconnectOption(true));
        return conn;
    }

    void destroy(mysqlpp::Connection* cp)
    {
        delete cp;
    }

    unsigned int max_idle_time()
    {
        return 600;
    }
};

}

#endif