#define DUCKDB_EXTENSION_MAIN
#include <algorithm>
#include <cctype>
#include "rest_api_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include <api_request.hpp>
#include "duckdb/planner/operator/logical_filter.hpp"
#include "duckdb/common/types/value.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/planner/filter/conjunction_filter.hpp"
#include "duckdb/planner/filter/constant_filter.hpp"
#include "duckdb/planner/operator/logical_get.hpp"
#include "duckdb/planner/expression/bound_constant_expression.hpp"
#include "duckdb/planner/expression.hpp"
#include "duckdb/planner/expression/bound_reference_expression.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/common/enum_util.hpp"
#include "duckdb/parser/query_node/select_node.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"
#include "postgres_parser.hpp"

#include "duckdb/parser/parsed_data/create_view_info.hpp"
#include "duckdb/parser/statement/select_statement.hpp"
#include "duckdb/parser/statement/create_statement.hpp"

#define _GNU_SOURCE // Ensure this is defined before including unistd.h for mkstemps
#include <unistd.h> // For mkstemps
#include <cstdio>     // For std::remove
#include <fstream>    // For std::ofstream

#include "nlohmann/json.hpp" 
#include <algorithm> // For std::find_if

#include "helpers.hpp"
#include "extract_filters.hpp"
#include "rest_api_config.hpp"


// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

// using namespace std;
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


    LogicalType JsonToDuckDBType(const std::string& type) {
        if (type == "number") {
            std::cout << "Converting JSON number to DuckDB DOUBLE\n";
            return LogicalType::DOUBLE;  // Can be INTEGER if preferred
        } else if (type == "integer") {
            return LogicalType::INTEGER;
        } else if (type == "string") {
            return LogicalType::VARCHAR;
        } else if (type == "boolean") {
            return LogicalType::BOOLEAN;
        } else if (type == "array") {
            return LogicalType::LIST(LogicalType::VARCHAR);
        }

        return LogicalType::UNKNOWN;
    }


    ApiSchema parseJson(const std::string& jsonString) {
        ApiSchema apiSchema;
        try {
            json jsonObj = json::parse(jsonString);
            
            apiSchema.objects = jsonObj.at("objects").get<int>();
            apiSchema.name = jsonObj.at("name").get<std::string>();
            apiSchema.access = jsonObj.at("access").get<std::string>();

            for (const auto& param : jsonObj.at("parameters")) {
                ColumnType p;
                p.name = param.at("name").get<std::string>();
                p.type = JsonToDuckDBType(param.at("type").get<std::string>());
                p.json_type = param.at("type").get<std::string>(); 
                apiSchema.parameters.columns.push_back(p);  
            }
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << "\n";
        }
        
        return apiSchema;
    }

    ConfigItem* findConfigByName(ConfigList& configList, const std::string& name) {

        auto it = std::find_if(configList.begin(), configList.end(),
            [&name](const ConfigItem& item) {
                return item.name == name;
            });

        if (it != configList.end()) {
            return  &(*it); // this is not a pointer, it's a reference
            // return &it->config; // Return a pointer to the config if found
        } else {
            return nullptr; 
        }
    }


    struct SimpleData : public GlobalTableFunctionState {
        SimpleData() : offset(0) {
        }
        idx_t offset;
        optional_ptr<TableFilterSet> filters;
        vector<column_t> column_ids;    
    };


    std::string GetRestApiConfigFile(ClientContext &context) {

        std::string name = "rest_api_config_file";

		Value config_file;
		if (!context.TryGetCurrentSetting(name, config_file) ) {
			throw InvalidInputException("Need the parameters damnit");
		}
        std::cout << "Option value from config: " << config_file.ToString() << std::endl;
        return config_file.ToString();
        
    }


    struct BindArguments : public TableFunctionData {
        string item_name;
        vector<unique_ptr<Expression>> filters;
        vector<std::pair<string, string>> options;
        string api;
        std::vector<ColumnType> columns;
        int rowcount;
        // idx_t estimated_cardinality; // Optional, used for cardinality estimation in statistics
    };


    static void PushdownComplexFilter(ClientContext &context, LogicalGet &get, FunctionData *bind_data_p,
                                    vector<unique_ptr<Expression>> &filters) {

        std::cout << "\n## Pushdown complex filter! ##\n"  << std::endl;
        auto &bind_data = (BindArguments &)*bind_data_p;

        if (!filters.empty()) {
            std::cout << "Pushing down non-complex filter! "  << std::endl;
        } else {
            std::cout << "No non-complex filter provided" << std::endl;
        }

        for (auto &filter : filters) {
            std::cout << "Adding filter: " << filter->ToString() << std::endl;
            bind_data.filters.push_back(filter->Copy());
            // bind_data.filters.push_back(std::move(filter));
        }
        std::cout << "## Done pushingdown complex filter! ##\n" << std::endl;

        // Clear filters to indicate they have been consumed
        std::cout << "clearing filters" << std::endl;
        std::cout << " filters filtered \n" << std::endl;
        for (auto &filter : filters) {
            std::cout << "Filter: " << filter->ToString() << std::endl;
        }
        // filters.clear();
        std::cout << "Done clearing filters" << std::endl;
    }


    unique_ptr<GlobalTableFunctionState> rest_api_init(ClientContext &context, TableFunctionInitInput &input) {

        std::cout << "\n## Initializing Simple Table Function ##\n" << std::endl;
        for (auto &col : input.column_ids) {
            std::cout << "Column: " << col << std::endl;
        }

        for (auto &projection_id : input.projection_ids) {
            std::cout << "Projection ID: " << projection_id << std::endl;
        }

        optional_ptr<TableFilterSet> filter_set = input.filters;

        if (filter_set) {
            for (auto &filter_pair: filter_set->filters) {
                column_t column_index = filter_pair.first;

                const std::unique_ptr<TableFilter>& filter_ptr = filter_pair.second;

                std::cout << "filter: " << static_cast<int>(filter_ptr->filter_type) << std::endl;

            }
        }

        auto result = make_uniq<SimpleData>();
        result->offset = 0;
        result->filters = input.filters;
        result->column_ids = input.column_ids;
        return std::move(result);
    }
    
    // Bind function to define schema
    static unique_ptr<FunctionData> rest_api_bind(ClientContext &context, TableFunctionBindInput &input, vector<LogicalType> &return_types, vector<string> &names) {
    
        std::cout << "\n## Binding Simple Table Function ##\n" << std::endl;

        auto bind_data = make_uniq<BindArguments>();
        //bind_data->item_name = input.inputs[0].ToString(); // first positional argument for api or named_parameter instead?
        bind_data->filters = vector<unique_ptr<Expression>>();
        std::cout << "&&&&&&&&&&&&&&& filters: " << bind_data->filters.size() << std::endl;

        for (auto &kv : input.named_parameters) {
            auto loption = StringUtil::Lower(kv.first);
            std::cout << "Found named parameter: " << loption << " = " << kv.second.ToString() << std::endl;
            if (loption == "options") {
                std::string json_str = kv.second.GetValue<std::string>();
                if (!json_str.empty()) {
                    bind_data->options = rest_api_config::ParseOptionsFromJSON(json_str);
                    for (auto &option : bind_data->options) {
                        std::cout << "Option: " << option.first << " = " << option.second << std::endl;
                    }
                }
            }

            if (loption == "api") {
                bind_data->api = kv.second.GetValue<std::string>();
                std::cout << "API: " << bind_data->api << std::endl;
            }
        }


        std::cout << "Examining TableFunctionBindInput:" << std::endl;

        // Print named_parameters
        std::cout << "named_parameters:" << std::endl;
        if (!input.named_parameters.empty()) {
            for (const auto& param : input.named_parameters) {
                std::cout << "  " << param.first << ": " << param.second.ToString() << std::endl;
            }
        } else {
            std::cout << "  (empty)" << std::endl;
        }

        std::cout << std::endl;

        // Ensure there is at least one argument
        // if (input.inputs.size() < 1) {
        //     throw std::runtime_error("Expected at least one argument");
        // }
        // 
        auto config_file = GetRestApiConfigFile(context);
        std::cout << "Config file: " << config_file << std::endl;

        auto cfg = load_config(config_file);
        auto &api = bind_data->api;
        auto config = findConfigByName(cfg, api) ;

        if (!config) {
            std::cerr << "No configuration found for API: " << api << std::endl;
        } else {
            std::cout << "Using configuration: " << config->name << std::endl;
            std::cout << "host: " << config->config.host << std::endl;
        }

        std:string api_url = "https://" + config->config.host + ":" + std::to_string(config->config.port) + "/" + config->config.root_uri + "/" + config->config.endpoints.schema.uri;

        std::cout << "API URL: " << api_url << std::endl;

        WebRequest request = WebRequest(api_url, "GET");
        std::string response_body = request.queryAPI();

        // std::string response_body = query_api(api_url, "");

        std::cout << "Response Body: " << response_body << std::endl;
       

        ApiSchema apiSchema = parseJson(response_body);

        bind_data->columns = apiSchema.parameters.columns;
        bind_data->rowcount = apiSchema.objects;


        for (auto &column : apiSchema.parameters.columns) {
            std::cout << "Parameter: " << column.name << std::endl;
            std::cout << "Type: " << column.type.ToString() << std::endl;
            names.push_back(column.name);
            return_types.push_back(column.type);
        }

        return std::move(bind_data); 
    }
    


    static void rest_api_table_function(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
        std::cout << "\n## Executing Simple Table Function ##\n" << std::endl;
       
        /**
         * projection_pushdown: 
         *  column_names are in bind_data.columns
         *  selected columns (idx) are in data_p.column_ids;
         *  only return / process (/ fetch) the data in column_ids
         */

        auto config_file = GetRestApiConfigFile(context);

        auto cfg = load_config(config_file);

        auto &data_p = data.global_state->Cast<SimpleData>();


        idx_t data_queries = 1;
        if (data_p.offset >= data_queries) {
            return;
        }

        auto &bind_data = (BindArguments &)*data.bind_data;
        auto &filters = data_p.filters;
        auto &column_ids = data_p.column_ids;

        auto &options = bind_data.options;
        auto &api = bind_data.api;

        if (!options.empty()) {
            for (auto &option : options) {
                std::cout << "  " << option.first << " = " << option.second << std::endl;
            }
        }

        if (api.empty()) {
            std::cerr << "No API provided. Skipping rest call." << std::endl;
            return;
        } else {
            std::cout << "querying API: " << api << std::endl;
        }

        auto current_query = context.GetCurrentQuery();
        std::cout << "Query: " << current_query.c_str() << std::endl;

        if (helpers::startsWithCaseInsensitive(current_query, "CREATE OR REPLACE TABLE")){
            std::cout << "removing CREATE OR REPLACE TABLE statement from query" << std::endl;
            helpers::removeBeforeSelect(current_query);
        }
        
        std::cout << "Updated query: " << current_query.c_str() << std::endl;

        Parser p;
	    p.ParseQuery(current_query);

        auto s = CreateViewInfo::ParseSelect(current_query);



        std::cout << "Select statement: " << s->ToString() << std::endl;
        auto select_statement = static_cast<duckdb::SelectStatement*>(s.get());
        auto where = select_statement->node.get();
        // std::cout << "Where clause: " << where->cte_map << std::endl; 


        if (select_statement->type == duckdb::StatementType::SELECT_STATEMENT){
            std::cout << "SELECT statement" << std::endl;
            auto &select = select_statement->node->Cast<SelectNode>();

            std::cout << "Where: " << where->ToString() << std::endl;
            // The WHERE clause is typically part of the SELECT node, not a direct member of SelectStatement.
            // Cast the query node to SelectNode to access the WHERE clause
            if (select_statement->node->type == duckdb::QueryNodeType::SELECT_NODE) {
                auto &select_node = dynamic_cast<duckdb::SelectNode&>(*select_statement->node);
                std::cout << "Extracting WHERE clause filters from SELECT node:" << std::endl;
                // Extract filters from the WHERE clause if present
                if (select_node.where_clause) {
                    std::cout << "Extracting WHERE clause filters:" << std::endl;
                    ExtractFilters(*select_node.where_clause);
                }
            }

        }

        // auto tokens = p.Tokenize(current_query);
        // std::cout << "Number of tokens: " << tokens.size() << std::endl;


        auto s_count = p.statements.size();

        std::cout << "Number of statements: " << s_count << std::endl;
        

        auto &select = p.statements[0]->Cast<SelectStatement>();
        
        auto &select_node = select.node->Cast<SelectNode>();

        auto select_list = std::move(select_node.select_list);

        vector<string> select_column_names;

        for (auto &column : select_list) {
            std::cout << "column: " << column.get()->GetName() << std::endl;
            select_column_names.push_back(column.get()->GetName());
            // std::cout << "type: " << column.type.ToString() << std::endl;
        }

        if (helpers::contains(select_column_names, "count_star()")) {
            std::cout << "count_star() found. Returning count of rows." << std::endl;
            output.SetCardinality(1);
            output.SetValue(0, 0, 1);
            data_p.offset++;
            return;
        }

        if (helpers::contains(select_column_names, "*")) {
            std::cout << "SELECT '*' found. OK if number of columns is less than api limit, else throw error!" << std::endl;
        }

        // auto modifiers = select_node.modifiers;
        if (!select_node.modifiers.empty()) {
            
            // && select_node.modifiers[0]->type == ResultModifierType::ORDER_MODIFIER &&
            // select_node.modifiers.size() == 1) {
            
            std::cout << "Modifiers found!! " << std::endl;
            std::cout << "Number of modifiers: " << select_node.modifiers.size() << std::endl;

            for (auto &modifier : select_node.modifiers) {

                if(modifier->type == ResultModifierType::ORDER_MODIFIER) {
                    auto &order = modifier->Cast<OrderModifier>();
                    for (auto &o : order.orders) {
                        std::cout << "order column: " << o.expression.get()->GetName() << std::endl;

                        switch (o.type) {
                            case OrderType::ASCENDING:
                                std::cout << "order type: ASCENDING" << std::endl;
                                break;
                            case OrderType::DESCENDING:
                                std::cout << "order type: DESCENDING" << std::endl;  
                                break;                      
                            case OrderType::INVALID:
                                std::cout << "order type: INVALID" << std::endl;
                                break;
                            default:
                                std::cout << "order type: UNKNOWN, so using ASCENDING" << std::endl;
                                break;
                        }

                    }                    
                }


                if (modifier->type == ResultModifierType::LIMIT_MODIFIER ) {
                    auto &limit = modifier->Cast<LimitModifier>();
                    std::cout << "LIMIT found!! " << std::endl;
                    std::cout << "Limit: " << limit.limit.get()->GetName() << std::endl;
                }

                if (modifier->type == ResultModifierType::DISTINCT_MODIFIER ) {
                    auto &distinct = modifier->Cast<DistinctModifier>();

                    std::cout << "DISTINCT found!! " << std::endl;
                    auto &targets = distinct.distinct_on_targets;
                    std::cout << "Number of distinct targets: " << targets.size() << std::endl;
                    for (auto &target : targets) {
                        std::cout << "distinct target: " << target.get()->GetName() << std::endl;
                    }
                }
            }




        }

        std::cout << "Number of statements: " << p.statements.size() << std::endl;

        auto config = findConfigByName(cfg, api) ;

        if (!config) {
            std::cerr << "No configuration found for API: " << api << std::endl;
            return;
        } else {
            std::cout << "Using configuration: " << config->name << std::endl;
            std::cout << "host: " << config->config.host << std::endl;
        }

        std:string api_url = "https://" + config->config.host + ":" + std::to_string(config->config.port) + "/" + config->config.root_uri + "/" + config->config.endpoints.data.uri;

        std::cout << "API URL: " << api_url << std::endl;

        WebRequest request = WebRequest(api_url);

        std::string response_body = request.queryAPI();

        // Create a temporary file to store JSON data
        char tmp_file_path[] = "./tmp/json_data_XXXXXX.json";
        int fd = mkstemps(tmp_file_path, 5); // 5 characters for ".json" extension
        if (fd == -1) {
            throw std::runtime_error("Failed to create temporary file.");
        }
        std::ofstream tmp_file(tmp_file_path);
        if (!tmp_file) {
            throw std::runtime_error("Failed to open temporary file for writing.");
        }
        tmp_file << response_body;
        tmp_file.close();


        json jsonData = nlohmann::json::parse(response_body);

        auto columns = bind_data.columns;

        size_t row_idx = 0;

        output.SetCardinality(jsonData.size());

        

        for (const auto& obj : jsonData) {

            size_t col_idx = 0;

            // for (const auto& c : columns) {
            for (const auto& c_idx : column_ids) {
                auto c = columns[c_idx];
                // std::cout << "Column Name: " << c.name << std::endl;
                // std::cout << "col_idx: " << col_idx << std::endl;
                // std::cout << "column type: " << c.json_type << std::endl;

                if (c.json_type == "number") {
                    // std::cout << "JSON TYpe Number " << std::endl;
                    // std::cout << "Column Name: " << c.name << std::endl;
                    output.SetValue(col_idx, row_idx, obj[c.name].get<double>()  );
                } else if (c.json_type == "string") {
                    // std::cout << "JSON TYpe STRING " << std::endl;
                    // std::cout << "Column Name: " << c.name << std::endl;
                    // output.SetValue(col_idx, row_idx, "testing");
                    output.SetValue(col_idx, row_idx, obj[c.name].get<string>()  );
                } else if (c.json_type == "array") {
                    //std::cout << "JSON Type Array " << std::endl;
                    auto array = obj[c.name].get<std::vector<std::string>>();

                    std::vector<Value> value_list;
                    for (const auto &item : array) {
                        value_list.push_back(Value(item));
                    }

                    Value list_value = Value::LIST(std::move(value_list));
                    output.SetValue(col_idx, row_idx, list_value);

                } else {
                    std::cerr << "Unknown JSON Type: " << c.json_type << std::endl;
                    // output.SetValue(col_idx, row_idx, c);
                }
                // output.SetValue(col_idx, row_idx, c);
                ++col_idx;
            }
            ++row_idx;
        }
        data_p.offset++;
    }

    

    inline void RestApiScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
        auto &name_vector = args.data[0];
        UnaryExecutor::Execute<string_t, string_t>(
            name_vector, result, args.size(),
            [&](string_t name) {
                return StringVector::AddString(result, "RestApi "+name.GetString()+" 🐥");;
            });
    }

    inline void RestApiOpenSSLVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
        auto &name_vector = args.data[0];
        UnaryExecutor::Execute<string_t, string_t>(
            name_vector, result, args.size(),
            [&](string_t name) {
                return StringVector::AddString(result, "RestApi " + name.GetString() +
                                                        ", my linked OpenSSL version is " +
                                                        OPENSSL_VERSION_TEXT );;
            });
    }

    static void LoadInternal(DatabaseInstance &instance) {
        // Register a scalar function
        auto rest_api_scalar_function = ScalarFunction("rest_api", {LogicalType::VARCHAR}, LogicalType::VARCHAR, RestApiScalarFun);
        ExtensionUtil::RegisterFunction(instance, rest_api_scalar_function);

        // Register another scalar function
        auto rest_api_openssl_version_scalar_function = ScalarFunction("rest_api_openssl_version", {LogicalType::VARCHAR},
                                                    LogicalType::VARCHAR, RestApiOpenSSLVersionScalarFun);
        ExtensionUtil::RegisterFunction(instance, rest_api_openssl_version_scalar_function);

        // the table function
        auto rest_api_table_func = TableFunction("query_json_api", {}, rest_api_table_function, rest_api_bind, rest_api_init);
        rest_api_table_func.filter_pushdown = false;
        rest_api_table_func.projection_pushdown = true;    
        // rest_api_table_func.cardinality = simple_cardinality;
        rest_api_table_func.pushdown_complex_filter = PushdownComplexFilter;

        rest_api_table_func.named_parameters["order_by"] = LogicalType::VARCHAR;
        rest_api_table_func.named_parameters["limit"] = LogicalType::VARCHAR;
        rest_api_table_func.named_parameters["columns"] = LogicalType::VARCHAR;
        rest_api_table_func.named_parameters["options"] = LogicalType::VARCHAR;
        rest_api_table_func.named_parameters["api"] = LogicalType::VARCHAR;

        ExtensionUtil::RegisterFunction(instance, rest_api_table_func);
        
        auto &config = DBConfig::GetConfig(instance);

        config.AddExtensionOption("rest_api_config_file", "REST API Config File Location", LogicalType::VARCHAR,
                                Value("/users/thisguy/rest_api_extension.json"));

    }

    void RestApiExtension::Load(DuckDB &db) {
        LoadInternal(*db.instance);
    }
    std::string RestApiExtension::Name() {
        return "rest_api";
    }

    std::string RestApiExtension::Version() const {
    #ifdef EXT_VERSION_REST_API
        return EXT_VERSION_REST_API;
    #else
        return "";
    #endif
    }

    } // namespace duckdb

    extern "C" {

    DUCKDB_EXTENSION_API void rest_api_init(duckdb::DatabaseInstance &db) {
        duckdb::DuckDB db_wrapper(db);
        db_wrapper.LoadExtension<duckdb::RestApiExtension>();
    }

    DUCKDB_EXTENSION_API const char *rest_api_version() {
        return duckdb::DuckDB::LibraryVersion();
    }
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
