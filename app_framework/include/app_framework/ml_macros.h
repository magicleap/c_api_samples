// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once

#include <ml_logging.h>

#define MLRESULT_EXPECT(result, expected, string_fn, level)                                                \
  do {                                                                                                     \
    MLResult _evaluated_result = result;                                                                   \
    ML_LOG_IF(level, (expected) != _evaluated_result, "%s:%d %s (%X)%s", __FILE__, __LINE__, __FUNCTION__, \
              _evaluated_result, (string_fn)(_evaluated_result));                                          \
  } while (0)

#define UNWRAP_MLRESULT(result) MLRESULT_EXPECT((result), MLResult_Ok, MLGetResultString, Error)
#define UNWRAP_MLRESULT_FATAL(result) MLRESULT_EXPECT((result), MLResult_Ok, MLGetResultString, Fatal)

#define UNWRAP_MLPRIVILEGES_RESULT_FATAL(result) \
  MLRESULT_EXPECT((result), MLPrivilegesResult_Granted, MLPrivilegesGetResultString, Fatal)

#define UNWRAP_MLPASSABLE_WORLD_RESULT(result) \
  MLRESULT_EXPECT((result), MLResult_Ok, MLPersistentCoordinateFrameGetResultString, Error)
