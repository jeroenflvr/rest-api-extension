PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME=rest_api
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

# Include the Makefile from extension-ci-tools
CORE_EXTENSIONS='autocomplete;httpfs;icu;parquet;json;delta'
GEN=ninja
include extension-ci-tools/makefiles/duckdb_extension.Makefile