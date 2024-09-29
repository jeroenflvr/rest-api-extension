#include "duckdb.hpp"
#include <string>
#include <iostream>
#include "models.hpp"

namespace duckdb {
   QueryIR  process_query(ClientContext &context, const std::string& query_string);    
}