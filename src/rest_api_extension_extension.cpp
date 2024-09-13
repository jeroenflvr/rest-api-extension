#define DUCKDB_EXTENSION_MAIN

#include "rest_api_extension_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include <api_request.hpp>

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

namespace duckdb {

 
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
};

unique_ptr<GlobalTableFunctionState> simple_init(ClientContext &context, TableFunctionInitInput &input) {
    auto result = make_uniq<SimpleData>();
    return std::move(result);
}
 
// Bind function to define schema
static unique_ptr<FunctionData> simple_bind(ClientContext &context, TableFunctionBindInput &input, vector<LogicalType> &return_types, vector<string> &names) {
    // Define the columns of the table
    names.push_back("id");
    return_types.push_back(LogicalType::INTEGER);
 
    names.push_back("name");
    return_types.push_back(LogicalType::VARCHAR);
 
    names.push_back("age");
    return_types.push_back(LogicalType::INTEGER);
 
    return nullptr;  // No additional data needed
}
 
// Initialize function operator data (state)

 
// Table function to return static data, with proper state management
static void simple_table_function(ClientContext &context, TableFunctionInput &data, DataChunk &output) {

    auto &data_p = data.global_state->Cast<SimpleData>();
    idx_t data_queries = 1;
    if (data_p.offset >= data_queries) {
        return;
    }

    // Get the static test data
    auto rows = GetStaticTestData();
 
    output.SetCardinality(rows.size());
    auto api_url = get_env_string("API_URL");

    std::cout << "API_URL: " << api_url << std::endl;

    // Fill the output with data from the current row onward
    for (idx_t row_idx = 0; row_idx < rows.size(); row_idx++) {
        // Fill the DataChunk
        output.SetValue(0, row_idx, rows[row_idx][0]); // id
        output.SetValue(1, row_idx, rows[row_idx][1]); // name
        output.SetValue(2, row_idx, rows[row_idx][2]); // age
    }

    data_p.offset++;
 
    // // Update the current row index
    // state.current_row += max_rows;
 
    // // If all rows are processed, signal DuckDB that no more rows are available
    // if (state.current_row >= rows.size()) {
    //     output.SetCardinality(0); // No more rows
    // }
}
 

inline void RestApiExtensionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "RestApiExtension "+name.GetString()+" 🐥");;
        });
}

inline void RestApiExtensionOpenSSLVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "RestApiExtension " + name.GetString() +
                                                     ", my linked OpenSSL version is " +
                                                     OPENSSL_VERSION_TEXT );;
        });
}

static void LoadInternal(DatabaseInstance &instance) {
    // Register a scalar function
    auto rest_api_extension_scalar_function = ScalarFunction("rest_api_extension", {LogicalType::VARCHAR}, LogicalType::VARCHAR, RestApiExtensionScalarFun);
    ExtensionUtil::RegisterFunction(instance, rest_api_extension_scalar_function);

    // Register another scalar function
    auto rest_api_extension_openssl_version_scalar_function = ScalarFunction("rest_api_extension_openssl_version", {LogicalType::VARCHAR},
                                                LogicalType::VARCHAR, RestApiExtensionOpenSSLVersionScalarFun);
    ExtensionUtil::RegisterFunction(instance, rest_api_extension_openssl_version_scalar_function);

    TableFunction simple_table_func("simple_table", {}, simple_table_function, simple_bind, simple_init);
    ExtensionUtil::RegisterFunction(instance, simple_table_func);

}

void RestApiExtensionExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string RestApiExtensionExtension::Name() {
	return "rest_api_extension";
}

std::string RestApiExtensionExtension::Version() const {
#ifdef EXT_VERSION_REST_API_EXTENSION
	return EXT_VERSION_REST_API_EXTENSION;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void rest_api_extension_init(duckdb::DatabaseInstance &db) {
    duckdb::DuckDB db_wrapper(db);
    db_wrapper.LoadExtension<duckdb::RestApiExtensionExtension>();
}

DUCKDB_EXTENSION_API const char *rest_api_extension_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
