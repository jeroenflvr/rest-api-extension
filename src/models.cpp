#include "duckdb.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include "models.hpp"

namespace duckdb {
    LogicalType JsonToDuckDBType(const std::string& type) {
        if (type == "number") {
            // std::cout << "Converting JSON number to DuckDB DOUBLE\n";
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


    std::string GetRestApiConfigFile(ClientContext &context) {

        std::string name = "rest_api_config_file";

		Value config_file;
		if (!context.TryGetCurrentSetting(name, config_file) ) {
			throw InvalidInputException("Need the parameters damnit");
		}
        // std::cout << "\tOption value from config: " << config_file.ToString() << std::endl;
        return config_file.ToString();
        
    }

    

}



