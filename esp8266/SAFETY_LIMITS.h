/*
 * SAFETY_LIMITS.h
 *
 * Quick Reference: PhantomScript Safety Limits
 * All limits are enforced at runtime to prevent resource exhaustion,
 * memory corruption, and denial of service attacks.
 *
 * Version: 1.1 
 * Last Updated: 2025-10-06
 */

#ifndef PHANTOMSCRIPT_SAFETY_LIMITS_H
#define PHANTOMSCRIPT_SAFETY_LIMITS_H

/*
 * ============================================================================
 * RESOURCE LIMITS
 * ============================================================================
 */

// Maximum number of user-defined variables ($var = value)
// Exceeded: "Maximum number of variables exceeded"
#define PS_MAX_VARIABLES 50

// Maximum number of constants (DEFINE #NAME value)
// Exceeded: "Maximum number of constants exceeded"
#define PS_MAX_CONSTANTS 30

// Maximum number of functions (FUNCTION name ... END_FUNCTION)
// Exceeded: "Maximum number of functions exceeded"
#define PS_MAX_FUNCTIONS 10

/*
 * ============================================================================
 * EXECUTION LIMITS
 * ============================================================================
 */

// Maximum total loop iterations across ALL loops (global counter)
// Prevents nested loops from bypassing per-loop limits
// Exceeded: "WHILE loop exceeded max iterations"
#define PS_MAX_LOOP_ITERATIONS 10000

// Maximum function call depth (recursion limit)
// Conservative for ESP8266's ~4KB stack
// Exceeded: "Maximum call depth exceeded"
#define PS_MAX_CALL_DEPTH 5

// Maximum script execution time in milliseconds (5 minutes)
// Prevents infinite loops and long-running scripts from locking device
// Exceeded: "Script execution timeout (max 5 minutes)"
#define PS_MAX_EXECUTION_TIME 300000

/*
 * ============================================================================
 * MEMORY LIMITS
 * ============================================================================
 */

// Maximum length of a single line in script file
// Lines longer than this are truncated on load
// Prevents memory exhaustion during file loading
#define PS_MAX_LINE_LENGTH 256

// Maximum length of a string literal or string variable
// Enforced in tokenizer and typeString()
// Prevents heap exhaustion and excessively long typing operations
#define PS_MAX_STRING_LENGTH 512

// Maximum number of lines in a script file
// Exceeded: "Script too large (max 1000 lines)"
#define PS_MAX_SCRIPT_LINES 1000

/*
 * ============================================================================
 * CALCULATED WORST-CASE MEMORY USAGE
 * ============================================================================
 */

// Heap Usage (Worst Case):
// - Variables:    50 × 100 bytes  =  5,000 bytes
// - Constants:    30 ×  50 bytes  =  1,500 bytes
// - Functions:    10 ×  50 bytes  =    500 bytes
// - Script Lines: 1000 × 50 bytes = 50,000 bytes
// - Tokenizer:                      5,000 bytes (temporary)
// - Strings:                          512 bytes (limited)
// --------------------------------------------------------
// TOTAL:                          ~62,000 bytes (~62 KB)
//
// ESP8266 Free RAM: ~80 KB
// Safety Margin: ~18 KB (22%)

// Stack Usage (Worst Case):
// - execute():              ~200 bytes
// - parseLine():            ~100 bytes
// - evaluateExpression():  ~1500 bytes (max recursion depth 10)
// - cmdCallFunction():     ~1000 bytes (max depth 5)
// --------------------------------------------------------
// TOTAL:                   ~3000 bytes (~3 KB)
//
// ESP8266 Stack Size: 4 KB
// Safety Margin: 1 KB (25%)

/*
 * ============================================================================
 * ERROR MESSAGES GENERATED
 * ============================================================================
 */

/*
 * Script Loading Errors:
 * - "Failed to open file: <filename>"
 * - "Script too large (max 1000 lines)"
 *
 * Resource Limit Errors:
 * - "Maximum number of variables exceeded"
 * - "Maximum number of constants exceeded"
 * - "Maximum number of functions exceeded"
 *
 * Control Flow Errors:
 * - "WHILE without END_WHILE"
 * - "WHILE loop exceeded max iterations"
 * - "IF without END_IF"
 * - "FUNCTION without END_FUNCTION"
 * - "Function not found: <name>"
 * - "Maximum call depth exceeded"
 *
 * Arithmetic Errors:
 * - "Integer overflow in multiplication"
 * - "Integer overflow in division"
 * - "DEFINE requires numeric value"
 *
 * Syntax Errors:
 * - "Invalid DEFINE syntax"
 * - "DELAY requires milliseconds"
 * - "Unknown command: <command>"
 *
 * Execution Errors:
 * - "Script execution timeout (max 5 minutes)"
 */

/*
 * ============================================================================
 * SECURITY GUARANTEES
 * ============================================================================
 */

/*
 * The following attack vectors are PROTECTED AGAINST:
 *
 * 1. ✅ Memory Exhaustion
 *    - Unlimited variables/constants/functions
 *    - Unbounded string allocations
 *    - Large file uploads
 *    → Mitigation: All resources have hard limits
 *
 * 2. ✅ Stack Overflow
 *    - Deep recursion
 *    - Nested function calls
 *    → Mitigation: Call depth limited to 5
 *
 * 3. ✅ Infinite Loops
 *    - WHILE (1 == 1)
 *    - Nested loops bypassing limits
 *    → Mitigation: Global iteration counter + 5-minute timeout
 *
 * 4. ✅ Integer Overflow
 *    - $x = 2147483647; $x = $x + 1
 *    - $x = 1000000; $x = $x * 1000000
 *    → Mitigation: Overflow checks before operations
 *
 * 5. ✅ Division by Zero
 *    - $x = 10 / 0
 *    → Mitigation: Division silently skipped if divisor == 0
 *
 * 6. ✅ Denial of Service
 *    - Long-running scripts
 *    - Resource exhaustion
 *    → Mitigation: Execution timeout + resource limits
 *
 * 7. ✅ Malformed Input
 *    - Unclosed strings
 *    - Missing END_WHILE/END_IF/END_FUNCTION
 *    → Mitigation: Robust error handling with clear messages
 */

/*
 * ============================================================================
 * TESTING CHECKLIST
 * ============================================================================
 */

/*
 * Before deploying to production, verify these test cases pass:
 *
 * ☐ Test 1: Integer overflow detection
 *   $x = 2147483647
 *   $x = $x + 1
 *   → Expected: Error message
 *
 * ☐ Test 2: Division by zero safety
 *   $x = 10 / 0
 *   → Expected: No crash, $x unchanged
 *
 * ☐ Test 3: String length limit
 *   STRING AAAAAAAAAA... (1000 bytes)
 *   → Expected: Truncated to 512 bytes
 *
 * ☐ Test 4: Nested loop limit
 *   WHILE ($i < 100)
 *       WHILE ($j < 100)
 *           $k = $k + 1
 *       END_WHILE
 *   END_WHILE
 *   → Expected: Error at 10,000 total iterations
 *
 * ☐ Test 5: Deep recursion protection
 *   FUNCTION recurse
 *       recurse
 *   END_FUNCTION
 *   recurse
 *   → Expected: Error at depth 5
 *
 * ☐ Test 6: Variable count limit
 *   $var1 = 1, $var2 = 2, ... $var51 = 51
 *   → Expected: Error at variable 51
 *
 * ☐ Test 7: File size limit
 *   Upload 10MB script with 50,000 lines
 *   → Expected: Rejected at 1,000 lines
 *
 * ☐ Test 8: Execution timeout
 *   WHILE (1 == 1)
 *       DELAY 1000
 *   END_WHILE
 *   → Expected: Timeout after 5 minutes
 *
 * ☐ Test 9: Stop mechanism
 *   Long-running script + WebSocket SCRIPT_STOP
 *   → Expected: Immediate termination
 *
 * ☐ Test 10: Malformed input
 *   STRING "unclosed string
 *   → Expected: Truncated, no crash
 */

/*
 * ============================================================================
 * PERFORMANCE EXPECTATIONS
 * ============================================================================
 */

/*
 * Expected execution times (on ESP8266 @ 80MHz):
 *
 * - Simple script (10 lines):          < 100ms
 * - Medium script (100 lines):         < 1 second
 * - Large script (1000 lines):         < 10 seconds
 * - Maximum complexity script:         < 5 minutes (timeout enforced)
 *
 * Memory overhead per operation:
 * - Variable creation:                 ~100 bytes
 * - Constant creation:                 ~50 bytes
 * - Function definition:               ~50 bytes
 * - Loop iteration:                    ~0 bytes (no allocation)
 * - String typing (512 bytes):         ~30 seconds (60ms per char)
 */

/*
 * ============================================================================
 * TUNING GUIDE
 * ============================================================================
 */

/*
 * If you need to adjust limits for your specific use case:
 *
 * TO INCREASE SAFETY (reduce limits):
 * - Decrease PS_MAX_VARIABLES to 25 (more memory for other tasks)
 * - Decrease PS_MAX_CALL_DEPTH to 3 (more stack safety)
 * - Decrease PS_MAX_EXECUTION_TIME to 60000 (1 minute max)
 *
 * TO ALLOW MORE COMPLEX SCRIPTS (increase limits):
 * - Increase PS_MAX_SCRIPT_LINES to 2000 (if RAM allows)
 * - Increase PS_MAX_LOOP_ITERATIONS to 50000 (longer loops)
 * - Increase PS_MAX_STRING_LENGTH to 1024 (longer strings)
 *
 * WARNING: Increasing limits may cause:
 * - Heap exhaustion (crashes, reboots)
 * - Stack overflow (immediate crash)
 * - Watchdog timer resets (device appears frozen)
 * - Unresponsive web interface
 *
 * ALWAYS test thoroughly after changing limits!
 */

#endif // PHANTOMSCRIPT_SAFETY_LIMITS_H
