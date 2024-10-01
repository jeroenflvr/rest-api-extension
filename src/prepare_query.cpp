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
#include "rest_api_config.hpp"


// using namespace std;
// using namespace duckdb;


namespace duckdb {
    
    QueryIR process_query(std::string &api_name, ClientContext &context, const std::string& query_string) {
        std::cout << "Processing query: " << query_string << std::endl;
        std::cout << "API name: " << api_name << std::endl;

        std::string current_query = query_string.c_str();

        QueryIR query_ir;

        if (helpers::startsWithCaseInsensitive(current_query, "CREATE OR REPLACE TABLE")){
            logger.LOG_INFO("removing CREATE OR REPLACE TABLE statement from query");
            helpers::removeBeforeSelect(current_query);
        }

        query_ir.query = current_query;

        auto config_file = GetRestApiConfigFile(context);
        auto config = rest_api_config::load_config(config_file, api_name);

        
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

        auto s_count = p.statements.size();

        logger.LOG_INFO("Number of statements: " + std::to_string(s_count));
        
        auto &select = p.statements[0]->Cast<SelectStatement>();
        
        auto &select_node = select.node->Cast<SelectNode>();

        auto select_list = std::move(select_node.select_list);

        vector<string> select_column_names;

        for (auto &column : select_list) {
            logger.LOG_INFO("pushing column: " + column.get()->GetName());
            select_column_names.push_back(column.get()->GetName());
            query_ir.columns.emplace_back(column.get()->GetName());
        }

        if (helpers::contains(select_column_names, "*")) {
            logger.LOG_INFO("SELECT '*' found. OK if number of columns is less than api limit, else throw error!");
        }

        if (!select_node.modifiers.empty()) {
            
            
            logger.LOG_INFO("Modifiers found!!");
            logger.LOG_INFO("Number of modifiers: " + std::to_string(select_node.modifiers.size()));

            for (auto &modifier : select_node.modifiers) {

                RAOrderBy order_by;

                if(modifier->type == ResultModifierType::ORDER_MODIFIER) {
                    auto &order = modifier->Cast<OrderModifier>();
                    for (auto &o : order.orders) {
                        logger.LOG_INFO("order column: " + o.expression.get()->GetName());
                        order_by.column = o.expression.get()->GetName();
                        switch (o.type) {
                            case OrderType::ASCENDING:
                                logger.LOG_INFO("order type: ASCENDING");
                                order_by.ascending = true;
                                break;
                            case OrderType::DESCENDING:
                                order_by.ascending = false;
                                logger.LOG_INFO("order type: DESCENDING");
                                break;                      
                            case OrderType::INVALID:
                                order_by.ascending = true; // Default to ascending if invalid type is found. 
                                logger.LOG_ERROR("order type: INVALID");
                                break;
                            default:
                                order_by.ascending = true; // Default to ascending if unknown type is found. 
                                logger.LOG_INFO("order type: UNKNOWN, so using ASCENDING");
                                break;
                        }

                         query_ir.order_by.emplace_back(order_by);

                    }                    
                }

                if (modifier->type == ResultModifierType::LIMIT_MODIFIER ) {
                    auto &limit = modifier->Cast<LimitModifier>();
                    logger.LOG_INFO("LIMIT found!!");
                    logger.LOG_INFO("Limit: " + limit.limit.get()->GetName());
                    query_ir.limit = std::stoul(limit.limit.get()->GetName());
                }

                if (modifier->type == ResultModifierType::DISTINCT_MODIFIER ) {
                    auto &distinct = modifier->Cast<DistinctModifier>();

                    logger.LOG_INFO("DISTINCT found!!");
                    auto &targets = distinct.distinct_on_targets;
                    logger.LOG_INFO("Number of distinct targets: " + std::to_string(targets.size()));
                    for (auto &target : targets) {
                        logger.LOG_INFO("distinct target: " + target.get()->GetName());
                        query_ir.distinct.emplace_back(target.get()->GetName());
                    }
                }
            }

        }

        logger.LOG_INFO("Number of statements: " + std::to_string(p.statements.size()));


        query_ir.config = config;

        return query_ir;
    }    
}