#include "duckdb.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include "duckdb/parser/parser.hpp"
#include "duckdb/parser/parsed_data/create_view_info.hpp"
#include "duckdb/parser/statement/select_statement.hpp"
#include "duckdb/parser/statement/create_statement.hpp"
#include "duckdb/parser/query_node/select_node.hpp"

#include "models.hpp"
#include "prepare_query.hpp"
#include "helpers.hpp"
#include "logger.hpp"
#include "extract_filters.hpp"


// using namespace std;
// using namespace duckdb;


namespace duckdb {
    
    QueryIR process_query(std::string &api_name, ClientContext &context, const std::string& query_string) {
        std::cout << "Processing query: " << query_string << std::endl;
        std::cout << "API name: " << api_name << std::endl;

        std::string current_query = query_string.c_str();

        if (helpers::startsWithCaseInsensitive(current_query, "CREATE OR REPLACE TABLE")){
            logger.LOG_INFO("removing CREATE OR REPLACE TABLE statement from query");
            helpers::removeBeforeSelect(current_query);
        }
        
        logger.LOG_INFO("Updated query: " + current_query);
        Parser p;
	    p.ParseQuery(current_query);

        auto s = CreateViewInfo::ParseSelect(current_query);

        logger.LOG_INFO("Select statement: " + s->ToString());
        auto select_statement = static_cast<duckdb::SelectStatement*>(s.get());
        auto where = select_statement->node.get();


        if (select_statement->type == duckdb::StatementType::SELECT_STATEMENT){
            logger.LOG_INFO("SELECT statement");
            auto &select = select_statement->node->Cast<SelectNode>();

            logger.LOG_INFO("Where: " + where->ToString());
            // The WHERE clause is typically part of the SELECT node, not a direct member of SelectStatement.
            // Cast the query node to SelectNode to access the WHERE clause
            if (select_statement->node->type == duckdb::QueryNodeType::SELECT_NODE) {
                auto &select_node = dynamic_cast<duckdb::SelectNode&>(*select_statement->node);
                logger.LOG_INFO("Extracting WHERE clause filters from SELECT node");
                if (select_node.where_clause) {
                    logger.LOG_INFO("Extracting WHERE clause filters");
                    ExtractFilters(*select_node.where_clause);

                }
                
            }

        }



        QueryIR query_ir;
        return query_ir;
    }    
}