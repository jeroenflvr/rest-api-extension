#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include "nlohmann/json.hpp" 

#include "rest_api_config.hpp"

using json = nlohmann::json;



namespace duckdb {

    struct ColumnType {
        std::string name;
        LogicalType type;
        std::string json_type;
    };

    struct Schema {
        std::vector<ColumnType> columns;
    };

    struct ApiSchema {
        int objects;
        std::string name;
        std::string access;
        Schema parameters;
    };



    struct RACondition {
        std::string left;
        std::string right;
        std::string the_operator;
    };

    struct RAOrderBy {
        std::string column;
        bool ascending;
    };

    struct QueryIR {
        std::string query;
        std::vector<RACondition> conditions;
        std::vector<std::string> columns;
        std::vector<RAOrderBy> order_by;
        size_t limit;
        std::vector<std::string> distinct;
        rest_api_config::Config config;
        // std::vector<rest_api_config::ConfigItem> cfg;
        
    };

    LogicalType JsonToDuckDBType(const std::string& type);

    ApiSchema parseJson(const std::string& jsonString);
    struct SimpleData : public GlobalTableFunctionState {
        SimpleData() : offset(0) {
        }
        idx_t offset;
        optional_ptr<TableFilterSet> filters;
        vector<column_t> column_ids;    
        QueryIR query_ir;
        size_t last_rowcount;
        idx_t page_idx;

    };

    struct BindArguments : public TableFunctionData {
        string item_name;
        vector<unique_ptr<Expression>> filters;
        vector<std::pair<string, string>> options;
        string api;
        std::vector<ColumnType> columns;
        int rowcount;
        // idx_t estimated_cardinality; // Optional, used for cardinality estimation in statistics
    };

    std::string GetRestApiConfigFile(ClientContext &context);



}