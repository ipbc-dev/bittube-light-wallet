#include "MySqlConnectionPool.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>
#include <memory>


namespace xmreg {

MySqlConnectionPool MySqlConnectionPool::_inst;
string MySqlConnectionPool::url;
size_t MySqlConnectionPool::port;
string MySqlConnectionPool::username;
string MySqlConnectionPool::password;
string MySqlConnectionPool::dbname;

}