#pragma once

#include <optional>
#include <utility>

#include "pipeline_op.h"

#include "sources/as_data_flow.h"
#include "sources/dir.h"

#include "adapters/dropnullopt.h"
#include "adapters/filter.h"
#include "adapters/open_files.h"
#include "adapters/split_expected.h"
#include "adapters/split.h"
#include "adapters/transform.h"

#include "terminals/aggregate_by_key.h"
#include "terminals/as_vector.h"
#include "terminals/join.h"
#include "terminals/out.h"
#include "terminals/write.h"

#include "types/join_result.h"
#include "types/kv.h"