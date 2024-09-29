
#include <iostream>

#include "duckdb/parser/expression/comparison_expression.hpp"  // Include the comparison expressions
#include "duckdb/parser/expression/conjunction_expression.hpp" // Include conjunction expressions
#include "duckdb/parser/expression/operator_expression.hpp" // Include operator expressions
#include "duckdb/parser/expression/function_expression.hpp"
#include "postgres_parser.hpp"


namespace duckdb {

    void ExtractFilters(duckdb::ParsedExpression &expr) {
        // Handle comparison expressions like "=", "!=", "<", ">", "IN", and "NOT IN"

        std::cout << "RRRRRRRRRRR expression type: " << duckdb::ExpressionTypeToString(expr.type) << std::endl;

        if (expr.GetExpressionClass() == duckdb::ExpressionClass::COMPARISON) {
            auto &comparison = dynamic_cast<duckdb::ComparisonExpression&>(expr);
            std::cout << "Left: " << comparison.left->ToString() << std::endl;

            // Handle different comparison types, including IN/NOT IN
            std::cout << "---------- expression type: " << duckdb::ExpressionTypeToString(expr.type) << std::endl;

            if (expr.type == duckdb::ExpressionType::COMPARE_NOT_IN) {
                std::cout << "Operator: NOT IN" << std::endl;
            } else {
                std::cout << "Operator: " << duckdb::ExpressionTypeToOperator(expr.type) << std::endl;
            }

            // Handle right side of the comparison (e.g., the list for IN/NOT IN)
            if (comparison.right->expression_class == duckdb::ExpressionClass::OPERATOR) {
                auto &op_expr = dynamic_cast<duckdb::OperatorExpression&>(*comparison.right);
                std::cout << "Right: (";
                for (size_t i = 1; i < op_expr.children.size(); ++i) {
                    std::cout << op_expr.children[i]->ToString();
                    if (i < op_expr.children.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << ")" << std::endl;
            } else {
                std::cout << "Right: " << comparison.right->ToString() << std::endl;
            }
        }
        // Handle operator expressions like "IN" and "NOT IN"
        else if (expr.GetExpressionClass() == duckdb::ExpressionClass::OPERATOR) {
            auto &op_expr = dynamic_cast<duckdb::OperatorExpression&>(expr);
            
            // Special handling for "IN" and "NOT IN" which have multiple right-side values
            std::cout << "Left: " << op_expr.children[0]->ToString() << std::endl;
            std::cout << "Operator: " << duckdb::ExpressionTypeToOperator(expr.type) << std::endl;

            std::cout << "Right: (";
            for (size_t i = 1; i < op_expr.children.size(); ++i) {
                std::cout << op_expr.children[i]->ToString();
                if (i < op_expr.children.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << ")" << std::endl;
        }
        // Handle conjunctions (AND/OR)
        else if (expr.GetExpressionClass() == duckdb::ExpressionClass::CONJUNCTION) {
            std::cout << "which level is this? " << std::endl;
            auto &conjunction = dynamic_cast<duckdb::ConjunctionExpression&>(expr);
            std::cout << "Conjunction: " << (expr.type == duckdb::ExpressionType::CONJUNCTION_AND ? "AND" : "OR") << std::endl;
            for (auto &child : conjunction.children) {
                ExtractFilters(*child);
            }
        }

        // Handle function expressions like "LIKE" or "NOT LIKE" if treated as functions
        else if (expr.GetExpressionClass() == duckdb::ExpressionClass::FUNCTION) {
            auto &func_expr = dynamic_cast<duckdb::FunctionExpression&>(expr);
            std::cout << "Function: " << func_expr.function_name << std::endl;
            for (auto &arg : func_expr.children) {
                std::cout << "Argument: " << arg->ToString() << std::endl;
            }
            
            // Assuming the function arguments are the left and right sides
            if (func_expr.children.size() == 2) {
                std::cout << "Left: " << func_expr.children[0]->ToString() << std::endl;
                std::cout << "Right: " << func_expr.children[1]->ToString() << std::endl;
            }
        }


        // Handle other expressions if needed (e.g., function calls, constants)
        else {
            std::cout << "Unhandled expression type: " << expr.ToString() << std::endl;
        }


    }
}