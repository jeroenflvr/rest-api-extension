#define DUCKDB_EXTENSION_MAIN

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
// #include "duckdb/planner/expression/bound_limit_expression.hpp"
#include "duckdb/planner/expression.hpp"
#include "duckdb/planner/expression/bound_reference_expression.hpp"
// #include "duckdb/planner/expression/bound_order_expression.hpp"
// #include "duckdb/optimizer/statistics/node_statistics.hpp"
#include "nlohmann/json.hpp" 
#include <algorithm> // For std::find_if





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
        return LogicalType::DOUBLE;  // Can be INTEGER if preferred
    } else if (type == "integer") {
        return LogicalType::INTEGER;
    } else if (type == "string") {
        return LogicalType::VARCHAR;
    } else if (type == "boolean") {
        return LogicalType::BOOLEAN;
    } else if (type == "array") {
        // For simplicity, assume array of VARCHAR. Adjust to your specific use case.
        return LogicalType::LIST(LogicalType::VARCHAR);
    }

    // Default case if none of the above match
    return LogicalType::UNKNOWN;
}


// Function to parse the JSON and return the struct
ApiSchema parseJson(const std::string& jsonString) {
    ApiSchema apiSchema;
    try {
        // Parse the JSON string into a json object
        json jsonObj = json::parse(jsonString);
        
        // Extract the basic fields
        apiSchema.objects = jsonObj.at("objects").get<int>();
        apiSchema.name = jsonObj.at("name").get<std::string>();
        apiSchema.access = jsonObj.at("access").get<std::string>();

        // Extract the parameters array
        for (const auto& param : jsonObj.at("parameters")) {
            ColumnType p;
            p.name = param.at("name").get<std::string>();
            p.type = JsonToDuckDBType(param.at("type").get<std::string>());
            p.json_type = param.at("type").get<std::string>(); 
            apiSchema.parameters.columns.push_back(p);  // Correctly push to the vector
        }
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << "\n";
    }
    
    return apiSchema;
}

ConfigItem* findConfigByName(ConfigList& configList, const std::string& name) {
    // Use std::find_if to find the ConfigItem with the specified name
    auto it = std::find_if(configList.begin(), configList.end(),
        [&name](const ConfigItem& item) {
            return item.name == name;
        });

    // Check if we found the item
    if (it != configList.end()) {
        return  &(*it); // Return a pointer to the config if found
        // return &it->config; // Return a pointer to the config if found
    } else {
        return nullptr; // Return nullptr if not found
    }
}

// Mock static data to return
std::vector<std::vector<Value>> GetStaticTestData() {
    return {
        {Value::INTEGER(1), Value("Alice"), Value::INTEGER(30)},
        {Value::INTEGER(2), Value("Bob"), Value::INTEGER(25)},
        {Value::INTEGER(3), Value("Charlie"), Value::INTEGER(35)}
    };
}

struct SimpleData : public GlobalTableFunctionState {
    SimpleData() : offset(0) {
    }
    idx_t offset;
    optional_ptr<TableFilterSet> filters;
    vector<column_t> column_ids;    
};

// Function to parse JSON string into a vector of key-value pairs
std::vector<std::pair<std::string, std::string>> ParseOptionsFromJSON(const std::string &json_str) {
    std::vector<std::pair<std::string, std::string>> options;

    // Parse the JSON string
    json parsed_json;
    try {
        parsed_json = json::parse(json_str);
    } catch (const json::parse_error &e) {
        throw std::invalid_argument(std::string("Failed to parse JSON options: ") + e.what());
    }

    // Ensure the JSON is an object
    if (!parsed_json.is_object()) {
        throw std::invalid_argument("JSON options must be an object with key-value pairs.");
    }

    // Iterate through the JSON object and populate the options vector
    for (auto it = parsed_json.begin(); it != parsed_json.end(); ++it) {
        if (!it.value().is_string()) {
            throw std::invalid_argument("All values in JSON options must be strings.");
        }
        options.emplace_back(it.key(), it.value().get<std::string>());
    }

    return options;
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


// static unique_ptr<NodeStatistics> simple_cardinality(ClientContext &context, const FunctionData *bind_data_p) {
//     auto &bind_data = (BindArguments &)*bind_data_p;
//     idx_t base_cardinality = 3; // Since our test data has 3 rows
//     // idx_t estimated_cardinality = std::min(base_cardinality, bind_data.limit);

//     return make_uniq<NodeStatistics>(estimated_cardinality, estimated_cardinality);
// }


static void PushdownComplexFilter(ClientContext &context, LogicalGet &get, FunctionData *bind_data_p,
                                  vector<unique_ptr<Expression>> &filters) {
    auto &bind_data = (BindArguments &)*bind_data_p;

    if (!filters.empty()) {
        std::cout << "Pushing down complex filter! "  << std::endl;
    } else {
        std::cout << "No complex filter provided" << std::endl;
    }

    // Move filters into bind_data for later use
    for (auto &filter : filters) {
        std::cout << "Adding filter: " << filter->ToString() << std::endl;
        bind_data.filters.push_back(std::move(filter));
    }

    // Clear filters to indicate they have been consumed
    filters.clear();
}



unique_ptr<GlobalTableFunctionState> simple_init(ClientContext &context, TableFunctionInitInput &input) {
    std::cout << "Initializing Simple Table Function" << std::endl;
    // loop through the columns
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
static unique_ptr<FunctionData> simple_bind(ClientContext &context, TableFunctionBindInput &input, vector<LogicalType> &return_types, vector<string> &names) {
    // Define the columns of the table
    // Create a bind data structure and store the item_name
    auto bind_data = make_uniq<BindArguments>();
    //bind_data->item_name = input.inputs[0].ToString(); // First argument is 'item_name'
    bind_data->filters = vector<unique_ptr<Expression>>();

    for (auto &kv : input.named_parameters) {
        auto loption = StringUtil::Lower(kv.first);
        std::cout << "Found named parameter: " << loption << " = " << kv.second.ToString() << std::endl;
        if (loption == "options") {
            std::string json_str = kv.second.GetValue<std::string>();
            if (!json_str.empty()) {
                bind_data->options = ParseOptionsFromJSON(json_str);
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




    std::cout << "Binding Simple Table Function" << std::endl;

    // for (auto &expression : input.inputs) {
    //     if (expression.type().id() == LogicalTypeId::VARCHAR) {
    //         std::cout << "item_name: " << expression.ToString() << std::endl;
    //     }
    // }

    // Capture limit from named parameters
    // if (input.named_parameters.find("limit") != input.named_parameters.end()) {
    //     bind_data->limit = input.named_parameters["limit"].GetValue<idx_t>();
    // }

    // Capture order_by from named parameters
    // auto order_by_entry = input.named_parameters.find("order_by");
    // if (order_by_entry != input.named_parameters.end()) {
    //     auto order_by = StringValue::Get(order_by_entry->second);
    //     std::cout << "have order_by: " << order_by << std::endl;
    //     bind_data->order_by = order_by;
    // } else {
    //     std::cout << "No order_by provided" << std::endl;
    // }

    // auto &column_ids = input.named_parameters["projected_columns"].GetValue<vector<column_t>>();

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

    auto cfg = load_config();
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

    std::string response_body = query_api(api_url, "");

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

    // names.push_back("pid");
    // return_types.push_back(LogicalType::INTEGER);
 
    // names.push_back("pname");
    // return_types.push_back(LogicalType::VARCHAR);
 
    // names.push_back("age");
    // return_types.push_back(LogicalType::INTEGER);
 
    // return nullptr;  // No additional data needed
    return std::move(bind_data); 
}
 

static void simple_table_function(ClientContext &context, TableFunctionInput &data, DataChunk &output) {

    auto cfg = load_config();

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
        std::cout << "JEEJ!! Options: " << std::endl;
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

    std::string response_body = query_api(api_url, "");

    // std::cout << "Response Body: " << response_body << std::endl;

    //auto api_data = query_api(api_url, config->config);

    json jsonData = nlohmann::json::parse(response_body);

    auto columns = bind_data.columns;

    // vector<std::string> column_names;
    // // Iterate over each object in the JSON array
    // for (const auto& o : columns){
        
    //     column_names.push_back(o.name);
    //     std::cout << "Column Name: " << o.name << std::endl;    
    // }

    size_t row_idx = 0;

    output.SetCardinality(jsonData.size());

    for (const auto& obj : jsonData) {
        // Iterate over each property in the JSON object
        std::cout << "obj: " << obj.dump(4) << std::endl;
        std::cout << "row_idx: " << row_idx << std::endl;

        size_t col_idx = 0;

        for (const auto& c : columns) {
            std::cout << "Column Name: " << c.name << std::endl;
            std::cout << "col_idx: " << col_idx << std::endl;

            if (c.json_type == "number") {
                std::cout << "JSON TYpe Number " << std::endl;
                output.SetValue(col_idx, row_idx, obj[c.name].get<double>()  );
            } else if (c.json_type == "string") {
                std::cout << "JSON Type String " << std::endl;
                output.SetValue(col_idx, row_idx, obj[c.name].get<std::string>()  );
            } else {
                std::cerr << "Unknown JSON Type: " << c.json_type << std::endl;
                // output.SetValue(col_idx, row_idx, c);
            }
            // output.SetValue(col_idx, row_idx, c);
            ++col_idx;
        }
        ++row_idx;
        std::cout << "------" << std::endl;
    }

    // // Get the static test data
    // auto rows = GetStaticTestData();
    // output.SetCardinality(rows.size());
    // // auto api_url = get_env_string("API_URL");

    // // std::cout << "API_URL: " << api_url << std::endl;
    // // Fill the output with data from the current row onward
    // for (idx_t row_idx = 0; row_idx < rows.size(); row_idx++) {
    //     // Fill the DataChunk
    //     output.SetValue(0, row_idx, rows[row_idx][0]); // id
    //     output.SetValue(1, row_idx, rows[row_idx][1]); // name
    //     output.SetValue(2, row_idx, rows[row_idx][2]); // age
    // }


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

    auto simple_table_func = TableFunction("query_json_api", {}, simple_table_function, simple_bind, simple_init);
    // simple_table_func.filter_pushdown = false;
    // simple_table_func.projection_pushdown = false;    
    // simple_table_func.cardinality = simple_cardinality;
    // simple_table_func.pushdown_complex_filter = PushdownComplexFilter;

    simple_table_func.named_parameters["order_by"] = LogicalType::VARCHAR;
    simple_table_func.named_parameters["limit"] = LogicalType::VARCHAR;
    simple_table_func.named_parameters["columns"] = LogicalType::VARCHAR;
    simple_table_func.named_parameters["options"] = LogicalType::VARCHAR;
    simple_table_func.named_parameters["api"] = LogicalType::VARCHAR;

    ExtensionUtil::RegisterFunction(instance, simple_table_func);

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
