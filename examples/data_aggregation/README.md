# Data Aggregation Example

This example demonstrates a BehaviorNet workflow using only built-in actors:
- **HttpActor**: Fetches data from REST APIs
- **DataStoreActor**: Stores and aggregates fetched data

## Workflow Overview

The workflow fetches weather data from multiple cities and stores the results:

```
[entry] -> [fetch_city1] -> [fetch_city2] -> [aggregate] -> [exit]
```

1. Token enters at `entry` with list of cities to fetch
2. `fetch_city1` fetches weather for first city via HTTP
3. `fetch_city2` fetches weather for second city via HTTP
4. `aggregate` combines results and stores in DataStore
5. Token exits with aggregated data

## Files

- `config.json` - BehaviorNet configuration defining places and transitions
- `main.cpp` - C++ runner that loads config and executes the workflow
- `BUILD.bazel` - Build configuration

## Running the Example

```bash
# From the behaviornet root directory
bazel run //examples/data_aggregation:data_aggregation_example

# Run tests
bazel test //examples/data_aggregation:data_aggregation_test
```

## How It Works

The example uses the test HTTP server to simulate API responses. In production,
you would configure the HttpActor with actual API endpoints.

The DataStoreActor maintains state between action executions, allowing data
from multiple HTTP calls to be aggregated into a single result.
