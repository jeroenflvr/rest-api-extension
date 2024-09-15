# rest_api_extension
__*DISCLAIMER extremely premature, prototype maybe, MVP*__

This repository is based on https://github.com/duckdb/extension-template, check it out if you want to build and ship your own DuckDB extension.

---

This extension, rest_api_extension, allows you to fetch data from a JSON API.


supports:
- [ ] static schema in api config json
- [ ] schema for the schema in api config json
- [ ] infer schema from the data
- [ ] add custom headers to the request (ie. Bearer token)
- [ ] authenticate (oauth/oidc)
- [x] schema endpoint
- [x] translate datatypes
- [ ] async query results
- [x] set config file location in ~/.duckdbrc
- [ ] fix unit tests
- [ ] ...



## Building
### Managing dependencies
DuckDB extensions uses VCPKG for dependency management. Enabling VCPKG is very simple: follow the [installation instructions](https://vcpkg.io/en/getting-started) or just run the following:
```shell
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_TOOLCHAIN_PATH=`pwd`/vcpkg/scripts/buildsystems/vcpkg.cmake
```
Note: VCPKG is only required for extensions that want to rely on it for dependency management. If you want to develop an extension without dependencies, or want to do your own dependency management, just skip this step. Note that the example extension uses VCPKG to build with a dependency for instructive purposes, so when skipping this step the build may not work without removing the dependency.

### Build steps
Now to build the extension, run:
```sh
make
```
The main binaries that will be built are:
```sh
./build/release/duckdb
./build/release/test/unittest
./build/release/extension/rest_api_extension/rest_api_extension.duckdb_extension
```
- `duckdb` is the binary for the duckdb shell with the extension code automatically loaded.
- `unittest` is the test runner of duckdb. Again, the extension is already linked into the binary.
- `rest_api_extension.duckdb_extension` is the loadable binary as it would be distributed.

## Running the extension
To run the extension code, simply start the shell with `./build/release/duckdb`.

Now we can use the features from the extension directly in DuckDB. The template contains a table function `query_json_api()`.  This function
takes a mandatory argument 'api' and few optional arguments (under construction): order_by (string), limit (int), options (vector<pair<string, string>>).

config
Set in ~/.duckdbrc, or before use
```sql
SET rest_api_config_file = '/users/thisguy/rest_api_extension.json';
```

Check config details from duckdb
```sql
D SELECT current_setting('rest_api_config_file');
┌─────────────────────────────────────────┐
│ current_setting('rest_api_config_file') │
│                 varchar                 │
├─────────────────────────────────────────┤
│ /users/thisguy/rest_api_extension.json  │
└─────────────────────────────────────────┘
D 
```

command
```sql
SELECT * FROM query_json_api(order_by = 'name', options = '{"option1": "value1"}', api='animals') ;
```

with given config
```json
[
    { 
            "name": "peeps",
            "config": {
                "host": "localhost",
                "port": 3000,
                "root_uri": "",
                "endpoints": {
                    "data": {
                        "uri": "data" 
                        },
                    "schema": {
                            "uri": "schema"
                        }
                }
            }
    },
    { 
            "name": "animals",
            "config": {
                "host": "freetestapi.com",
                "port": 443,
                "root_uri": "api",
                "endpoints": {
                    "data": {
                        "uri": "v1/animals" 
                        },
                    "schema": {
                            "uri": "site/animals"
                        }
                }
            }
    }

]


```

```
./build/release/duckdb
v1.0.0 1f98600c2c
Enter ".help" for usage hints.
Connected to a transient in-memory database.
Use ".open FILENAME" to reopen on a persistent database.
D SELECT * FROM query_json_api(order_by = 'name', options = '{"option1": "value1"}', api='animals') ;
Found named parameter: api = animals
API: animals
Found named parameter: options = {"option1": "value1"}
Option: option1 = value1
Found named parameter: order_by = name
named_parameters:
  api: animals
  options: {"option1": "value1"}
  order_by: name

Using configuration: animals
host: freetestapi.com
API URL: https://freetestapi.com:443/api/site/animals
Response Body: {"objects":50,"name":"Animals","access":"/animals","parameters":[{"name":"id","type":"number"},{"name":"name","type":"string"},{"name":"species","type":"string"},{"name":"family","type":"string"},{"name":"habitat","type":"string"},{"name":"place_of_found","type":"string"},{"name":"diet","type":"string"},{"name":"description","type":"string"},{"name":"weight_kg","type":"number"},{"name":"height_cm","type":"number"},{"name":"image","type":"string"}]}

┌────────┬────────────────────┬──────────────────────┬───────────────────┬───┬─────────────┬──────────────────────┬───────────┬───────────┬──────────────────────┐
│   id   │        name        │       species        │      family       │ … │    diet     │     description      │ weight_kg │ height_cm │        image         │
│ double │      varchar       │       varchar        │      varchar      │   │   varchar   │       varchar        │  double   │  double   │       varchar        │
├────────┼────────────────────┼──────────────────────┼───────────────────┼───┼─────────────┼──────────────────────┼───────────┼───────────┼──────────────────────┤
│    1.0 │ Lion               │ Panthera leo         │ Felidae           │ … │ Carnivore   │ The lion is a larg…  │     190.0 │     120.0 │ https://fakeimg.pl…  │
│    2.0 │ Elephant           │ Loxodonta africana   │ Elephantidae      │ … │ Herbivore   │ The elephant is th…  │    6000.0 │     300.0 │ https://fakeimg.pl…  │
│    3.0 │ Tiger              │ Panthera tigris      │ Felidae           │ … │ Carnivore   │ The tiger is a pow…  │     250.0 │     100.0 │ https://fakeimg.pl…  │
│    4.0 │ Kangaroo           │ Macropus             │ Macropodidae      │ … │ Herbivore   │ Kangaroos are mars…  │      85.0 │     150.0 │ https://fakeimg.pl…  │
│    5.0 │ Gorilla            │ Gorilla beringei     │ Hominidae         │ … │ Herbivore   │ Gorillas are large…  │     180.0 │     160.0 │ https://fakeimg.pl…  │
│    6.0 │ Polar Bear         │ Ursus maritimus      │ Ursidae           │ … │ Carnivore   │ Polar bears are la…  │     500.0 │     130.0 │ https://fakeimg.pl…  │
│    7.0 │ Koala              │ Phascolarctos cine…  │ Phascolarctidae   │ … │ Herbivore   │ Koalas are arborea…  │      12.0 │      60.0 │ https://fakeimg.pl…  │
│    8.0 │ Giraffe            │ Giraffa camelopard…  │ Giraffidae        │ … │ Herbivore   │ Giraffes are the t…  │     800.0 │     550.0 │ https://fakeimg.pl…  │
│    9.0 │ Panda              │ Ailuropoda melanol…  │ Ursidae           │ … │ Herbivore   │ Pandas are bears k…  │      85.0 │      90.0 │ https://fakeimg.pl…  │
│   10.0 │ Cheetah            │ Acinonyx jubatus     │ Felidae           │ … │ Carnivore   │ Cheetahs are the f…  │      50.0 │      80.0 │ https://fakeimg.pl…  │
│   11.0 │ Hippopotamus       │ Hippopotamus amphi…  │ Hippopotamidae    │ … │ Herbivore   │ Hippopotamuses are…  │    2000.0 │     150.0 │ https://fakeimg.pl…  │
│   12.0 │ Chimpanzee         │ Pan troglodytes      │ Hominidae         │ … │ Omnivore    │ Chimpanzees are hi…  │      50.0 │     100.0 │ https://fakeimg.pl…  │
│   13.0 │ Red Panda          │ Ailurus fulgens      │ Ailuridae         │ … │ Omnivore    │ Red pandas are sma…  │       5.0 │      50.0 │ https://fakeimg.pl…  │
│   14.0 │ Komodo Dragon      │ Varanus komodoensis  │ Varanidae         │ … │ Carnivore   │ Komodo dragons are…  │      90.0 │     150.0 │ https://fakeimg.pl…  │
│   15.0 │ Orangutan          │ Pongo                │ Hominidae         │ … │ Omnivore    │ Orangutans are gre…  │      70.0 │     130.0 │ https://fakeimg.pl…  │
│   16.0 │ Platypus           │ Ornithorhynchus an…  │ Ornithorhynchidae │ … │ Carnivore   │ Platypuses are uni…  │       2.0 │      20.0 │ https://fakeimg.pl…  │
│   17.0 │ Sloth              │ Folivora             │ Megalonchidae     │ … │ Herbivore   │ Sloths are slow-mo…  │       5.0 │      60.0 │ https://fakeimg.pl…  │
│   18.0 │ Pangolin           │ Manis                │ Manidae           │ … │ Insectivore │ Pangolins are uniq…  │      10.0 │      40.0 │ https://fakeimg.pl…  │
│   19.0 │ Quokka             │ Setonix brachyurus   │ Macropodidae      │ … │ Herbivore   │ Quokkas are small …  │       3.0 │      40.0 │ https://fakeimg.pl…  │
│   20.0 │ Fennec Fox         │ Vulpes zerda         │ Canidae           │ … │ Omnivore    │ Fennec foxes are s…  │       1.5 │      20.0 │ https://fakeimg.pl…  │
│     ·  │   ·                │      ·               │    ·              │ · │    ·        │          ·           │        ·  │        ·  │          ·           │
│     ·  │   ·                │      ·               │    ·              │ · │    ·        │          ·           │        ·  │        ·  │          ·           │
│     ·  │   ·                │      ·               │    ·              │ · │    ·        │          ·           │        ·  │        ·  │          ·           │
│   31.0 │ Zebra              │ Equus zebra          │ Equidae           │ … │ Herbivore   │ Zebras are strikin…  │     300.0 │     150.0 │ https://fakeimg.pl…  │
│   32.0 │ Arctic Fox         │ Vulpes lagopus       │ Canidae           │ … │ Omnivore    │ Arctic foxes are s…  │       3.5 │      25.0 │ https://fakeimg.pl…  │
│   33.0 │ Gibbon             │ Hylobatidae          │ Hylobatidae       │ … │ Omnivore    │ Gibbons are small …  │       6.5 │      50.0 │ https://fakeimg.pl…  │
│   34.0 │ Fossa              │ Cryptoprocta ferox   │ Eupleridae        │ … │ Carnivore   │ Fossas are carnivo…  │       6.5 │      40.0 │ https://fakeimg.pl…  │
│   35.0 │ Puma               │ Puma concolor        │ Felidae           │ … │ Carnivore   │ Pumas, also known …  │      60.0 │      75.0 │ https://fakeimg.pl…  │
│   36.0 │ Gray Wolf          │ Canis lupus          │ Canidae           │ … │ Carnivore   │ Gray wolves, also …  │      45.0 │      80.0 │ https://fakeimg.pl…  │
│   37.0 │ Bison              │ Bison bison          │ Bovidae           │ … │ Herbivore   │ Bisons are large a…  │     900.0 │     200.0 │ https://fakeimg.pl…  │
│   38.0 │ Gharial            │ Gavialis gangeticus  │ Gavialidae        │ … │ Carnivore   │ Gharials are large…  │     350.0 │     500.0 │ https://fakeimg.pl…  │
│   39.0 │ Black Rhinoceros   │ Diceros bicornis     │ Rhinocerotidae    │ … │ Herbivore   │ Black Rhinoceroses…  │    1400.0 │     160.0 │ https://fakeimg.pl…  │
│   40.0 │ Mandrill           │ Mandrillus sphinx    │ Cercopithecidae   │ … │ Omnivore    │ Mandrills are colo…  │      35.0 │      70.0 │ https://fakeimg.pl…  │
│   41.0 │ Beluga Whale       │ Delphinapterus leu…  │ Monodontidae      │ … │ Carnivore   │ Beluga whales, als…  │    1400.0 │     400.0 │ https://fakeimg.pl…  │
│   42.0 │ Manatee            │ Trichechus           │ Trichechidae      │ … │ Herbivore   │ Manatees, also kno…  │     500.0 │     150.0 │ https://fakeimg.pl…  │
│   43.0 │ Snow Leopard       │ Panthera uncia       │ Felidae           │ … │ Carnivore   │ Snow leopards are …  │      50.0 │      60.0 │ https://fakeimg.pl…  │
│   44.0 │ Leopard Seal       │ Hydrurga leptonyx    │ Phocidae          │ … │ Carnivore   │ Leopard seals are …  │     450.0 │     320.0 │ https://fakeimg.pl…  │
│   45.0 │ Okapi              │ Okapia johnstoni     │ Giraffidae        │ … │ Herbivore   │ Okapis are rare an…  │     250.0 │     150.0 │ https://fakeimg.pl…  │
│   46.0 │ Red-eyed Tree Frog │ Agalychnis callidr…  │ Hylidae           │ … │ Insectivore │ The Red-eyed Tree …  │       0.1 │       7.0 │ https://fakeimg.pl…  │
│   47.0 │ Chameleon          │ Chamaeleonidae       │ Squamata          │ … │ Insectivore │ Chameleons are uni…  │       0.1 │      20.0 │ https://fakeimg.pl…  │
│   48.0 │ Blue Whale         │ Balaenoptera muscu…  │ Balaenopteridae   │ … │ Carnivore   │ The blue whale is …  │  200000.0 │    2500.0 │ https://fakeimg.pl…  │
│   49.0 │ Mongoose           │ Herpestidae          │ Herpestidae       │ … │ Carnivore   │ Mongooses are smal…  │       1.5 │      25.0 │ https://fakeimg.pl…  │
│   50.0 │ Capuchin Monkey    │ Cebus                │ Cebidae           │ … │ Omnivore    │ Capuchin monkeys a…  │       3.9 │      40.0 │ https://fakeimg.pl…  │
├────────┴────────────────────┴──────────────────────┴───────────────────┴───┴─────────────┴──────────────────────┴───────────┴───────────┴──────────────────────┤
│ 50 rows (40 shown)                                                                                                                        11 columns (9 shown) │
└────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
D 
```

## Running the tests
Different tests can be created for DuckDB extensions. The primary way of testing DuckDB extensions should be the SQL tests in `./test/sql`. These SQL tests can be run using:
```sh
make test
```

### Installing the deployed binaries
To install your extension binaries from S3, you will need to do two things. Firstly, DuckDB should be launched with the
`allow_unsigned_extensions` option set to true. How to set this will depend on the client you're using. Some examples:

CLI:
```shell
duckdb -unsigned
```

Python:
```python
con = duckdb.connect(':memory:', config={'allow_unsigned_extensions' : 'true'})
```

NodeJS:
```js
db = new duckdb.Database(':memory:', {"allow_unsigned_extensions": "true"});
```

Secondly, you will need to set the repository endpoint in DuckDB to the HTTP url of your bucket + version of the extension
you want to install. To do this run the following SQL query in DuckDB:
```sql
SET custom_extension_repository='bucket.s3.eu-west-1.amazonaws.com/<your_extension_name>/latest';
```
Note that the `/latest` path will allow you to install the latest extension version available for your current version of
DuckDB. To specify a specific version, you can pass the version instead.

After running these steps, you can install and load your extension using the regular INSTALL/LOAD commands in DuckDB:
```sql
INSTALL rest_api_extension
LOAD rest_api_extension
```
