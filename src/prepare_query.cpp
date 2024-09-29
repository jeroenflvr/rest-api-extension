#include "duckdb.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include "models.hpp"
#include "prepare_query.hpp"


namespace duckdb {
    
    QueryIR process_query(ClientContext &context, const std::string& query_string) {
        std::cout << "Processing query: " << query_string << std::endl;
        QueryIR query_ir;
        return query_ir;
    }    
}