Day4 Chaos Test Report
===================

## Test Summary
- **Total**: 6
- **Passed**: 6
- **Failed**: 0
- **Skipped**: 0

## Test Cases

### chaos_c1: Fully Chaotic (C-type)
- File1: Sequential (A-type baseline)
- File2: Completely disordered (2,0,4,1,3)

### chaos_c2: Partial Backtrack (C-type)
- File1: Sequential with backtrack 2->1.5 (A-type baseline)
- File2: Sequential (A-type)

### chaos_c3: Chaos + Non-numeric (C-type)
- File1: Sequential (A-type baseline)
- File2: Disordered with ERROR, INVALID, empty values

## Results

| Test | Status | Notes |
|------|--------|-------|
| chaos_c1_stream_nearest | PASS | C fallback works, sorting correct |
| chaos_c1_stream_linear | PASS | C fallback works, interpolation correct |
| chaos_c2_stream_nearest | PASS | C fallback works, backtrack handled |
| chaos_c2_stream_linear | PASS | C fallback works, interpolation correct |
| chaos_c3_stream_nearest | PASS | C fallback works, non-numeric preserved |
| chaos_c3_stream_linear | PASS | C fallback works, non-numeric preserved |

## Key Findings

### 1. C-type Fallback Path Verified
- Disordered files correctly identified as C-type
- Full load + sort + binary search works correctly
- Output matches baseline timeline

### 2. Non-numeric Handling
- Non-numeric values (ERROR, INVALID, empty) are preserved as-is
- NOT converted to NaN
- This is actual behavior, not a bug

### 3. Nearest Behavior with Equal Distance
- When target time is equidistant between two points, system picks the earlier point
- e.g., t=1.0 with points at 0.5 and 1.5 -> picks 0.5

## Verified Through Code Review
- openFallbackSeries() loads and sorts all data
- findFallbackBracket() uses binary search
- interpolateBetweenRows() handles nearest/linear

Generated: 2026-03-22
