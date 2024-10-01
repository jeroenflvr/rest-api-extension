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

// #include "nlohmann/json.hpp" 
#include <algorithm> // For std::find_if

#include "helpers.hpp"
#include "extract_filters.hpp"
#include "rest_api_config.hpp"
#include "logger.hpp"
#include "models.hpp"
#include "prepare_query.hpp"

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

// using namespace std;
// using json = nlohmann::json;

namespace duckdb {


    static void PushdownComplexFilter(ClientContext &context, LogicalGet &get, FunctionData *bind_data_p,
                                    vector<unique_ptr<Expression>> &filters) {

        std::cout << "\n## Step 2: Pushdown complex filter ##\n"  << std::endl;
        std::cout << "a way to get the filters/where statements - seems insufficient for our use case" << std::endl;
        logger.LOG_INFO("Step 2: PushdownComplexFilter");
        auto &bind_data = (BindArguments &)*bind_data_p;

        if (!filters.empty()) {
            // std::cout << "Pushing down filter! "  << std::endl;
            logger.LOG_INFO("Pushing down  filter");
        } else {
            // std::cout << "No non-complex filter provided" << std::endl;
            logger.LOG_INFO("No non-complex filter provided");

        }

        for (auto &filter : filters) {
            // std::cout << "Adding filter: " << filter->ToString() << std::endl;
            logger.LOG_INFO("Adding filter: " + filter->ToString());
            bind_data.filters.push_back(filter->Copy());
            // bind_data.filters.push_back(std::move(filter));
        }
        //std::cout << "## Done pushingdown complex filter! ##\n" << std::endl;

        // Clear filters to indicate they have been consumed
        // std::cout << "clearing filters" << std::endl;
        //std::cout << " filters filtered \n" << std::endl;
        for (auto &filter : filters) {
            // std::cout << "Filter: " << filter->ToString() << std::endl;
            logger.LOG_INFO("Filter: " + filter->ToString());
        }
        // filters.clear();
        //std::cout << "Done clearing filters" << std::endl;
    }


    unique_ptr<GlobalTableFunctionState> rest_api_init(ClientContext &context, TableFunctionInitInput &input) {

        std::cout << "\n## Step 3: Initializing Simple Table Function ##" << std::endl;
        std::cout << "Getting predicate: which columns to fetch. Prep AST and IR." << std::endl;
        logger.LOG_INFO("Step 3: Initializing Simple Table Function");
        for (auto &col : input.column_ids) {
            // std::cout << "Column: " << col << std::endl;
            logger.LOG_INFO("Column: " + std::to_string(col));
        }


        optional_ptr<TableFilterSet> filter_set = input.filters;

        if (filter_set) {
            for (auto &filter_pair: filter_set->filters) {
                column_t column_index = filter_pair.first;

                const std::unique_ptr<TableFilter>& filter_ptr = filter_pair.second;

                logger.LOG_INFO("Found filter: " + std::to_string(static_cast<int>(filter_ptr->filter_type)));

            }
        }
        auto current_query = context.GetCurrentQuery();
        std::cout << "init query: " << current_query << std::endl;

        auto &bind_data = input.bind_data->Cast<BindArguments>();
        auto api = bind_data.api;
        std::cout << "API from bind data in INIT: " << api << std::endl;        

        auto query_ir = process_query(api, context, current_query);
        
        auto result = make_uniq<SimpleData>();
        result->offset = 0;
        result->filters = input.filters;
        result->column_ids = input.column_ids;
        result->query_ir = query_ir;
        return std::move(result);
    }
    
    // Bind function to define schema
    static unique_ptr<FunctionData> rest_api_bind(ClientContext &context, TableFunctionBindInput &input, vector<LogicalType> &return_types, vector<string> &names) {
    
        std::cout << "\n## Step 1: Binding Simple Table Function ##" << std::endl;
        std::cout << "Reading config, defining schema, query schema endpoint if needed, binding columns and types" << std::endl;
        logger.LOG_INFO("Reading config, defining schema, query schema endpoint if needed, binding columns and types");
        logger.LOG_INFO("------ START ------");
        logger.LOG_INFO("Binding Simple Table Function");


        auto bind_data = make_uniq<BindArguments>();
        //bind_data->item_name = input.inputs[0].ToString(); // first positional argument for api or named_parameter instead?
        bind_data->filters = vector<unique_ptr<Expression>>();

        for (auto &kv : input.named_parameters) {
            auto loption = StringUtil::Lower(kv.first);
            logger.LOG_INFO("Found named parameter: " + loption + " = " + kv.second.ToString());
            // std::cout << "\tFound named parameter: " << loption << " = " << kv.second.ToString() << std::endl;
            if (loption == "options") {
                std::string json_str = kv.second.GetValue<std::string>();
                if (!json_str.empty()) {
                    bind_data->options = rest_api_config::ParseOptionsFromJSON(json_str);
                    for (auto &option : bind_data->options) {
                        logger.LOG_INFO("Found option: " + option.first + " = " + option.second);
                        // std::cout << "\t\tOption: " << option.first << " = " << option.second << std::endl;
                    }
                }
            }

            if (loption == "api") {
                bind_data->api = kv.second.GetValue<std::string>();
                logger.LOG_INFO("API: " + bind_data->api);
                // std::cout << "\tAPI: " << bind_data->api << std::endl;
            }
        }


        // std::cout << "\n\tExamining TableFunctionBindInput:" << std::endl;
        logger.LOG_INFO("Examining TableFunctionBindInput");

        // Print named_parameters
        // std::cout << "\tnamed_parameters:" << std::endl;
        logger.LOG_INFO("named_parameters");
        if (!input.named_parameters.empty()) {
            for (const auto& param : input.named_parameters) {
                logger.LOG_INFO("  " + param.first + ": " + param.second.ToString());
                // std::cout << "\t\t  " << param.first << ": " << param.second.ToString() << std::endl;
            }
        } else {
            logger.LOG_INFO("  (empty)");
            // std::cout << "  (empty)" << std::endl;
        }

        // std::cout << std::endl;

        // Ensure there is at least one argument
        // if (input.inputs.size() < 1) {
        //     throw std::runtime_error("Expected at least one argument");
        // }
        // 
        auto config_file = GetRestApiConfigFile(context);
        logger.LOG_INFO("Config file: " + config_file);

        auto &api = bind_data->api;
        auto config = rest_api_config::load_config(config_file, api);


        std:string api_url = "https://" + config.host + ":" + std::to_string(config.port) + "/" + config.root_uri + "/" + config.endpoints.schema.uri;
        logger.LOG_INFO("API URL: " + api_url);


        WebRequest request = WebRequest(api_url, "GET");
        std::string response_body = request.queryAPI();

        logger.LOG_INFO("Response Body: " + response_body);
       

        ApiSchema apiSchema = parseJson(response_body);

        bind_data->columns = apiSchema.parameters.columns;
        bind_data->rowcount = apiSchema.objects;

        logger.LOG_INFO("Schema from API");
        for (auto &column : apiSchema.parameters.columns) {
            logger.LOG_INFO(column.name + ": " + column.type.ToString());
            names.push_back(column.name);
            return_types.push_back(column.type);
        }

        return std::move(bind_data); 
    }
    


    static void rest_api_table_function(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
        std::cout << "\n## Step 4: Executing Simple Table Function ##\n" << std::endl;
        logger.LOG_INFO("Executing Table Function");
       
        /**
         * projection_pushdown: 
         *  column_names are in bind_data.columns
         *  selected columns (idx) are in data_p.column_ids;
         *  only return / process (/ fetch) the data in column_ids
         */

        auto config_file = GetRestApiConfigFile(context);


        auto &data_p = data.global_state->Cast<SimpleData>();

        auto query_ir = data_p.query_ir;

        // check if we have everything we need
        std::cout << "Query IR LIMIT in table function: " << query_ir.limit << std::endl;
        for (auto &o : query_ir.order_by) {
            logger.LOG_INFO("Order by: " + o.column);
            std::cout << "\tOrder by column: " << o.column << std::endl;
            std::cout << "\tOrder by ascending: " << o.ascending << std::endl;
        }

        idx_t data_queries = 1;
        if (data_p.offset >= data_queries) {
            return;
        }

        auto &bind_data = (BindArguments &)*data.bind_data;
        auto &filters = data_p.filters;
        auto &column_ids = data_p.column_ids;

        // auto &options = bind_data.options;
        // auto &api = bind_data.api;
        // auto cfg = rest_api_config::load_config(config_file, api);

        // auto cfg = query_ir.cfg;

        auto config = query_ir.config;


        std:string api_url = "https://" + config.host + ":" + std::to_string(config.port) + "/" + config.root_uri + "/" + config.endpoints.data.uri;
        logger.LOG_INFO("API URL: " + api_url);

        WebRequest request = WebRequest(api_url);

        std::string response_body = request.queryAPI();

        // Create a temporary file to store JSON data
        // char tmp_file_path[] = "./tmp/json_data_XXXXXX.json";
        // int fd = mkstemps(tmp_file_path, 5); // 5 characters for ".json" extension
        // if (fd == -1) {
        //     throw std::runtime_error("Failed to create temporary file.");
        // }
        // std::ofstream tmp_file(tmp_file_path);
        // if (!tmp_file) {
        //     throw std::runtime_error("Failed to open temporary file for writing.");
        // }
        // tmp_file << response_body;
        // tmp_file.close();


        json jsonData = nlohmann::json::parse(response_body);

        auto columns = bind_data.columns;

        size_t row_idx = 0;

        output.SetCardinality(jsonData.size());

        

        for (const auto& obj : jsonData) {

            size_t col_idx = 0;

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
                    logger.LOG_ERROR("Unknown JSON Type: " + c.json_type);
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
        logger.LOG_INFO("Loading RestApi extension");
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
        logger.LOG_INFO("i88888 Loading RestApi extension");
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
